#ifndef VIEWER_HPP
#define VIEWER_HPP

#include "component.hpp"
#include "world.hpp"

namespace ecs {
namespace internal {
template <AllowedEntityType Entity, AllowedComponentType... Components>
class ViewIterator {
};
} // namespace internal

template <AllowedEntityType Entity>
class Viewer;

/// 视图类，用于遍历选定的组件和实体
///
/// @tparam Entity 实体类型
/// @tparam Required 必须的组件类型
/// @tparam Optional 可选的组件类型
/// @tparam Exclude 排除的组件类型
///
template <AllowedEntityType Entity,
    AllowedUniqueComponentsTupleType Required = std::tuple<>,
    AllowedUniqueComponentsTupleType Optional = std::tuple<>,
    AllowedUniqueComponentsTupleType Exclude = std::tuple<>>
class View;

template <AllowedEntityType Entity>
class Viewer {
public:
    using WorldType = World<Entity>;
    using ComponentTypeId = std::size_t;

    template <AllowedUniqueComponentsTupleType Required = std::tuple<>,
        AllowedUniqueComponentsTupleType Optional = std::tuple<>,
        AllowedUniqueComponentsTupleType Exclude = std::tuple<>>
    [[nodiscard]] constexpr View<Entity, Required, Optional, Exclude> View() {
        static_assert(!CheckDuplicateComponentsTuple<Required, Optional, Exclude>(),
                      "Viewer: Duplicate components in Required, Optional or Exclude");
        return ecs::View<Entity, Required, Optional, Exclude>{*this};
    }

private:
    explicit Viewer(WorldType& world): world_(world) {
    }

    WorldType& world() {
        return world_;
    }

    Registry<Entity>& registry() {
        return world_.registry_;
    }

    friend class World<Entity>;

    template <AllowedEntityType Entity2,
        AllowedUniqueComponentsTupleType Required,
        AllowedUniqueComponentsTupleType Optional,
        AllowedUniqueComponentsTupleType Exclude>
    friend class View;

private:
    WorldType& world_;
};


template <AllowedEntityType Entity,
    AllowedUniqueComponentsTupleType Required,
    AllowedUniqueComponentsTupleType Optional,
    AllowedUniqueComponentsTupleType Exclude>
class View {
    static_assert(!CheckDuplicateComponentsTuple<Required, Optional, Exclude>(),
                  "View: Duplicate components in Required, Optional or Exclude");

public:
    using ViewerType = Viewer<Entity>;

    using WorldType = World<Entity>;
    using RegistryType = Registry<Entity>;
    using BasicStorageType = BasicStorage<Entity>;

    using BasicStorageIteratorType = typename BasicStorageType::IteratorType;
    using ConstEntityToComponentsIteratorType = typename RegistryType::ConstEntityToComponentsIteratorType;

    using RequiredTupleType = UnderlyingTupleType<Required>;
    using RequiredReferenceTupleType = ReferenceTupleType<Required>;

    using OptionalTupleType = UnderlyingTupleType<Optional>;
    using OptionalPointerTupleType = PointerTupleType<Optional>;

    using ExcludeTupleType = UnderlyingTupleType<Exclude>;

    /// 返回的类型，对于 Required 返回引用，对于 Optional 返回指针
    using ReturnTupleType = std::tuple<RequiredReferenceTupleType, OptionalPointerTupleType>;

private:
    struct BasicStorageRange {
        BasicStorageIteratorType it;
        BasicStorageIteratorType end;

        [[nodiscard]] constexpr bool HasNext() const {
            return it != end;
        }
    };

    struct ConstEntityToComponentsIteratorRange {
        ConstEntityToComponentsIteratorType it;
        ConstEntityToComponentsIteratorType end;

        [[nodiscard]] constexpr bool HasNext() const {
            return it != end;
        }
    };

public:
    constexpr std::optional<ReturnTupleType> Next() {
        // 先检查是否有 Required 组件
        if (!initialized_) {
            Initialize();
        }

        if (!initialized_) {
            return std::nullopt;
        }

        // 一直遍历，直到找到一个符合条件的 Entity
        std::optional<Entity> entity = std::nullopt;
        while ((entity = NextCandidateEntity())) {
            if (CheckEntity(*entity)) {
                break;
            }
        }

        if (!entity) {
            return std::nullopt;
        } else {
            // 从 registry 中获取这个 Entity 的所有组件
            return GetComponents(*entity);
        }
    }

private:
    explicit View(ViewerType& viewer): viewer_(viewer) {
    }

    constexpr WorldType& world() const {
        return viewer_.world();
    }

    constexpr RegistryType& registry() const {
        return viewer_.registry();
    }

    constexpr std::optional<Entity> NextCandidateEntity() {
        if constexpr (has_required_k) {
            // 如果有 Required 组件，应该遍历 storage

            if (!storage_range_) return std::nullopt;
            if (!storage_range_->HasNext()) return std::nullopt;

            const auto entity = *storage_range_->it;
            ++storage_range_->it;
            return entity;
        } else {
            // 如果没有 Required 组件，应该遍历所有的 Entity

            if (!entity_to_components_range_) return std::nullopt;
            if (!entity_to_components_range_->HasNext()) return std::nullopt;

            const auto entity = entity_to_components_range_->it->first;
            ++entity_to_components_range_->it;
            return entity;
        }
    }

    /// 初始化函数，如果发生错误或者不存在 Required 的所有组件，那么 initialized_ 不会被设置为 true
    constexpr void Initialize() {
        if (!has_required_k) {
            // 如果没有 Required 组件，那么应该从所有的 Entity 中遍历
            entity_to_components_range_ = ConstEntityToComponentsIteratorRange{
                registry().EntityToComponentsBegin(),
                registry().EntityToComponentsEnd()
            };
        } else {
            // 如果有 Required 组件，那么应该从所有的 Entity 中遍历

            // 如果 registry 中不包含 Required 组件的 storage，那么就不需要遍历了
            if (!registry().template HasAllStorageOfComponentsTuple<RequiredTupleType>()) {
                return;
            }

            auto& storage = registry()
                .template GetBasicStorageOfComponent<std::tuple_element_t<0, RequiredTupleType>>();

            // 拿到这个 storage 的迭代器
            storage_range_ = BasicStorageRange{storage.Begin(), storage.End()};
        }

        initialized_ = true;
    }

    /// 检查实体是否符合条件
    constexpr bool CheckEntity(Entity entity) const {
        if (!registry().ContainsEntity(entity)) return false;
        if constexpr (has_required_k) {
            // 检查是否有 Required 组件
            if (!registry().template ContainsAllComponentsTuple<RequiredTupleType>(entity)) return false;
        }

        if constexpr (exclude_size_k > 0) {
            // 检查是否有 Exclude 组件
            if (registry().template ContainsAnyComponentsTuple<ExcludeTupleType>(entity)) return false;
        }
        return true;
    }

    constexpr ReturnTupleType GetComponents(Entity entity) const {
        const auto required_tuple = registry().template GetComponentsReferencesTuple<RequiredTupleType>(entity);

        const auto optional_tuple = registry().template GetComponentsPointersTuple<OptionalTupleType>(entity);

        return {required_tuple, optional_tuple};
    }

private:
    friend class World<Entity>;
    friend class Viewer<Entity>;

private:
    ViewerType& viewer_;

    bool initialized_ = false;
    std::optional<BasicStorageRange> storage_range_;
    std::optional<ConstEntityToComponentsIteratorRange> entity_to_components_range_;

    static constexpr auto required_size_k = size_k<Required>;
    static constexpr auto optional_size_k = size_k<Optional>;
    static constexpr auto exclude_size_k = size_k<Exclude>;

    static constexpr auto has_required_k = required_size_k > 0;

    static constexpr auto required_type_ids_k = type_ids_k<Required>;
    static constexpr auto optional_type_ids_k = type_ids_k<Optional>;
    static constexpr auto exclude_type_ids_k = type_ids_k<Exclude>;
};
} // namespace ecs

#endif // VIEWER_HPP
