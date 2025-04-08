#ifndef TYPE_HPP
#define TYPE_HPP
#include <cstddef>
#include <functional>
#include <string_view>


namespace ecs {
using TypeId = std::size_t;

namespace internal {
/// Hash 函数，用于计算类型的哈希值
constexpr std::size_t fnv1a_64(std::string_view str) {
    constexpr std::size_t FNV_OFFSET_BASIS = 14695981039346656037ull;
    constexpr std::size_t FNV_PRIME = 1099511628211ull;

    std::size_t hash = FNV_OFFSET_BASIS;
    for (const char c : str) {
        hash ^= static_cast<std::size_t>(c);
        hash *= FNV_PRIME;
    }
    return hash;
}
} // namespace internal

/// 用模板函数的签名进行哈希，从而在编译期获得 typeid
template <typename T>
constexpr std::size_t GetTypeId() noexcept {
#if defined(__clang__)
    constexpr std::string_view sig = __PRETTY_FUNCTION__;
#elif defined(__GNUC__)
    constexpr std::string_view sig = __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
    constexpr std::string_view sig = __FUNCSIG__;
#endif

    return internal::fnv1a_64(sig);
}
} // namespace ecs

#endif // TYPE_HPP
