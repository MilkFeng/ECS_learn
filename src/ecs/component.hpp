#ifndef COMPONENT_HPP
#define COMPONENT_HPP
#include <array>
#include <optional>
#include <tuple>
#include <type_traits>

#include "type.hpp"

namespace ecs {
/// 要求必须是平凡的类型，必须是 decay 过的类型
template <typename Type>
concept AllowedComponentType = std::is_trivially_copyable_v<Type> &&
    std::is_trivially_default_constructible_v<Type> &&
    std::is_aggregate_v<Type> &&
    std::is_same_v<Type, std::decay_t<Type>>;


/// ComponentTypeId 类型
using ComponentTypeId = TypeId;

namespace internal::duplicate {
template <AllowedComponentType... Components>
constexpr bool CheckDuplicateComponents();

template <AllowedComponentType HeadComponent, AllowedComponentType... TailComponents>
constexpr bool CheckDuplicateComponentsSplit();


template <AllowedComponentType... Components>
constexpr bool CheckDuplicateComponents() {
    return CheckDuplicateComponentsSplit<Components...>();
}

template <>
constexpr bool CheckDuplicateComponents() {
    return false;
}

template <AllowedComponentType HeadComponent, AllowedComponentType... TailComponents>
constexpr bool CheckDuplicateComponentsSplit() {
    if constexpr ((std::is_same_v<HeadComponent, TailComponents> || ...)) {
        // 如果有重复的 Component，返回 false
        return true;
    } else {
        return CheckDuplicateComponents<TailComponents...>();
    }
}

constexpr bool CheckDuplicateComponentTypeIds() {
    return false;
}

constexpr bool CheckDuplicateComponentTypeIds(const ComponentTypeId type_id_1,
                                              const ComponentTypeId type_id_2) {
    return type_id_1 == type_id_2;
}

template <typename... ComponentTypeIds>
    requires (std::is_same_v<ComponentTypeId, std::decay_t<ComponentTypeIds>> && ...)
constexpr bool CheckDuplicateComponentTypeIds(ComponentTypeId head_type_id,
                                              ComponentTypeIds... tail_type_ids) {
    if ((CheckDuplicateComponentTypeIds(head_type_id, tail_type_ids) || ...)) {
        return true;
    } else {
        return CheckDuplicateComponentTypeIds(tail_type_ids...);
    }
}
} // namespace internal::duplicate


namespace internal::components {
/// 使用模板的特化来判断是否是组件元组
template <typename Type>
struct ComponentsTupleTrait;

template <typename Type>
    requires requires { typename Type::TupleType; }
struct ComponentsTupleTrait<Type> : ComponentsTupleTrait<typename Type::TupleType> {
};

template <AllowedComponentType... Components>
struct ComponentsTupleTrait<std::tuple<Components...>> {
    /// 原始元组类型
    using TupleType = std::tuple<std::decay_t<Components>...>;

    /// 引用元组类型，用于 Required 的情况
    using ReferenceTupleType = std::tuple<std::decay_t<Components>&...>;

    /// 指针元组类型，用于 Optional 的情况
    using PointerTupleType = std::tuple<std::decay_t<Components>*...>;

    static constexpr bool is_duplicate_k = duplicate::CheckDuplicateComponents<Components...>();
    static constexpr std::size_t size_k = sizeof...(Components);
    static constexpr std::array<ComponentTypeId, size_k> type_ids_k = {GetTypeId<std::decay_t<Components>>()...};
};
} // namespace internal::components

// 注意在作用域之外写泛型函数时，隐式实例化时的链接性是内部的，所以不需要 static


/// 检查是否有重复的 ComponentTypeId，传入的参数必须是 ComponentTypeId 类型
template <typename... ComponentTypeIds>
    requires (std::is_same_v<ComponentTypeId, std::decay_t<ComponentTypeIds>> && ...)
constexpr bool CheckDuplicateComponentTypeIds(ComponentTypeIds... tail_type_ids) {
    return internal::duplicate::CheckDuplicateComponentTypeIds(tail_type_ids...);
}

/// 检查是否有重复的 Component
template <AllowedComponentType... Components>
constexpr bool CheckDuplicateComponents() {
    return internal::duplicate::CheckDuplicateComponents<Components...>();
}

/// 将 Component 类型转换为 ComponentTypeId 类型列表
template <AllowedComponentType... Components>
std::array<ComponentTypeId, sizeof...(Components)> ToComponentTypeIds() {
    return {GetTypeId<Components>()...};
}


template <typename Type>
struct ComponentsTupleTrait : internal::components::ComponentsTupleTrait<Type> {
    using InternalComponentsTupleTrait = internal::components::ComponentsTupleTrait<Type>;
    using TupleType = typename InternalComponentsTupleTrait::TupleType;

    static constexpr bool is_duplicate_k = InternalComponentsTupleTrait::is_duplicate_k;
    static constexpr std::size_t size_k = InternalComponentsTupleTrait::size_k;
    static constexpr std::array<ComponentTypeId, size_k> type_ids_k = InternalComponentsTupleTrait::type_ids_k;
};

template <typename Type>
concept AllowedComponentsTupleType = requires {
    typename ComponentsTupleTrait<Type>;
};

template <AllowedComponentsTupleType Type>
using UnderlyingTupleType = typename ComponentsTupleTrait<Type>::TupleType;

template <AllowedComponentsTupleType Type>
constexpr bool is_duplicate_k = ComponentsTupleTrait<Type>::is_duplicate_k;

template <AllowedComponentsTupleType Type>
constexpr std::size_t size_k = ComponentsTupleTrait<Type>::size_k;

template <AllowedComponentsTupleType Type>
constexpr std::array<ComponentTypeId, size_k<Type>> type_ids_k =
    ComponentsTupleTrait<Type>::type_ids_k;

template <AllowedComponentsTupleType Type>
using ReferenceTupleType = typename ComponentsTupleTrait<Type>::ReferenceTupleType;

template <AllowedComponentsTupleType Type>
using PointerTupleType = typename ComponentsTupleTrait<Type>::PointerTupleType;


namespace internal::components_duplicate {
template <AllowedComponentsTupleType... ComponentsTuples>
constexpr bool CheckDuplicateComponentsTupleCross();

template <AllowedComponentsTupleType Head, AllowedComponentsTupleType... Tails>
constexpr bool CheckDuplicateComponentsTupleCrossSplit();

template <AllowedComponentsTupleType... ComponentsTuples>
constexpr bool CheckDuplicateComponentsTupleCross() {
    return CheckDuplicateComponentsTupleCrossSplit<ComponentsTuples...>();
}

template <>
constexpr bool CheckDuplicateComponentsTupleCross() {
    return false;
}


#if __cplusplus < 202002L
// 如果是 C++ 17 的话，使用 index_sequence 来实现
// 但是这个项目使用了 concept，所以不能用 C++ 17 编译，这一段代码就当学习了
template <AllowedComponentsTupleType T1, AllowedComponentsTupleType T2, std::size_t ... N1, std::size_t ... N2>
constexpr bool CheckDuplicateComponentsTupleCrossTwoSequence(std::index_sequence<N1...>,
                                                             std::index_sequence<N2...>) {
    return duplicate::CheckDuplicateComponentTypeIds(
        ComponentsTupleTrait<T1>::type_ids_k[N1]...,
        ComponentsTupleTrait<T2>::type_ids_k[N2]...);
}

template <AllowedComponentsTupleType Head, AllowedComponentsTupleType... Tails>
constexpr bool CheckDuplicateComponentsTupleCrossSplit() {
    return (CheckDuplicateComponentsTupleCrossTwoSequence<Head, Tails>(
        std::make_index_sequence<size_k<Head>>(),
        std::make_index_sequence<size_k<Tails>>()
    ) || ...) || CheckDuplicateComponentsTupleCross<Tails...>();
}
#else
// 如果是 CPP 20 的话，直接用编译期 for 循环就可以了
template <AllowedComponentsTupleType T1, AllowedComponentsTupleType T2>
constexpr bool CheckDuplicateComponentsTupleCrossTwoSequence() {
    constexpr std::size_t size_1 = ComponentsTupleTrait<T1>::size_k;
    constexpr std::size_t size_2 = ComponentsTupleTrait<T2>::size_k;
    for (std::size_t i = 0; i < size_1; ++i) {
        for (std::size_t j = 0; j < size_2; ++j) {
            if (ComponentsTupleTrait<T1>::type_ids_k[i] == ComponentsTupleTrait<T2>::type_ids_k[j]) {
                return true;
            }
        }
    }
    return false;
}

template <AllowedComponentsTupleType Head, AllowedComponentsTupleType... Tails>
constexpr bool CheckDuplicateComponentsTupleCrossSplit() {
    return (CheckDuplicateComponentsTupleCrossTwoSequence<Head, Tails>() || ...) ||
        CheckDuplicateComponentsTupleCross<Tails...>();
}
#endif

template <AllowedComponentsTupleType... ComponentsTuples>
constexpr bool CheckDuplicateComponentsTupleInner() {
    return (is_duplicate_k<ComponentsTuples> || ...);
}

template <AllowedComponentsTupleType... ComponentsTuples>
constexpr bool CheckDuplicateComponentsTuple() {
    return CheckDuplicateComponentsTupleInner<ComponentsTuples...>() ||
        CheckDuplicateComponentsTupleCross<ComponentsTuples...>();
}
} // namespace internal::components_duplicate

/// 检查是否有重复的 ComponentsTuple，如果有重复的 Component，返回 true
template <AllowedComponentsTupleType... ComponentsTuples>
constexpr bool CheckDuplicateComponentsTuple() {
    return internal::components_duplicate::CheckDuplicateComponentsTuple<ComponentsTuples...>();
}

template <typename Type>
concept AllowedUniqueComponentsTupleType =
    AllowedComponentsTupleType<Type> && !CheckDuplicateComponentsTuple<Type>();
} // namespace ecs

#endif // COMPONENT_HPP
