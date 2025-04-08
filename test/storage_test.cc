#include "ecs/ecs.hpp"

#include <gtest/gtest.h>

struct MyComponent {
    std::uint32_t value;
};

TEST(StorageTest, StorageTest1) {
    ecs::Storage<std::uint32_t, MyComponent> storage;

    storage.Upsert(0x13, MyComponent{123});
    storage.Upsert(0x14, MyComponent{456});
    storage.Upsert(0x15, MyComponent{789});

    ASSERT_EQ(storage.ComponentOf(0x13).value, 123);
    ASSERT_EQ(storage.ComponentOf(0x14).value, 456);
    ASSERT_EQ(storage.ComponentOf(0x15).value, 789);

    storage.Pop(0x14);
    ASSERT_FALSE(storage.Contains(0x14));
    ASSERT_EQ(storage.Size(), 2);

    // for (auto [entity, component] : storage) {
    //     std::cout << "Entity: " << entity << ", Component: " << component.value << std::endl;
    // }
    //
    // for (auto it = storage.BasicBegin(); it != storage.BasicEnd(); ++it) {
    //     std::cout << *it << std::endl;
    // }
    //
    // for (const auto entity : storage.ToBasicStorage()) {
    //     std::cout << entity << std::endl;
    // }
}