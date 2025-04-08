#include "ecs/ecs.hpp"

#include <gtest/gtest.h>

struct MyComponent {
    std::uint32_t value;
};

struct MyComponent2 {
    std::uint64_t value;
};

enum class MyEntity : std::uint32_t {
};

TEST(CommandsTest, CommandsTest1) {
    ecs::World<MyEntity> world;

    auto& commands = world.commands();

    commands.Spawn<MyComponent>(MyComponent{32})
            .Spawn<MyComponent2>(MyComponent2{64})
            .Execute();

    auto& reg = world.registry();

    auto& storage1 = reg.GetStorageOfComponent<MyComponent>();
    auto& storage2 = reg.GetStorageOfComponent<MyComponent2>();

    ASSERT_EQ(storage1.Size(), 1);
    ASSERT_EQ(storage2.Size(), 1);

    commands.Destroy(MyEntity{0}).Execute();

    ASSERT_EQ(storage1.Size(), 0);
    ASSERT_EQ(storage2.Size(), 1);

    commands.Destroy(MyEntity{1}).Execute();

    ASSERT_EQ(storage1.Size(), 0);
    ASSERT_EQ(storage2.Size(), 0);
}
