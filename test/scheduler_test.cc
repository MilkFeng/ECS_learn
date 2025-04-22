#include "ecs/ecs.hpp"

#include <gtest/gtest.h>

using namespace ecs;

using SchedulerType = StageScheduler<>;
using SystemGraphType = typename SchedulerType::SystemGraphType;
using SystemType = typename SystemGraphType::SystemType;

TEST(SchedulerTest, SchedulerTest1) {
    SchedulerType scheduler(4);
    std::vector<int> results;
    std::mutex mutex;

    SystemType system0 = [&]() {
        std::lock_guard lock(mutex);
        results.push_back(0);
    };

    SystemType system1 = [&]() {
        std::lock_guard lock(mutex);
        results.push_back(1);
    };

    SystemType system2 = [&]() {
        std::lock_guard lock(mutex);
        results.push_back(2);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    };

    SystemType system3 = [&]() {
        std::lock_guard lock(mutex);
        results.push_back(3);
    };

    SystemType system4 = [&]() {
        std::lock_guard lock(mutex);
        results.push_back(4);
    };

    SystemType system5 = [&]() {
        std::lock_guard lock(mutex);
        results.push_back(5);
    };

    SystemType system6 = [&]() {
        std::lock_guard lock(mutex);
        results.push_back(6);
    };


    scheduler.AddSystem(system0);
    scheduler.AddSystem(system1);
    scheduler.AddSystem(system2);
    scheduler.AddSystem(system3);
    scheduler.AddSystem(system4);
    scheduler.AddSystem(system5);
    scheduler.AddSystem(system6);

    scheduler.AddConstraint(0, 1);
    scheduler.AddConstraint(0, 2);
    scheduler.AddConstraint(1, 3);
    scheduler.AddConstraint(2, 3);
    scheduler.AddConstraint(3, 4);
    scheduler.AddConstraint(3, 5);
    scheduler.AddConstraint(4, 6);
    scheduler.AddConstraint(5, 6);

    //           5 --------|
    //           ^         |
    //           |         v
    // 0 -> 1 -> 3 -> 4 -> 6
    // |         ^
    // v         |
    // 2 --------|


    ASSERT_EQ(scheduler.Size(), 7);

    ASSERT_TRUE(scheduler.ContainsConstraint(0, 1));
    ASSERT_TRUE(scheduler.ContainsConstraint(0, 2));
    ASSERT_TRUE(scheduler.ContainsConstraint(1, 3));
    ASSERT_TRUE(scheduler.ContainsConstraint(2, 3));
    ASSERT_TRUE(scheduler.ContainsConstraint(3, 4));
    ASSERT_TRUE(scheduler.ContainsConstraint(3, 5));
    ASSERT_TRUE(scheduler.ContainsConstraint(4, 6));
    ASSERT_TRUE(scheduler.ContainsConstraint(5, 6));

    ASSERT_FALSE(scheduler.CheckCycle());

    scheduler.Execute();

    ASSERT_EQ(results.size(), 7);

    ASSERT_EQ(results[0], 0);

    ASSERT_TRUE(results[1] == 1 || results[1] == 2);
    ASSERT_TRUE(results[2] == 1 || results[2] == 2);
    ASSERT_TRUE(results[1] != results[2]);

    ASSERT_EQ(results[3], 3);

    ASSERT_TRUE(results[4] == 4 || results[4] == 5);
    ASSERT_TRUE(results[5] == 4 || results[5] == 5);
    ASSERT_TRUE(results[4] != results[5]);

    ASSERT_EQ(results[6], 6);
}
