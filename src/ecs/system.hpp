#ifndef SYSTEM_HPP
#define SYSTEM_HPP
#include <functional>

namespace ecs {
namespace internal {
template <std::size_t N, typename... Args>
struct FirstNArgs;

template <std::size_t N, typename T, typename... Ts>
    requires (N > 0) && (N <= sizeof...(Ts) + 1)
struct FirstNArgs<N, T, Ts...> {
    using Type = std::tuple<T, typename FirstNArgs<N - 1, Ts...>::Type>;
};

template <typename T, typename... Ts>
struct FirstNArgs<0, T, Ts...> {
    using Type = std::tuple<>;
};


template <std::size_t N, typename... Args>
using FirstNArgsTuple = typename FirstNArgs<N, Args...>::Type;
} // namespace internal

/// System 节点，用于构建 System 依赖图
template <typename... SystemArgs>
struct SystemNode {
    using SystemIdType = std::uint32_t;
    using SystemType = std::function<void(SystemArgs...)>;


    SystemIdType id{};
    SystemType system;

    std::unordered_set<SystemIdType> tos;
    std::unordered_set<SystemIdType> froms;

    SystemNode(const SystemIdType id, const SystemType& system)
        : id(id), system(system), tos(), froms() {
    }

    [[nodiscard]] constexpr std::size_t InDegree() const {
        return froms.size();
    }

    [[nodiscard]] constexpr std::size_t OutDegree() const {
        return tos.size();
    }

    constexpr bool operator==(const SystemNode& other) const {
        return id == other.id;
    }

    constexpr bool operator!=(const SystemNode& other) const {
        return !(*this == other);
    }

    void operator()(SystemArgs... args) const {
        system(args...);
    }
};


/// System 依赖图，用于管理 System 之间的依赖关系，交给调度器来更好地并行执行
///
/// 非线程安全
template <typename... SystemArgs>
class SystemGraph {
public:
    using SystemNodeType = SystemNode<SystemArgs...>;
    using SystemType = typename SystemNodeType::SystemType;
    using SystemIdType = typename SystemNodeType::SystemIdType;

    SystemGraph() noexcept = default;

    SystemGraph(const SystemGraph&) = default;
    SystemGraph& operator=(const SystemGraph&) = default;

    SystemGraph(SystemGraph&&) noexcept = default;
    SystemGraph& operator=(SystemGraph&&) noexcept = default;

    ~SystemGraph() = default;

    constexpr SystemIdType AddSystem(const SystemType& system) {
        SystemIdType node_id = 0;
        if (!free_ids_.empty()) {
            node_id = free_ids_.back();
            free_ids_.pop_back();
        } else {
            node_id = nodes_.size();
        }

        assert(node_id <= nodes_.size());

        if (node_id == nodes_.size()) {
            nodes_.emplace_back(node_id, system);
        } else {
            nodes_[node_id] = {node_id, system};
        }

        return node_id;
    }

    constexpr void AddConstraint(const SystemIdType from_id, const SystemIdType to_id) {
        if (from_id == to_id) {
            throw std::runtime_error("Self loop is not allowed");
        }

        auto& from_node = FindSystemVariable(from_id);
        auto& to_node = FindSystemVariable(to_id);

        from_node.tos.insert(to_id);
        to_node.froms.insert(from_id);
    }

    constexpr void RemoveConstraint(const SystemIdType from_id, const SystemIdType to_id) {
        if (from_id == to_id) return;

        auto& from_node = FindSystemVariable(from_id);
        auto& to_node = FindSystemVariable(to_id);

        from_node.tos.erase(to_id);
        to_node.froms.erase(from_id);
    }

    constexpr bool ContainsConstraint(const SystemIdType from_id, const SystemIdType to_id) const {
        if (from_id == to_id) return false;

        const auto& from_node = FindSystem(from_id);
        return from_node.tos.contains(to_id);
    }

    constexpr void RemoveSystem(const SystemIdType id) {
        auto& node = FindSystemVariable(id);
        assert(node.id == id);

        for (const auto to_id : node.tos) {
            auto& to_node = FindSystemVariable(to_id);
            to_node.froms.erase(id);
        }

        for (const auto from_id : node.froms) {
            auto& from_node = FindSystemVariable(from_id);
            from_node.tos.erase(id);
        }

        node.id = invalid_id_k;
        node.system = nullptr;
        node.tos.clear();
        node.froms.clear();

        free_ids_.push_back(id);
    }

    constexpr bool ContainsSystem(const SystemIdType id) const {
        const auto underlying_id = ToUnderlying<SystemType>(id);
        return underlying_id < nodes_.size() && nodes_[underlying_id].id == id;
    }

    [[nodiscard]] constexpr bool CheckCycle() const {
        std::unordered_set<SystemIdType> stack = {};
        std::unordered_set<SystemIdType> visited = {};

        for (const auto& node : nodes_) {
            if (visited.contains(node.id)) continue;

            stack.clear();
            if (CheckCycleDfs(node.id, visited, stack)) {
                return true;
            }
        }

        return false;
    }

    [[nodiscard]] constexpr std::size_t Size() const {
        return nodes_.size() - free_ids_.size();
    }

    [[nodiscard]] constexpr bool Empty() const {
        return nodes_.empty() || free_ids_.size() == nodes_.size();
    }

    [[nodiscard]] constexpr const std::vector<SystemNodeType>& nodes() const {
        return nodes_;
    }

    constexpr const SystemNodeType& FindSystem(const SystemIdType id) const {
        return const_cast<SystemGraph*>(this)->FindSystemVariable(id);
    }

    constexpr void Clear() {
        nodes_.clear();
        free_ids_.clear();
    }

private:
    constexpr bool CheckCycleDfs(const SystemIdType id,
                                 std::unordered_set<SystemIdType>& visited,
                                 std::unordered_set<SystemIdType>& stack) const {
        stack.insert(id);
        visited.insert(id);

        for (const auto& node = FindSystem(id); const auto to_id : node.tos) {
            if (stack.contains(to_id)) {
                return true;
            }

            if (CheckCycleDfs(to_id, visited, stack)) {
                return true;
            }
        }
        stack.erase(id);
        return false;
    }

    constexpr SystemNodeType& FindSystemVariable(const SystemIdType id) {
        assert(id < nodes_.size());
        if (auto& node = nodes_[id]; node.id == id) {
            return node;
        } else {
            throw std::runtime_error("System not found");
        }
    }

private:
    std::vector<SystemNodeType> nodes_;
    std::vector<SystemIdType> free_ids_;

    static constexpr SystemIdType invalid_id_k = 0;
};
} // namespace ecs

#endif // SYSTEM_HPP
