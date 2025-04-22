#ifndef COMMANDS_HPP
#define COMMANDS_HPP
#include <functional>

#include "component.hpp"
#include "world.hpp"

namespace ecs {
template <AllowedEntityType Entity>
using Command = std::function<void(World<Entity>&)>;


/// 命令队列，这是一个线程安全的队列
template <typename... CommandArgs>
class CommandQueue {
public:
    using CommandType = std::function<void(CommandArgs...)>;

private:
    struct Node {
        std::shared_ptr<CommandType> data{};
        std::unique_ptr<Node> next{};
    };

public:
    CommandQueue() noexcept :
        head_(std::make_unique<Node>()),
        tail_(head_.get()) {
    }

    CommandQueue(const CommandQueue&) = delete;
    CommandQueue& operator=(const CommandQueue&) = delete;

    CommandQueue(CommandQueue&&) noexcept = default;
    CommandQueue& operator=(CommandQueue&&) noexcept = default;

    ~CommandQueue() = default;

    constexpr void Push(CommandType command) {
        // 将 InvokeType 转换为 CommandType
        std::shared_ptr<CommandType> new_data = std::make_shared<CommandType>(command);
        std::unique_ptr<Node> p = std::make_unique<Node>();
        Node* const new_tail = p.get();

        {
            std::lock_guard lock(tail_m_);
            tail_->data = new_data;
            tail_->next = std::move(p);
            tail_ = new_tail;
        }

        data_cond_.notify_one();
    }

    constexpr std::shared_ptr<CommandType> WaitAndPop() {
        const std::unique_ptr<Node> old_head = WaitPopHead();
        return old_head->data;
    }

    constexpr std::shared_ptr<CommandType> TryPop() {
        const std::unique_ptr<Node> old_head = TryPopHead();
        return old_head ? old_head->data : nullptr;
    }

    [[nodiscard]] constexpr bool Empty() const {
        std::lock_guard lock(head_m_);
        return head_.get() == GetTail();
    }

    constexpr void Execute(CommandArgs... args) {
        std::scoped_lock lock(head_m_, tail_m_);
        while (head_.get() != tail_) {
            std::unique_ptr<Node> old_head = PopHeadNoThreadSafe();
            (*old_head->data)(args...);
        }
    }

    constexpr void Clear() {
        std::scoped_lock lock(head_m_, tail_m_);
        while (head_.get() != tail_) {
            std::unique_ptr<Node> old_head = PopHeadNoThreadSafe();
            old_head.reset();
        }
    }

private:
    constexpr Node* GetTail() {
        std::lock_guard lock(tail_m_);
        return tail_;
    }

    constexpr std::unique_ptr<Node> PopHeadNoThreadSafe() {
        std::unique_ptr<Node> old_head = std::move(head_);
        head_ = std::move(old_head->next);
        return old_head;
    }

    constexpr std::unique_ptr<Node> WaitPopHead() {
        std::unique_lock lock(head_m_);
        data_cond_.wait(lock, [this] {
            return head_.get() != GetTail();
        });

        return PopHeadNoThreadSafe();
    }

    constexpr std::unique_ptr<Node> TryPopHead() {
        std::lock_guard lock(head_m_);
        if (head_.get() == GetTail()) return nullptr;

        return PopHeadNoThreadSafe();
    }

private:
    std::mutex head_m_;
    std::mutex tail_m_;

    std::condition_variable data_cond_;

    std::unique_ptr<Node> head_;
    Node* tail_;

    // std::queue<CommandType> queue_;
};


namespace internal {
template <AllowedEntityType Entity, AllowedComponentType... Components>
struct SpawnCommand {
    static_assert(!CheckDuplicateComponents<Components...>(),
                  "SpawnCommand: Duplicate components in SpawnCommand");

    using CommandType = Command<Entity>;

    explicit SpawnCommand(Components... components) noexcept :
        components_(std::forward<Components>(components)...) {
    }

    void operator()(World<Entity>& world) const {
        const auto entity = world.registry().CreateEntity();
        world.registry().AttachComponents(entity, std::get<Components>(components_)...);
    }

    std::tuple<Components...> components_;
};


template <AllowedEntityType Entity>
struct DestroyCommand {
    using CommandType = Command<Entity>;

    explicit DestroyCommand(const Entity entity) noexcept : entity_(entity) {
    }

    void operator()(World<Entity>& world) const {
        world.registry().DestroyEntity(entity_);
    }

    Entity entity_;
};

template <AllowedEntityType Entity, AllowedComponentType... Components>
struct AttachCommand {
    static_assert(!CheckDuplicateComponents<Components...>(),
                  "AttachCommand: Duplicate components in AttachCommand");

    using CommandType = Command<Entity>;

    explicit AttachCommand(const Entity entity, Components... components) noexcept
        : entity_(entity), components_(components...) {
    }

    void operator()(World<Entity>& world) const {
        const auto underlying = ToUnderlying<Entity>(entity_);
        world.registry().AttachComponents(underlying, std::get<Components>(components_)...);
    }

    Entity entity_;
    std::tuple<Components...> components_;
};

template <AllowedEntityType Entity, AllowedComponentType... Components>
struct DetachCommand {
    using CommandType = Command<Entity>;

    explicit DetachCommand(const Entity entity) noexcept : entity_(entity) {
    }

    void operator()(World<Entity>& world) const {
        const auto underlying = ToUnderlying<Entity>(entity_);
        const auto type_ids = ToComponentTypeIds<Components...>();
        world.registry().DetachComponents(underlying, type_ids);
    }

    Entity entity_;
};

template <AllowedEntityType Entity, AllowedResourceType Resource>
struct AddResourceCommand {
    using CommandType = Command<Entity>;

    explicit AddResourceCommand(const Resource resource) noexcept : resource_(resource) {
    }

    void operator()(World<Entity>& world) const {
        world.resources().UpsertResource(resource_);
    }

    Resource resource_;
};

template <AllowedEntityType Entity, AllowedResourceType Resource>
struct RemoveResourceCommand {
    using CommandType = Command<Entity>;

    explicit RemoveResourceCommand() noexcept = default;

    void operator()(World<Entity>& world) const {
        world.resources().template RemoveResource<Resource>();
    }
};
} // namespace internal


template <AllowedEntityType Entity>
class Commands {
public:
    using WorldType = World<Entity>;

    using CommandQueueType = CommandQueue<WorldType&>;
    using CommandType = typename CommandQueueType::CommandType;

    using EntityTraits = EntityTraits<Entity>;

    using EntityOriginalType = typename EntityTraits::OriginalType;
    using EntityIdType = typename EntityTraits::IdType;
    using EntityUnderlyingType = typename EntityTraits::UnderlyingType;

    explicit Commands(WorldType& world) noexcept : world_(world), command_queue_() {
    }

    Commands(const Commands&) = delete;
    Commands& operator=(const Commands&) = delete;

    Commands(Commands&&) noexcept = default;
    Commands& operator=(Commands&&) noexcept = default;

    ~Commands() = default;

    template <AllowedComponentType... Components>
    constexpr Commands& Spawn(Components... components) {
        Command<Entity> command = internal::SpawnCommand<Entity, Components...>(components...);

        command_queue_.Push(command);
        return *this;
    }

    constexpr Commands& Destroy(const Entity entity) {
        Command<Entity> command = internal::DestroyCommand<Entity>(entity);
        command_queue_.Push(command);
        return *this;
    }

    template <AllowedComponentType... Components>
    constexpr Commands& Attach(const Entity entity, Components&&... components) {
        Command<Entity> command = internal::AttachCommand<Entity, Components...>(entity, components...);
        command_queue_.Push(command);
        return *this;
    }

    template <AllowedComponentType... Components>
    constexpr Commands& Detach(const Entity entity) {
        Command<Entity> command = internal::DetachCommand<Entity, Components...>(entity);
        command_queue_.Push(command);
        return *this;
    }

    template <AllowedResourceType Resource>
    constexpr Commands& AddResource(Resource resource) {
        Command<Entity> command = internal::AddResourceCommand<Entity, Resource>(resource);
        command_queue_.Push(command);
        return *this;
    }

    template <AllowedResourceType Resource>
    constexpr Commands& AddResource() {
        Command<Entity> command = internal::AddResourceCommand<Entity, Resource>(Resource{});
        command_queue_.Push(command);
        return *this;
    }

    template <AllowedResourceType Resource>
    constexpr Commands& RemoveResource() {
        Command<Entity> command = internal::RemoveResourceCommand<Entity, Resource>();
        command_queue_.Push(command);
        return *this;
    }

    constexpr void Execute() {
        command_queue_.Execute(world_);
    }

    constexpr void Clear() {
        command_queue_.Clear();
    }

    constexpr void Append(Commands& other) {
        command_queue_.Append(other.command_queue_);
    }

    [[nodiscard]] constexpr bool Empty() const {
        return command_queue_.Empty();
    }

    [[nodiscard]] constexpr std::size_t Size() const {
        return command_queue_.Size();
    }

private:
    WorldType& world_;
    CommandQueueType command_queue_;
};
} // namespace ecs

#endif // COMMANDS_HPP
