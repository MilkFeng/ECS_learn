#include "ecs/ecs.hpp"

#include <gtest/gtest.h>


enum class MyEntity : std::uint32_t {
};


struct MyComponent {
    std::uint32_t value;

    friend bool operator==(const MyComponent& lhs, const MyComponent& rhs) {
        return lhs.value == rhs.value;
    }
};

struct MyComponent2 {
    std::uint64_t value;

    friend bool operator==(const MyComponent2& lhs, const MyComponent2& rhs) {
        return lhs.value == rhs.value;
    }
};

TEST(ViewerTest, ViewerTest1) {
    ecs::World<MyEntity> world;

    auto& reg = world.registry();
    const auto entity1 = reg.CreateEntity();
    const auto entity2 = reg.CreateEntity();
    const auto entity3 = reg.CreateEntity();

    reg.AttachComponent<MyComponent>(entity1, {32});
    reg.AttachComponent<MyComponent2>(entity1, {64});
    reg.AttachComponent<MyComponent>(entity2, {128});
    reg.AttachComponent<MyComponent2>(entity3, {256});

    auto& viewer = world.viewer();

    std::vector<std::tuple<MyComponent, MyComponent2>> results;
    auto view = viewer.View<std::tuple<MyComponent, MyComponent2>>();
    while (auto res = view.Next()) {
        if (res) {
            const auto [required, optional] = *res;
            const auto& [component1, component2] = required;
            results.emplace_back(component1, component2);
        }
    }

    ASSERT_EQ(results.size(), 1);
    ASSERT_EQ(results[0], std::make_tuple(MyComponent{32}, MyComponent2{64}));

    std::vector<std::tuple<MyComponent, std::optional<MyComponent2>>> results2;
    auto view2 = viewer.View<std::tuple<MyComponent>, std::tuple<MyComponent2>>();
    while (auto res = view2.Next()) {
        if (res) {
            const auto [required, optional] = *res;
            const auto& [component1] = required;
            const auto [component2] = optional;
            if (component2 == nullptr) {
                results2.emplace_back(component1, std::nullopt);
            } else {
                results2.emplace_back(component1, *component2);
            }
        }
    }

    ASSERT_EQ(results2.size(), 2);
    ASSERT_EQ(results2[0], std::make_tuple(MyComponent{32}, std::optional<MyComponent2>{MyComponent2{64}}));
    ASSERT_EQ(results2[1], std::make_tuple(MyComponent{128}, std::optional<MyComponent2>{std::nullopt}));
}
