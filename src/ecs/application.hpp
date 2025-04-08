#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "world.hpp"
#include "scheduler.hpp"

namespace ecs {
/// Application 类
/// 用于管理整个应用程序的生命周期
template <AllowedEntityType Entity>
class Application {
public:
    using WorldType = World<Entity>;
    using ViewerType = Viewer<Entity>;
    using CommandsType = Commands<Entity>;

    using SystemType = std::function<void(ViewerType&, CommandsType&)>;
    using SchedulerType = Scheduler<ViewerType, CommandsType>;

    Application() noexcept = default;

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    Application(Application&&) noexcept = default;
    Application& operator=(Application&&) noexcept = default;

    constexpr void Run(const std::function<bool()>& should_exit = []() { return false; }) {
        // 先执行 startup
        startup_scheduler_.Execute(viewer(), commands());

        // 然后每一帧都执行 update
        while (!should_exit()) {
            // 执行调度器
            update_scheduler_.Execute(viewer(), commands());

            // 执行命令队列
            world_.commands().Execute();
        }

        // 最后执行 shutdown
        shutdown_scheduler_.Execute(viewer(), commands());
    }

    constexpr CommandsType& commands() {
        return world_.commands();
    }

    constexpr ViewerType& viewer() {
        return world_.viewer();
    }

    constexpr Application& AddStartupSystem(const SystemType& system) {
        startup_scheduler_.AddSystem(system);
        return *this;
    }

    constexpr Application& AddUpdateSystem(const SystemType& system) {
        update_scheduler_.AddSystem(system);
        return *this;
    }

    constexpr Application& AddShutdownSystem(const SystemType& system) {
        shutdown_scheduler_.AddSystem(system);
        return *this;
    }


    constexpr Application& AddStartupConstraint(const std::size_t from_id, const std::size_t to_id) {
        startup_scheduler_.AddConstraint(from_id, to_id);
        return *this;
    }

    constexpr Application& AddUpdateConstraint(const std::size_t from_id, const std::size_t to_id) {
        update_scheduler_.AddConstraint(from_id, to_id);
        return *this;
    }

    constexpr Application& AddShutdownConstraint(const std::size_t from_id, const std::size_t to_id) {
        shutdown_scheduler_.AddConstraint(from_id, to_id);
        return *this;
    }

    constexpr Application& RemoveStartupConstraint(const std::size_t from_id, const std::size_t to_id) {
        startup_scheduler_.RemoveConstraint(from_id, to_id);
        return *this;
    }

    constexpr Application& RemoveUpdateConstraint(const std::size_t from_id, const std::size_t to_id) {
        update_scheduler_.RemoveConstraint(from_id, to_id);
        return *this;
    }

    constexpr Application& RemoveShutdownConstraint(const std::size_t from_id, const std::size_t to_id) {
        shutdown_scheduler_.RemoveConstraint(from_id, to_id);
        return *this;
    }

    [[nodiscard]] constexpr bool ContainsStartupConstraint(const std::size_t from_id,
                                                           const std::size_t to_id) const {
        return startup_scheduler_.ContainsConstraint(from_id, to_id);
    }

    [[nodiscard]] constexpr bool ContainsUpdateConstraint(const std::size_t from_id,
                                                          const std::size_t to_id) const {
        return update_scheduler_.ContainsConstraint(from_id, to_id);
    }

    [[nodiscard]] constexpr bool ContainsShutdownConstraint(const std::size_t from_id,
                                                            const std::size_t to_id) const {
        return shutdown_scheduler_.ContainsConstraint(from_id, to_id);
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
} // namespace ecs

#endif // APPLICATION_HPP
