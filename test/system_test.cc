#include "ecs/ecs.hpp"

#include <gtest/gtest.h>

using namespace ecs;

using SystemGraphType = SystemGraph<>;
using SystemType = typename SystemGraphType::SystemType;

struct SimpleSystem {
    int value;
    void operator()() const {
        std::cout << "SimpleSystem " << value << std::endl;
    }
};

TEST(SystemTest, SystemTest1) {
    SystemType system1 = SimpleSystem{1};
    SystemType system2 = SimpleSystem{2};


    SystemGraphType graph;
    const auto id1 = graph.AddSystem(system1);
    const auto id2 = graph.AddSystem(system2);

    graph.AddConstraint(id1, id2);

    ASSERT_EQ(graph.Size(), 2);
    ASSERT_TRUE(graph.ContainsConstraint(id1, id2));
    ASSERT_FALSE(graph.ContainsConstraint(id2, id1));

    ASSERT_EQ(graph.FindSystem(id1).InDegree(), 0);
    ASSERT_EQ(graph.FindSystem(id2).InDegree(), 1);

    ASSERT_EQ(graph.FindSystem(id1).OutDegree(), 1);
    ASSERT_EQ(graph.FindSystem(id2).OutDegree(), 0);
}


TEST(SystemTest, SystemTestCycle) {
    const std::array<SystemType, 5> systems = {
        []() {
            std::cout << "System1" << std::endl;
        },
        []() {
            std::cout << "System2" << std::endl;
        },
        []() {
            std::cout << "System3" << std::endl;
        },
        []() {
            std::cout << "System4" << std::endl;
        },
        []() {
            std::cout << "System5" << std::endl;
        }
    };

    std::array<std::pair<SystemGraphType::SystemIdType, SystemGraphType::SystemIdType>, 5> constraints = {
        {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 0}}
    };

    SystemGraphType graph;
    for (const auto& system : systems) {
        graph.AddSystem(system);
    }

    for (const auto& [from, to] : constraints) {
        graph.AddConstraint(from, to);
    }

    ASSERT_EQ(graph.Size(), 5);
    ASSERT_TRUE(graph.ContainsConstraint(0, 1));
    ASSERT_TRUE(graph.ContainsConstraint(1, 2));
    ASSERT_TRUE(graph.ContainsConstraint(2, 3));
    ASSERT_TRUE(graph.ContainsConstraint(3, 4));
    ASSERT_TRUE(graph.ContainsConstraint(4, 0));

    ASSERT_TRUE(graph.CheckCycle());


    graph.RemoveConstraint(4, 0);

    ASSERT_EQ(graph.Size(), 5);
    ASSERT_TRUE(graph.ContainsConstraint(0, 1));
    ASSERT_TRUE(graph.ContainsConstraint(1, 2));
    ASSERT_TRUE(graph.ContainsConstraint(2, 3));
    ASSERT_TRUE(graph.ContainsConstraint(3, 4));
    ASSERT_FALSE(graph.ContainsConstraint(4, 0));

    ASSERT_FALSE(graph.CheckCycle());

}


TEST(SystemTest, SystemTest2) {
    SystemGraphType graph;
    std::vector<SimpleSystem> systems;
    for (int i = 0; i < 8; ++i) {
        systems.push_back(SimpleSystem{i});
        graph.AddSystem(systems.back());
    }

    ASSERT_THROW(graph.AddConstraint(0, 0), std::runtime_error);
    ASSERT_THROW(graph.AddConstraint(1, 1), std::runtime_error);

}