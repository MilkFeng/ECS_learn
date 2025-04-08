#ifndef ENTITY_HPP
#define ENTITY_HPP

#include <cstdint>
#include <type_traits>

namespace ecs {
namespace internal {
template <typename Type>
concept RawEntityType = std::is_same_v<Type, std::uint32_t> || std::is_same_v<Type, std::uint64_t>;

template <typename Type>
concept EnumEntityType = std::is_enum_v<Type> && RawEntityType<std::underlying_type_t<Type>>;

template <typename Type>
concept AllowedEntityType = RawEntityType<Type> || EnumEntityType<Type>;

/// 对实体类型的萃取，提取实体的类型、版本号类型、实体掩码、版本掩码
///
/// 仅对实体类型、枚举类型进行特化，其他类型不支持
template <AllowedEntityType Type>
struct EntityTraits;

// 针对枚举类型的特化，使用枚举的底层类型
template <EnumEntityType Type>
struct EntityTraits<Type> : EntityTraits<std::underlying_type_t<Type>> {
    using OriginalType = Type;
};

// 注意子类可以直接访问父类的静态成员，二者是同一个地址

// 针对 32 位整数类型的特化
template <>
struct EntityTraits<std::uint32_t> {
    using OriginalType = std::uint32_t;

    using UnderlyingType = std::uint32_t;
    using IdType = std::uint32_t;
    using VersionType = std::uint32_t;

    static constexpr UnderlyingType entity_shift_k = 20;
    static constexpr UnderlyingType entity_mask_k = 0xFFFFF;
    static constexpr UnderlyingType version_mask_k = 0xFFF;
};

// 针对 64 位整数类型的特化
template <>
struct EntityTraits<std::uint64_t> {
    using OriginalType = std::uint64_t;

    using UnderlyingType = std::uint64_t;
    using IdType = std::uint64_t;
    using VersionType = std::uint64_t;

    static constexpr UnderlyingType entity_shift_k = 32;
    static constexpr UnderlyingType entity_mask_k = 0xFFFFFFFF;
    static constexpr UnderlyingType version_mask_k = 0xFFFFFFFF;
};
} // namespace internal

/// 实体类型的 concept
template <typename Type>
concept AllowedEntityType = requires {
    typename internal::EntityTraits<Type>;
};

/// 实体类型的萃取
template <AllowedEntityType Type>
struct EntityTraits {
    /// 萃取类型
    using InternalTraitsType = internal::EntityTraits<Type>;

    /// 实体的原始类型
    using OriginalType = typename InternalTraitsType::OriginalType;

    /// 实体的底层类型，可以是 32 位整数或 64 位整数
    using UnderlyingType = typename InternalTraitsType::UnderlyingType;

    /// 实体 ID 的类型，可以是 32 位整数或 64 位整数
    using IdType = typename InternalTraitsType::IdType;

    /// 版本号的类型
    using VersionType = typename InternalTraitsType::VersionType;

    /// 实体 ID 的掩码
    static constexpr UnderlyingType entity_mask_k = InternalTraitsType::entity_mask_k;

    /// 版本号的掩码
    static constexpr UnderlyingType version_mask_k = InternalTraitsType::version_mask_k;

    /// 实体 ID 的位移
    static constexpr UnderlyingType entity_shift_k = InternalTraitsType::entity_shift_k;
};

template <AllowedEntityType Type>
using OriginalType = typename EntityTraits<Type>::OriginalType;

template <AllowedEntityType Type>
using UnderlyingType = typename EntityTraits<Type>::UnderlyingType;

template <AllowedEntityType Type>
using IdType = typename EntityTraits<Type>::IdType;

template <AllowedEntityType Type>
using VersionType = typename EntityTraits<Type>::VersionType;

template <AllowedEntityType Type>
constexpr UnderlyingType<Type> entity_mask_k = EntityTraits<Type>::entity_mask_k;

template <AllowedEntityType Type>
constexpr UnderlyingType<Type> version_mask_k = EntityTraits<Type>::version_mask_k;

template <AllowedEntityType Type>
constexpr UnderlyingType<Type> entity_shift_k = EntityTraits<Type>::entity_shift_k;

/// 将枚举类型转换为底层类型
template <AllowedEntityType Type>
constexpr UnderlyingType<Type> ToUnderlying(const OriginalType<Type> value) {
    return static_cast<UnderlyingType<Type>>(value);
}

template <AllowedEntityType Type>
constexpr OriginalType<Type> ToOriginal(const UnderlyingType<Type> value) {
    return static_cast<Type>(value);
}

/// 从 underlying 类型中提取实体 ID
template <AllowedEntityType Type>
constexpr IdType<Type> GetId(const UnderlyingType<Type> underlying) {
    return underlying & entity_mask_k<Type>;
}

/// 从 underlying 类型中提取实体 ID
template <AllowedEntityType Type>
constexpr VersionType<Type> GetVersion(const UnderlyingType<Type> underlying) {
    return (underlying >> entity_shift_k<Type>) & version_mask_k<Type>;
}

/// 将实体 ID 部分和版本号部分组合起来，生成原始类型
template <AllowedEntityType Type>
constexpr UnderlyingType<Type> CombineEntity(const UnderlyingType<Type> entity_part,
                                             const UnderlyingType<Type> version_part) {
    const auto combined = entity_part | (version_part << entity_shift_k<Type>);
    return combined;
}

/// 根据实体 ID 和版本号，生成原始类型
template <AllowedEntityType Type>
constexpr UnderlyingType<Type> MakeEntityUnderlying(const IdType<Type> entity, const VersionType<Type> version) {
    const auto entity_part = entity & entity_mask_k<Type>;
    const auto version_part = version & version_mask_k<Type>;

    return CombineEntity<Type>(entity_part, version_part);
}

/// 生成下一个版本的实体，跳过 entity_mask_k，因为这个值是保留的（用于标记空实体）
template <AllowedEntityType Type>
constexpr UnderlyingType<Type> GenNextVersion(const UnderlyingType<Type> underlying) {
    const auto id = GetId<Type>(underlying);
    const auto version = GetVersion<Type>(underlying);

    constexpr auto entity_mask = entity_mask_k<Type>;
    const auto next_version = version + 1;

    return MakeEntityUnderlying<Type>(id, next_version + (next_version == entity_mask));
}

/// 生成空实体
template <AllowedEntityType Type>
constexpr UnderlyingType<Type> NullEntity() {
    return MakeEntityUnderlying<Type>(entity_mask_k<Type>, version_mask_k<Type>);
}


enum class EntityU32Enum : std::uint32_t {
};

enum class EntityU64Enum : std::uint64_t {
};

using EntityU32 = std::uint32_t;
using EntityU64 = std::uint64_t;
} // namespace ecs

#endif // ENTITY_HPP
