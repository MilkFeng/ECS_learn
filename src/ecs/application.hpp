#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "world.hpp"
#include "scheduler.hpp"

namespace ecs {
template <AllowedEntityType Entity>
struct SystemArgPack {
    using ViewerType = Viewer<Entity>;
    using CommandsType = Commands<Entity>;
    using ResourcesType = Resources<Entity>;

    ViewerType& viewer;
    CommandsType& commands;
    ResourcesType& resources;
};



/// Application 类
/// 用于管理整个应用程序的生命周期
template <AllowedEntityType Entity>
class Application {
public:
    using WorldType = World<Entity>;
    using ViewerType = Viewer<Entity>;
    using CommandsType = Commands<Entity>;
    using ResourcesType = Resources<Entity>;
    using SystemArgPackType = SystemArgPack<Entity>;

    using SystemType = std::function<void(SystemArgPackType)>;
    using SchedulerType = Scheduler<SystemArgPackType>;
    using SchedulerStageIdType = typename SchedulerType::StageIdType;

    Application() noexcept {
        // 每个调度器默认有一个阶段
        startup_scheduler_.AddStageToFront();
        update_scheduler_.AddStageToFront();
        shutdown_scheduler_.AddStageToFront();
    }

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    Application(Application&&) noexcept = default;
    Application& operator=(Application&&) noexcept = default;

    constexpr void Run(const std::function<bool()>& should_exit = []() { return false; }) {
        auto pack = SystemArgPackType{
            .viewer = viewer(),
            .commands = commands(),
            .resources = resources()
        };

        // 先执行 startup
        startup_scheduler_.Execute(pack);

        // 执行命令队列
        world_.commands().Execute();

        // 然后每一帧都执行 update
        while (!should_exit()) {
            // 执行调度器
            update_scheduler_.Execute(pack);

            // 执行命令队列
            world_.commands().Execute();
        }

        // 最后执行 shutdown
        shutdown_scheduler_.Execute(pack);
    }

    constexpr CommandsType& commands() {
        return world_.commands();
    }

    constexpr ViewerType& viewer() {
        return world_.viewer();
    }

    constexpr ResourcesType& resources() {
        return world_.resources();
    }

    SchedulerType& startup_scheduler() {
        return startup_scheduler_;
    }

    SchedulerType& update_scheduler() {
        return update_scheduler_;
    }

    SchedulerType& shutdown_scheduler() {
        return shutdown_scheduler_;
    }

private:
    WorldType world_{};

    SchedulerType startup_scheduler_{};
    SchedulerType update_scheduler_{};
    SchedulerType shutdown_scheduler_{};
};


using EcsEntity = EntityU32Enum;
using EcsApplication = Application<EcsEntity>;
using EcsWorld = EcsApplication::WorldType;
using EcsViewer = EcsApplication::ViewerType;
using EcsCommands = EcsApplication::CommandsType;
using EcsSystem = EcsApplication::SystemType;
using EcsSystemArgPack = EcsApplication::SystemArgPackType;
} // namespace ecs

#endif // APPLICATION_HPP
