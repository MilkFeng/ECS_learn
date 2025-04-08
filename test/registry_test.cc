#include "ecs/ecs.hpp"

#include <gtest/gtest.h>

struct MyComponent {
    std::uint32_t value;
};

struct MyComponent2 {
    std::uint64_t value;
};

TEST(RegistryTest, RegistryTest1) {
    ecs::Registry<std::uint32_t> reg;
    const auto entity = reg.CreateEntity();

    reg.AttachComponent<MyComponent>(entity, {32});
    auto& storage = reg.GetStorageOfComponent<MyComponent>();

    ASSERT_EQ(storage.ComponentOf(entity).value, 32);
    ASSERT_EQ(storage.Size(), 1);

    reg.AttachComponents<MyComponent, MyComponent2>(entity, {32}, {54});
    auto& storage2 = reg.GetStorageOfComponent<MyComponent2>();

    ASSERT_EQ(storage2.ComponentOf(entity).value, 54);
    ASSERT_EQ(storage2.Size(), 1);

    const auto entity2 = reg.CreateEntity();

    ASSERT_NE(entity, entity2);

    reg.AttachComponent<MyComponent>(entity2, {64});

    ASSERT_EQ(storage.ComponentOf(entity2).value, 64);
    ASSERT_EQ(storage.ComponentOf(entity).value, 32);
    ASSERT_EQ(storage.Size(), 2);
}


TEST(RegistryTest, RegistryTestDuplicate) {
    ecs::Registry<std::uint32_t> reg;
    const auto entity = reg.CreateEntity();

    // 应该编译失败
    // reg.AttachComponents<MyComponent, MyComponent>(entity, {32}, {42});

    const std::size_t my_component_type_id = ecs::GetTypeId<MyComponent>();
    const std::size_t my_component2_type_id = ecs::GetTypeId<MyComponent2>();
    reg.DetachComponents(entity, my_component_type_id, my_component2_type_id);

    ASSERT_THROW(reg.DetachComponents(entity, my_component_type_id, my_component_type_id), std::runtime_error);
}