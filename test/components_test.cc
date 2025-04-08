#include "ecs/ecs.hpp"

#include <gtest/gtest.h>

namespace na2 {
struct MyComponent {
    std::uint32_t value;
};
}


template <typename T>
inline void ShowFuncSig() noexcept {
#if defined(__clang__)
    constexpr std::string_view sig = __PRETTY_FUNCTION__;
#elif defined(__GNUC__)
    constexpr std::string_view sig = __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
    constexpr std::string_view sig = __FUNCSIG__;
#endif

    std::cout << sig << std::endl;
}

template <ecs::AllowedComponentType Component>
inline void TestComponent() noexcept {
}

struct Component1 {
    std::uint32_t value;
};

struct Component2 {
    std::uint64_t value;
};

struct Component3 {
    std::uint8_t value;
};

TEST(ComponentTest, ComponentTest1) {
    constexpr auto res = ecs::CheckDuplicateComponentsTuple<
        std::tuple<Component1, Component2>, std::tuple<Component1>>();
    ASSERT_TRUE(res);

    constexpr auto res2 = ecs::CheckDuplicateComponentsTuple<
        std::tuple<Component1, Component2>, std::tuple<Component3>>();
    ASSERT_FALSE(res2);

    ASSERT_EQ(ecs::GetTypeId<std::uint32_t>(), ecs::GetTypeId<std::uint32_t>());
    ASSERT_NE(ecs::GetTypeId<std::uint32_t>(), ecs::GetTypeId<std::uint16_t>());

    struct MyComponent {
        std::uint32_t value;
    };

    ASSERT_NE(ecs::GetTypeId<MyComponent>(), ecs::GetTypeId<std::uint32_t>());
    ASSERT_NE(ecs::GetTypeId<MyComponent>(), ecs::GetTypeId<na2::MyComponent>());


    // ShowFuncSig<MyComponent>();
    // ShowFuncSig<na2::MyComponent>();

    // 下面的代码会编译失败
    // TestComponent<MyComponent&>();


}
