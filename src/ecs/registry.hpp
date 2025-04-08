#ifndef REGISTRY_HPP
#define REGISTRY_HPP

#include <list>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "component.hpp"
#include "storage.hpp"

namespace ecs {
template <AllowedEntityType Entity>
class Registry {
public:
    using EntityTraits = EntityTraits<Entity>;

    using EntityOriginalType = typename EntityTraits::OriginalType;
    using EntityIdType = typename EntityTraits::IdType;
    using EntityUnderlyingType = typename EntityTraits::UnderlyingType;

    using BasicStorageType = BasicStorage<Entity>;

    using StoragesType = std::unordered_map<ComponentTypeId, std::unique_ptr<BasicStorageType>>;
    using EntityToComponentsType = std::unordered_map<EntityOriginalType, std::unordered_set<ComponentTypeId>>;

    using FreeListType = std::list<EntityUnderlyingType>;

    using ConstStoragesIteratorType = typename StoragesType::const_iterator;
    using ConstEntityToComponentsIteratorType = typename EntityToComponentsType::const_iterator;

    Registry() = default;

    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;

    Registry(Registry&&) noexcept = default;
    Registry& operator=(Registry&&) noexcept = default;

    ~Registry() = default;

    constexpr EntityOriginalType CreateEntity() {
        if (free_list_.empty()) {
            free_list_.push_back(MakeEntityUnderlying<EntityOriginalType>(next_entity_, 0));
            ++next_entity_;
        }

        const auto underlying = free_list_.back();
        free_list_.pop_back();

        const auto entity = ToOriginal<EntityOriginalType>(underlying);
        entity_to_components_[entity] = {};
        return entity;
    }

    constexpr bool ContainsEntity(const EntityOriginalType entity) const {
        return entity_to_components_.contains(entity);
    }

    template <AllowedComponentType Component>
    constexpr BasicStorage<Entity>& GetBasicStorageOfComponent() {
        const auto type_id = GetTypeId<Component>();
        return GetBasicStorageOfComponent(type_id);
    }

    constexpr BasicStorage<Entity>& GetBasicStorageOfComponent(const ComponentTypeId type_id) {
        return *storages_.at(type_id);
    }

    constexpr const BasicStorage<Entity>& GetBasicStorageOfComponentConst(const ComponentTypeId type_id) const {
        return *storages_.at(type_id);
    }

    template <AllowedComponentType Component>
    constexpr Storage<Entity, Component>& GetStorageOfComponent() {
        const auto type_id = GetTypeId<Component>();
        const auto& basic_storage_ptr = storages_.at(type_id);

        return *static_cast<Storage<Entity, Component>*>(basic_storage_ptr.get());
    }

    template <AllowedComponentType Component>
    constexpr const Storage<Entity, Component>& GetStorageOfComponent() const {
        const auto type_id = ecs::GetTypeId<Component>();
        const auto& basic_storage_ptr = storages_.at(type_id);

        return *static_cast<Storage<Entity, Component>*>(basic_storage_ptr.get());
    }

    constexpr bool HasStorageOfComponent(const ComponentTypeId type_id) {
        return storages_.contains(type_id);
    }

    template <AllowedComponentType Component>
    constexpr bool HasStorageOfComponent() {
        const auto type_id = ecs::GetTypeId<Component>();
        return HasStorageOfComponent(type_id);
    }

    template <AllowedComponentType... Components>
    constexpr bool HasAllStorageOfComponents() {
        return (HasStorageOfComponent<Components>() && ...);
    }

    template <typename... ComponentTypeIds>
        requires (std::is_same_v<ComponentTypeId, std::decay_t<ComponentTypeIds>> && ...)
    constexpr bool HasAllStorageOfComponents(ComponentTypeIds... type_ids) {
        return (HasStorageOfComponent(type_ids) && ...);
    }

    template <AllowedComponentsTupleType ComponentsTuple>
    constexpr bool HasAllStorageOfComponentsTuple() {
        for (const auto type_id : type_ids_k<ComponentsTuple>) {
            if (!HasStorageOfComponent(type_id)) return false;
        }
        return true;
    }

    template <AllowedComponentType Component>
    constexpr Storage<Entity, Component>& GetOrCreateStorageOfComponent() {
        const auto type_id = ecs::GetTypeId<Component>();
        auto& storage = storages_[type_id];

        if (!storage) {
            storage = std::make_unique<Storage<Entity, Component>>();
            storages_[type_id] = std::move(storage);
        }

        return *static_cast<Storage<Entity, Component>*>(storage.get());
    }

    template <AllowedComponentType Component>
    constexpr void AttachComponent(const EntityOriginalType entity, Component component) {
        const auto underlying = ToUnderlying<EntityOriginalType>(entity);

        auto& storage = GetOrCreateStorageOfComponent<Component>();
        const auto type_id = ecs::GetTypeId<Component>();
        const auto entity_id = GetId<EntityOriginalType>(underlying);

        assert(storage.ContainsEntity(entity) || !storage.Contains(entity_id));
        entity_to_components_[entity].insert(type_id);
        storage.Upsert(entity, component);
    }

    template <AllowedComponentType... Components>
    constexpr void AttachComponents(const EntityOriginalType entity, Components... components) {
        // 检测是否有重复的 Component
        static_assert(!CheckDuplicateComponents<Components...>(), "Duplicate components");

        (AttachComponent(entity, components), ...);
    }

    constexpr void DetachComponent(const EntityOriginalType entity,
                                   const ComponentTypeId type_id) {
        const auto underlying = ToUnderlying<EntityOriginalType>(entity);

        if (!HasStorageOfComponent(type_id)) return;
        auto& storage = GetBasicStorageOfComponent(type_id);

        const auto entity_id = GetId<EntityOriginalType>(underlying);
        storage.Pop(entity_id);
        entity_to_components_[entity].erase(type_id);
    }

    template <AllowedComponentType Component>
    constexpr void DetachComponent(const EntityOriginalType entity) {
        const auto underlying = ToUnderlying<EntityOriginalType>(entity);

        if (!HasStorageOfComponent<Component>()) return;
        const auto type_id = ecs::GetTypeId<Component>();
        DetachComponent(underlying, type_id);
    }

    template <typename... ComponentTypeIds>
        requires (std::is_same_v<ComponentTypeId, std::decay_t<ComponentTypeIds>> && ...)
    constexpr void DetachComponents(const EntityOriginalType entity,
                                    const ComponentTypeIds... type_ids) {
        if (CheckDuplicateComponentTypeIds(type_ids...)) {
            throw std::runtime_error("Duplicate component type ids");
        }
        const auto underlying = ToUnderlying<EntityOriginalType>(entity);

        (DetachComponent(underlying, type_ids), ...);
    }

    template <typename Iterator>
        requires std::input_iterator<Iterator> &&
        std::is_same_v<ComponentTypeId, std::decay_t<typename std::iterator_traits<Iterator>::value_type>>
    constexpr void DetachComponents(const EntityOriginalType entity,
                                    Iterator begin, Iterator end) {
        const auto underlying = ToUnderlying<EntityOriginalType>(entity);

        for (auto it = begin; it != end; ++it) {
            DetachComponent(underlying, *it);
        }
    }

    template <typename... Components>
    constexpr void DetachComponents(const EntityOriginalType entity) {
        static_assert(!CheckDuplicateComponents<Components...>(), "Duplicate components");
        const auto underlying = ToUnderlying<EntityOriginalType>(entity);
        (DetachComponent<Components>(underlying), ...);
    }

    constexpr void DestroyEntity(const EntityOriginalType entity) {
        const auto underlying = ToUnderlying<EntityOriginalType>(entity);

        const auto type_ids = entity_to_components_[entity];
        for (const auto type_id : type_ids) {
            DetachComponent(entity, type_id);
        }

        entity_to_components_.erase(entity);

        const auto next_underlying = GenNextVersion<EntityOriginalType>(underlying);
        free_list_.push_back(next_underlying);
    }

    constexpr bool ContainsComponent(const EntityOriginalType entity,
                                     const ComponentTypeId type_id) {
        if (!HasStorageOfComponent(type_id)) return false;

        const auto underlying = ToUnderlying<EntityOriginalType>(entity);

        const auto& storage = GetBasicStorageOfComponent(type_id);
        const auto entity_id = GetId<EntityOriginalType>(underlying);

        return storage.Contains(entity_id);
    }

    template <AllowedComponentType Component>
    constexpr bool ContainsComponent(const EntityOriginalType entity) {
        const auto type_id = ecs::GetTypeId<Component>();
        return ContainsComponent(entity, type_id);
    }

    template <typename... ComponentTypeIds>
        requires (std::is_same_v<ComponentTypeId, std::decay_t<ComponentTypeIds>> && ...)
    constexpr bool ContainsAllComponents(const EntityOriginalType entity,
                                         const ComponentTypeIds... type_ids) {
        return (ContainsComponent(entity, type_ids) && ...);
    }

    template <typename... ComponentTypeIds>
        requires (std::is_same_v<ComponentTypeId, std::decay_t<ComponentTypeIds>> && ...)
    constexpr bool ContainsAnyComponents(const EntityOriginalType entity,
                                         const ComponentTypeIds... type_ids) {
        return (ContainsComponent(entity, type_ids) || ...);
    }

    template <AllowedComponentType... Components>
    constexpr bool ContainsAllComponents(const EntityOriginalType entity) {
        return (ContainsComponent<Components>(entity) && ...);
    }

    template <AllowedComponentType... Components>
    constexpr bool ContainsAnyComponents(const EntityOriginalType entity) {
        return (ContainsComponent<Components>(entity) || ...);
    }

    template <AllowedComponentsTupleType ComponentsTuple>
    constexpr bool ContainsAllComponentsTuple(const EntityOriginalType entity) {
        const auto type_ids = type_ids_k<ComponentsTuple>;
        for (const auto type_id : type_ids) {
            if (!ContainsComponent(entity, type_id)) {
                return false;
            }
        }
        return true;
    }

    template <AllowedComponentsTupleType ComponentsTuple>
    constexpr bool ContainsAnyComponentsTuple(const EntityOriginalType entity) {
        const auto type_ids = type_ids_k<ComponentsTuple>;
        for (const auto type_id : type_ids) {
            if (ContainsComponent(entity, type_id)) {
                return true;
            }
        }
        return false;
    }

    template <AllowedComponentType Component>
    constexpr Component& GetComponentReference(const EntityOriginalType entity) {
        const auto type_id = ecs::GetTypeId<Component>();
        auto& storage = GetBasicStorageOfComponent(type_id);

        const auto underlying = ToUnderlying<EntityOriginalType>(entity);
        const auto entity_id = GetId<EntityOriginalType>(underlying);

        // 从 BasicStorage 转换成 Storage
        auto* storage_ptr = static_cast<Storage<Entity, Component>*>(&storage);
        return storage_ptr->ComponentOf(entity_id);
    }

    template <AllowedComponentType Component>
    constexpr Component* GetComponentPointer(const EntityOriginalType entity) {
        const auto type_id = ecs::GetTypeId<Component>();

        if (!HasStorageOfComponent(type_id)) return nullptr;
        auto* storage = &GetBasicStorageOfComponent(type_id);
        if (!storage) return nullptr;

        const auto underlying = ToUnderlying<EntityOriginalType>(entity);
        const auto entity_id = GetId<EntityOriginalType>(underlying);

        if (!storage->Contains(entity_id)) return nullptr;

        // 从 BasicStorage 转换成 Storage
        auto* storage_ptr = static_cast<Storage<Entity, Component>*>(storage);
        return &storage_ptr->ComponentOf(entity_id);
    }


    template <AllowedComponentType... Components>
    constexpr std::tuple<Components&...> GetComponentReferences(const EntityOriginalType entity) {
        return std::tuple<Components&...>(GetComponentReference<Components>(entity)...);
    }

    template <AllowedComponentType... Components>
    constexpr std::tuple<Components*...> GetComponentPointers(const EntityOriginalType entity) {
        return std::tuple<Components*...>(GetComponentPointer<Components>(entity)...);
    }

    template <AllowedComponentsTupleType ComponentsTuple>
    constexpr auto GetComponentsReferencesTuple(const EntityOriginalType entity) {
        return std::apply([this, entity]<AllowedComponentType... Components>(const Components&... components) {
            return GetComponentReferences<Components...>(entity);
        }, ComponentsTuple{});
    }

    template <AllowedComponentsTupleType ComponentsTuple>
    constexpr auto GetComponentsPointersTuple(const EntityOriginalType entity) {
        return std::apply([this, entity]<AllowedComponentType... Components>(const Components&... components) {
            return GetComponentPointers<Components...>(entity);
        }, ComponentsTuple{});
    }

    /// Storage 的数量
    [[nodiscard]] constexpr std::size_t StorageSize() const noexcept {
        return storages_.size();
    }

    ConstStoragesIteratorType StoragesBegin() const noexcept {
        return storages_.begin();
    }

    ConstStoragesIteratorType StoragesEnd() const noexcept {
        return storages_.end();
    }

    /// 实体的数量
    [[nodiscard]] constexpr std::size_t EntityCount() const noexcept {
        return entity_to_components_.size();
    }

    /// 所有的 Entity
    [[nodiscard]] constexpr std::vector<EntityOriginalType> GetAllEntities() const {
        std::vector<EntityOriginalType> entities;
        for (const auto& [entity, _] : entity_to_components_) {
            entities.push_back(entity);
        }
        return entities;
    }

    ConstEntityToComponentsIteratorType EntityToComponentsBegin() const noexcept {
        return entity_to_components_.cbegin();
    }

    ConstEntityToComponentsIteratorType EntityToComponentsEnd() const noexcept {
        return entity_to_components_.cend();
    }

private:
    // 每种 Component 对应一个 Storage
    StoragesType storages_;

    // 存储每个 Entity 由哪些 Component 组成
    EntityToComponentsType entity_to_components_;

    // 里面存的是不用的 Entity 和 Version（已经加 1 的）
    FreeListType free_list_;

    // 当前最大的 Entity 之后的那个
    EntityIdType next_entity_;
};
} // namespace ecs

#endif // REGISTRY_HPP
