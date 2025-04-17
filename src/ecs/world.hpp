#ifndef WORLD_HPP
#define WORLD_HPP

#include "registry.hpp"

namespace ecs {
template <AllowedEntityType Entity>
class Commands;

template <AllowedEntityType Entity>
class Viewer;

/// World 类
template <AllowedEntityType Entity>
class World {
public:
    World() noexcept = default;

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    World(World&&) noexcept = default;
    World& operator=(World&&) noexcept = default;

    ~World() = default;

    Registry<Entity>& registry() noexcept {
        return registry_;
    }

    Commands<Entity>& commands() noexcept {
        return commands_;
    }

    Viewer<Entity>& viewer() noexcept {
        return viewer_;
    }

private:
    friend class Commands<Entity>;
    friend class Viewer<Entity>;

private:
    // 注意整个 registry 都不是线程安全的，需要由调度器保证无冲突访问
    Registry<Entity> registry_{};

    // 由于每个 System 都有可能访问 commands，所以它必须是线程安全的
    Commands<Entity> commands_{*this};

    // viewer 是用来遍历实体和组件的，应该让用户不要直接使用 registry
    Viewer<Entity> viewer_{*this};
};
} // namespace ecs

#endif // WORLD_HPP
