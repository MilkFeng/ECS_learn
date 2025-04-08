#include "ecs/ecs.hpp"

#include <gtest/gtest.h>

TEST(EntityTest, EntityTest) {
    using namespace ecs;
    using MyEntity = std::uint32_t;
    using MyEntityTraits = internal::EntityTraits<MyEntity>;

    MyEntity entity = 0x12345;
    MyEntity version = 0x678;
    MyEntity combined = CombineEntity<MyEntity>(entity, version);

    MyEntity _entity = GetId<MyEntity>(combined);
    MyEntity _version = GetVersion<MyEntity>(combined);

    MyEntity next_version_entity = GenNextVersion<MyEntity>(combined);
    MyEntity next_version = GetVersion<MyEntity>(next_version_entity);

    MyEntity null_entity = NullEntity<MyEntity>();

    ASSERT_EQ(entity, _entity);
    ASSERT_EQ(version, _version);

    ASSERT_EQ(version + 1, next_version);
    ASSERT_EQ(std::numeric_limits<MyEntity>::max(), null_entity);

    enum class MyEntityEnum : std::uint32_t {
    };

    using MyEntityEnumTraits = internal::EntityTraits<MyEntityEnum>;

    auto combined2 = CombineEntity<MyEntityEnum>(entity, version);
    auto _entity2 = GetId<MyEntityEnum>(combined2);
    auto _version2 = GetVersion<MyEntityEnum>(combined2);

    ASSERT_EQ(entity, _entity2);
    ASSERT_EQ(version, _version2);
}

TEST(EntityTest, EntityConceptTest) {
    enum class ErrorEntityEnum : std::uint8_t {
    };
    enum class ErrorEntityEnum2 : std::uint16_t {
    };

    ASSERT_FALSE(ecs::internal::AllowedEntityType<ErrorEntityEnum>);
    ASSERT_FALSE(ecs::internal::AllowedEntityType<ErrorEntityEnum2>);
}