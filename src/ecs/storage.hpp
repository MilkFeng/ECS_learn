#ifndef STORAGE_HPP
#define STORAGE_HPP

#include <cassert>
#include <memory>
#include <vector>

#include "entity.hpp"
#include "component.hpp"

namespace ecs {
namespace internal {
template <typename Storage, typename ComponentPayload>
struct StorageIterator;

template <typename Storage>
struct StorageIterator<Storage, void> {
public:
    using EntityOriginalType = typename Storage::EntityOriginalType;

    using value_type = EntityOriginalType;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;
    using iterator_category = std::random_access_iterator_tag;

    constexpr StorageIterator() = delete;

    explicit constexpr StorageIterator(Storage* storage) noexcept
        : storage_(storage), current_(0) {
    }

    constexpr StorageIterator(Storage* storage, const difference_type current) noexcept
        : storage_(storage), current_(current) {
    }

    constexpr StorageIterator(const StorageIterator&) noexcept = default;
    constexpr StorageIterator& operator=(const StorageIterator&) noexcept = default;

    constexpr StorageIterator(StorageIterator&&) noexcept = default;
    constexpr StorageIterator& operator=(StorageIterator&&) noexcept = default;

    constexpr ~StorageIterator() = default;

    [[nodiscard]] constexpr reference operator*() const noexcept {
        return storage_->entity_packed_[current_];
    }

    [[nodiscard]] constexpr pointer operator->() const noexcept {
        return &storage_->entity_packed_[current_];
    }

    constexpr StorageIterator& operator++() noexcept {
        ++current_;
        return *this;
    }

    constexpr StorageIterator operator++(int) noexcept {
        StorageIterator temp(*this);
        ++current_;
        return temp;
    }

    constexpr StorageIterator& operator--() noexcept {
        --current_;
        return *this;
    }

    constexpr StorageIterator operator--(int) noexcept {
        StorageIterator temp(*this);
        --current_;
        return temp;
    }

    constexpr bool operator==(const StorageIterator& other) const noexcept {
        return current_ == other.current_;
    }

    constexpr StorageIterator operator+(const difference_type n) const noexcept {
        return StorageIterator(storage_, current_ + n);
    }

    constexpr StorageIterator operator-(const difference_type n) const noexcept {
        return StorageIterator(storage_, current_ - n);
    }

    constexpr StorageIterator& operator+=(const difference_type n) noexcept {
        current_ += n;
        return *this;
    }

    constexpr StorageIterator& operator-=(const difference_type n) noexcept {
        current_ -= n;
        return *this;
    }

    [[nodiscard]] constexpr reference operator[](const difference_type n) const noexcept {
        return storage_->component_packed_[current_ + n];
    }

    [[nodiscard]] constexpr difference_type index() const noexcept {
        return current_;
    }

protected:
    Storage* storage_;
    difference_type current_;
};

template <typename Storage, typename ComponentPayload>
struct StorageIterator : StorageIterator<Storage, void> {
public:
    using BasicStorageIteratorType = StorageIterator<Storage, void>;

    using EntityOriginalType = typename Storage::EntityOriginalType;

    using value_type = std::pair<EntityOriginalType, ComponentPayload>;
    using difference_type = std::ptrdiff_t;
    using pointer = std::pair<EntityOriginalType*, ComponentPayload*>;
    using reference = std::pair<EntityOriginalType&, ComponentPayload&>;
    using iterator_category = std::random_access_iterator_tag;

    constexpr StorageIterator() = delete;

    explicit constexpr StorageIterator(Storage* storage) noexcept
        : BasicStorageIteratorType(storage) {
    }

    constexpr StorageIterator(Storage* storage, const difference_type current) noexcept
        : BasicStorageIteratorType(storage, current) {
    }

    constexpr StorageIterator(const StorageIterator&) noexcept = default;
    constexpr StorageIterator& operator=(const StorageIterator&) noexcept = default;

    constexpr StorageIterator(StorageIterator&&) noexcept = default;
    constexpr StorageIterator& operator=(StorageIterator&&) noexcept = default;

    constexpr ~StorageIterator() = default;

    [[nodiscard]] constexpr reference operator*() const noexcept {
        reference ref = {
            BasicStorageIteratorType::storage_->entity_packed_[BasicStorageIteratorType::current_],
            BasicStorageIteratorType::storage_->component_packed_[BasicStorageIteratorType::current_]
        };
        return ref;
    }

    [[nodiscard]] constexpr pointer operator->() const noexcept {
        pointer ptr = {
            &BasicStorageIteratorType::storage_->entity_packed_[BasicStorageIteratorType::current_],
            &BasicStorageIteratorType::storage_->component_packed_[BasicStorageIteratorType::current_]
        };
        return ptr;
    }
};

template <typename Storage>
using BasicStorageIterator = StorageIterator<Storage, void>;
} // namespace internal

/// 基础存储组件，是 Storage 的基类，里面有虚函数，用于放在 Registry 中
template <AllowedEntityType Entity>
class BasicStorage {
public:
    using EntityTraits = EntityTraits<Entity>;

    using EntityOriginalType = typename EntityTraits::OriginalType;
    using EntityIdType = typename EntityTraits::IdType;
    using EntityUnderlyingType = typename EntityTraits::UnderlyingType;

    // 稀疏数组，Entity 作为索引，Component 在数组中的位置作为值
    using SparseContainerType = std::vector<EntityIdType>;

    // 紧凑数组，存储 UnderlyingEntity（Entity + Version）
    using PackedEntityContainerType = std::vector<EntityOriginalType>;

    using IteratorType = internal::BasicStorageIterator<BasicStorage>;
    using ConstIteratorType = internal::BasicStorageIterator<const BasicStorage>;
    using ReverseIteratorType = std::reverse_iterator<IteratorType>;

    BasicStorage() noexcept : sparse_(), entity_packed_() {
    }

    BasicStorage(const BasicStorage&) = delete;
    BasicStorage& operator=(const BasicStorage&) = delete;

    BasicStorage(BasicStorage&& other) noexcept : sparse_(std::move(other.sparse_)),
                                                  entity_packed_(std::move(other.entity_packed_)) {
    }

    BasicStorage& operator=(BasicStorage&& other) noexcept {
        // 一定要检查自赋值
        if (this != &other) {
            sparse_ = std::move(other.sparse_);
        }

        return *this;
    }

    virtual ~BasicStorage() = default;

    constexpr bool Contains(const EntityIdType entity_id) const noexcept {
        return sparse_.size() > entity_id && sparse_[entity_id] != 0;
    }

    constexpr bool ContainsUnderlying(const EntityUnderlyingType underlying) const noexcept {
        const auto entity_id = GetId<EntityOriginalType>(underlying);
        if (!Contains(entity_id)) return false;
        const auto entity_packed_underlying = ToUnderlying<EntityOriginalType>(entity_packed_[IndexOf(entity_id)]);
        return entity_packed_underlying == underlying;
    }

    constexpr bool ContainsEntity(const EntityOriginalType entity) const noexcept {
        const auto underlying = ToUnderlying<EntityOriginalType>(entity);
        return ContainsUnderlying(underlying);
    }

    constexpr std::size_t IndexOf(const EntityIdType entity_id) const noexcept {
        return sparse_[entity_id] - 1;
    }

    constexpr EntityOriginalType& EntityOf(const EntityIdType id) noexcept {
        return entity_packed_[IndexOf(id)];
    }

    constexpr const EntityOriginalType& EntityOf(const EntityIdType id) const noexcept {
        return entity_packed_[IndexOf(id)];
    }

    virtual constexpr void Upsert(const EntityOriginalType entity) {
        const auto underlying = ToUnderlying<EntityOriginalType>(entity);
        const auto id = GetId<EntityOriginalType>(underlying);
        const auto version = GetVersion<EntityOriginalType>(underlying);

        if (Contains(id)) {
            const auto index = sparse_[id] - 1;
            EntityOf(id) = entity;
        } else {
            AssureEntity(id);
            sparse_[id] = entity_packed_.size() + 1;
            entity_packed_.push_back(entity);
        }
    }

    virtual constexpr void Pop(const EntityIdType entity_id) {
        if (!Contains(entity_id)) return;

        BasicStorage::SwapToBack(entity_id);
        entity_packed_.pop_back();
        sparse_[entity_id] = 0;
    }

    virtual constexpr void SwapToBack(const EntityIdType entity_id) {
        const auto index = sparse_[entity_id] - 1;
        const auto last_underlying = ToUnderlying<EntityOriginalType>(entity_packed_.back());
        const auto last_entity_id = GetId<EntityOriginalType>(last_underlying);
        BasicStorage::Swap(entity_id, last_entity_id);
    }

    virtual constexpr void Swap(const EntityIdType entity_id1,
                                const EntityIdType entity_id2) {
        if (entity_id1 == entity_id2) return;

        const auto index1 = IndexOf(entity_id1);
        const auto index2 = IndexOf(entity_id2);

        std::swap(entity_packed_[index1], entity_packed_[index2]);

        sparse_[entity_id1] = index2 + 1;
        sparse_[entity_id2] = index1 + 1;
    }

    constexpr void AssureEntity(const EntityIdType entity_id) {
        if (entity_id >= sparse_.size()) {
            sparse_.resize(entity_id + 1);
        }
    }

    virtual constexpr void Reserve(const std::size_t n) {
        entity_packed_.reserve(n);
    }

    virtual constexpr void ShrinkToFit() {
        entity_packed_.shrink_to_fit();
    }

    [[nodiscard]] constexpr std::size_t Size() const noexcept {
        return entity_packed_.size();
    }

    [[nodiscard]] constexpr bool Empty() const noexcept {
        return entity_packed_.empty();
    }

    [[nodiscard]] constexpr std::size_t Capacity() const noexcept {
        return entity_packed_.capacity();
    }

    constexpr IteratorType Begin() noexcept {
        return IteratorType(this);
    }

    constexpr IteratorType End() noexcept {
        return IteratorType(this, entity_packed_.size());
    }

    constexpr ConstIteratorType ConstBegin() const noexcept {
        return ConstIteratorType(this);
    }

    constexpr ConstIteratorType ConstEnd() const noexcept {
        return ConstIteratorType(this, entity_packed_.size());
    }

    constexpr ReverseIteratorType ReverseBegin() noexcept {
        return ReverseIteratorType(this);
    }

    constexpr ReverseIteratorType ReverseEnd() noexcept {
        return ReverseIteratorType(this, entity_packed_.size());
    }

private:
    friend struct internal::StorageIterator<BasicStorage, void>;

    friend IteratorType begin(BasicStorage& storage) noexcept { return storage.Begin(); }
    friend IteratorType end(BasicStorage& storage) noexcept { return storage.End(); }
    friend ConstIteratorType cbegin(const BasicStorage& storage) noexcept { return storage.ConstBegin(); }
    friend ConstIteratorType cend(const BasicStorage& storage) noexcept { return storage.ConstEnd(); }
    friend ReverseIteratorType rbegin(BasicStorage& storage) noexcept { return storage.ReverseBegin(); }
    friend ReverseIteratorType rend(BasicStorage& storage) noexcept { return storage.ReverseEnd(); }

protected:
    SparseContainerType sparse_;
    PackedEntityContainerType entity_packed_;
};

/// 使用稀疏集合存储组件
template <AllowedEntityType Entity, AllowedComponentType Component>
class Storage final : public BasicStorage<Entity> {
public:
    // 基类，定义在这里是为了方便使用其中的类型
    using BasicStorageType = BasicStorage<Entity>;

    using EntityOriginalType = typename BasicStorageType::EntityOriginalType;
    using EntityIdType = typename BasicStorageType::EntityIdType;
    using EntityUnderlyingType = typename BasicStorageType::EntityUnderlyingType;

    // 稀疏数组，Entity 作为索引，Component 在数组中的位置作为值
    using SparseContainerType = typename BasicStorageType::SparseContainerType;

    // 紧凑数组，存储 UnderlyingEntity（Entity + Version）
    using PackedEntityContainerType = typename BasicStorageType::PackedEntityContainerType;

    // 组件类型
    using ComponentType = Component;

    // 紧凑数组，存储 Component
    using PackedComponentContainerType = std::vector<ComponentType>;

    // 迭代器类型
    using IteratorType = internal::StorageIterator<Storage, ComponentType>;
    using ConstIteratorType = internal::StorageIterator<const Storage, const ComponentType>;
    using ReverseIteratorType = std::reverse_iterator<IteratorType>;

    using BasicIteratorType = typename BasicStorageType::IteratorType;
    using BasicConstIteratorType = typename BasicStorageType::ConstIteratorType;
    using BasicReverseIteratorType = typename BasicStorageType::ReverseIteratorType;

    Storage() noexcept : BasicStorageType(), component_packed_() {
    }

    Storage(const Storage&) = delete;
    Storage& operator=(const Storage&) = delete;

    Storage(Storage&& other) noexcept : BasicStorageType(std::move(other)),
                                        component_packed_(std::move(other.component_packed_)) {
    }

    Storage& operator=(Storage&& other) noexcept {
        // 一定要检查自赋值
        if (this != &other) {
            BasicStorageType::operator=(std::move(other));
            component_packed_ = std::move(other.component_packed_);
        }

        return *this;
    }

    ~Storage() override = default;

    constexpr ComponentType& ComponentOf(const EntityIdType entity_id) {
        return component_packed_[BasicStorageType::IndexOf(entity_id)];
    }

    constexpr const ComponentType& ComponentOf(const EntityIdType entity_id) const {
        return component_packed_[BasicStorageType::IndexOf(entity_id)];
    }

    /// 将 Component 插入到 Storage 中，使用万能引用
    constexpr void Upsert(const EntityOriginalType entity, ComponentType component) {
        BasicStorageType::Upsert(entity);

        const auto underlying = ToUnderlying<EntityOriginalType>(entity);
        const auto id = GetId<EntityOriginalType>(underlying);
        const auto index = BasicStorageType::IndexOf(id);

        assert(index <= component_packed_.size());

        if (index == component_packed_.size()) {
            component_packed_.emplace_back(component);
        } else if (index < component_packed_.size()) {
            component_packed_[index] = component;
        }
    }

    constexpr void Upsert(const EntityOriginalType entity) override {
        Storage::Upsert(entity, {});
    }

    constexpr void Pop(const EntityIdType entity_id) override {
        if (!BasicStorageType::Contains(entity_id)) return;

        Storage::SwapToBack(entity_id);

        component_packed_.pop_back();
        BasicStorageType::entity_packed_.pop_back();
        BasicStorageType::sparse_[entity_id] = 0;
    }

    constexpr IteratorType Begin() noexcept {
        return IteratorType(this);
    }

    constexpr IteratorType End() noexcept {
        return IteratorType(this, component_packed_.size());
    }

    constexpr ConstIteratorType ConstBegin() const noexcept {
        return ConstIteratorType(this);
    }

    constexpr ConstIteratorType ConstEnd() const noexcept {
        return ConstIteratorType(this, component_packed_.size());
    }

    constexpr ReverseIteratorType ReverseBegin() noexcept {
        return ReverseIteratorType(End());
    }

    constexpr ReverseIteratorType ReverseEnd() noexcept {
        return ReverseIteratorType(Begin());
    }

    constexpr BasicIteratorType BasicBegin() noexcept {
        return BasicStorageType::Begin();
    }

    constexpr BasicIteratorType BasicEnd() noexcept {
        return BasicStorageType::End();
    }

    constexpr BasicConstIteratorType BasicConstBegin() const noexcept {
        return BasicStorageType::ConstBegin();
    }

    constexpr BasicConstIteratorType BasicConstEnd() const noexcept {
        return BasicStorageType::ConstEnd();
    }

    constexpr BasicReverseIteratorType BasicReverseBegin() noexcept {
        return BasicStorageType::ReverseBegin();
    }

    constexpr BasicReverseIteratorType BasicReverseEnd() noexcept {
        return BasicStorageType::ReverseEnd();
    }

    constexpr BasicStorageType& ToBasicStorage() noexcept {
        return *this;
    }

    constexpr void Reserve(const std::size_t n) override {
        BasicStorageType::Reserve(n);
        component_packed_.reserve(n);
    }

    constexpr void ShrinkToFit() override {
        BasicStorageType::ShrinkToFit();
        component_packed_.shrink_to_fit();
    }

    constexpr void SwapToBack(const EntityIdType entity_id) override {
        const auto index = BasicStorageType::sparse_[entity_id] - 1;
        const auto last_underlying = ToUnderlying<EntityOriginalType>(BasicStorageType::entity_packed_.back());
        const auto last_entity_id = GetId<EntityOriginalType>(last_underlying);
        Storage::Swap(entity_id, last_entity_id);
    }

    constexpr void Swap(const EntityIdType entity_id1,
                        const EntityIdType entity_id2) override {
        BasicStorageType::Swap(entity_id1, entity_id2);
        std::swap(component_packed_[BasicStorageType::IndexOf(entity_id1)],
                  component_packed_[BasicStorageType::IndexOf(entity_id2)]);
    }

private:
    friend struct internal::StorageIterator<Storage, ComponentType>;
    friend struct internal::StorageIterator<Storage, void>;

    friend IteratorType begin(Storage& storage) noexcept { return storage.Begin(); }
    friend IteratorType end(Storage& storage) noexcept { return storage.End(); }
    friend ConstIteratorType cbegin(const Storage& storage) noexcept { return storage.ConstBegin(); }
    friend ConstIteratorType cend(const Storage& storage) noexcept { return storage.ConstEnd(); }
    friend ReverseIteratorType rbegin(Storage& storage) noexcept { return storage.ReverseBegin(); }
    friend ReverseIteratorType rend(Storage& storage) noexcept { return storage.ReverseEnd(); }

private:
    PackedComponentContainerType component_packed_;
};
} // namespace esc

#endif // STORAGE_HPP
