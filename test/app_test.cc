
#include <gtest/gtest.h>

#include "ecs/ecs.hpp"

using namespace std;
using namespace ecs;


struct MyComponent {
    std::uint32_t value;
};

struct MyComponent2 {
    std::uint64_t value;
};


void StartupSystem(EcsViewer& viewer, EcsCommands& commands) {
    commands.Spawn<MyComponent>(MyComponent{32})
            .Spawn<MyComponent2>(MyComponent2{64});
}

void System1(EcsViewer& viewer, EcsCommands& commands) {
    auto view = viewer.View<std::tuple<MyComponent, MyComponent2>>();
    std::cout << "System1" << std::endl;
    while (auto res = view.Next()) {
        std::cout << "System1: loop" << std::endl;
        if (res) {
            const auto [required, optional] = *res;
            const auto [component1, component2] = required;
            // Do something with component1 and component2
        }
    }
    std::cout << "System1: end" << std::endl;
}

void task(int x, int y) {

}

const int& get_x() {
    static int x;
    return x;
}


TEST(AppTest, AppTest1) {
    EcsApplication app;

    auto now = std::chrono::system_clock::now();

    app.AddStartupSystem(StartupSystem)
        .AddUpdateSystem(System1)
        .Run([now]() {
            auto new_now = std::chrono::system_clock::now();
            auto should_exit = new_now - now > std::chrono::seconds(1);
            std::cout << "should_exit: " << should_exit << std::endl;
            return should_exit;
        });
}