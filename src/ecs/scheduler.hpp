#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <condition_variable>
#include <future>
#include <queue>
#include <thread>

#include "system.hpp"

namespace ecs {
namespace internal {
class ThreadPool {
public:
    using TaskType = std::function<void()>;

    explicit ThreadPool(const std::size_t num_threads) : num_threads_(num_threads) {
        InitializeWorkers();
    }

    ~ThreadPool() {
        Stop();
    }

    /// 任务入队，返回一个 future 对象，可以获取任务的返回值
    template <typename InvokeType, typename... Args>
        requires std::invocable<InvokeType&&, Args&&...>
    auto Enqueue(InvokeType&& f, Args&&... args) {
        using ReturnType = std::invoke_result_t<InvokeType&&, Args&&...>;

        // 将任务包装成一个 packaged_task
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<InvokeType>(f), std::forward<Args>(args)...)
        );

        std::future<ReturnType> result = task->get_future();

        {
            // 与 WorkerThread 中的条件变量配合使用
            std::lock_guard lock(queue_mutex_);

            // 如果线程池停止了，就不能再添加任务了
            if (stop_) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }

            tasks_.emplace([task]() { (*task)(); });
        }
        condition_.notify_one();

        return result;
    }

    [[nodiscard]] bool IsStopped() const noexcept {
        return stop_;
    }

    void Restart() noexcept {
        if (!stop_) {
            Stop();
        }

        // 这里所有线程都停止了，所以可以直接重置
        stop_ = false;
        InitializeWorkers();
    }

    void Stop() noexcept {
        if (stop_) return;

        // 通知所有线程停止
        {
            std::unique_lock lock(queue_mutex_);
            stop_ = true;
        }

        // 通知所有线程
        condition_.notify_all();

        // 等待所有线程结束
        for (auto& worker : workers_) {
            worker.join();
        }

        workers_.clear();
    }

private:
    void InitializeWorkers() {
        workers_.clear();
        for (std::size_t i = 0; i < num_threads_; ++i) {
            workers_.emplace_back(WorkerThread, this);
        }
    }

    static void WorkerThread(ThreadPool* pool) {
        while (true) {
            std::function<void()> task;

            // 临界区
            {
                // 先获取锁
                std::unique_lock<std::mutex> lock(pool->queue_mutex_);

                // 等待条件变量，这里就是等待任务队列不为空
                // 或者如果线程池停止了，这里应该要继续执行
                pool->condition_.wait(lock, [pool] {
                    return pool->stop_ || !pool->tasks_.empty();
                });

                // 如果线程池停止了，且任务队列为空，就退出
                if (pool->stop_ && pool->tasks_.empty()) {
                    return;
                }

                // 从任务队列中取出任务并 pop
                task = std::move(pool->tasks_.front());
                pool->tasks_.pop();
            }
            // 临界区结束

            task();
        }
    }

private:
    std::vector<std::thread> workers_;
    std::queue<TaskType> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;

    bool stop_{false};
    std::size_t num_threads_;

    friend void WorkerThread(ThreadPool* pool);
};
} // namespace internal


template <typename... SystemArgs>
class Scheduler {
public:
    using SystemGraphType = SystemGraph<SystemArgs...>;
    using SystemType = typename SystemGraphType::SystemType;
    using SystemIdType = typename SystemGraphType::SystemIdType;

    Scheduler() noexcept : pool_(std::thread::hardware_concurrency()) {
    }

    explicit Scheduler(const std::size_t num_threads) : pool_(num_threads) {
    }

    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    Scheduler(Scheduler&&) noexcept = default;
    Scheduler& operator=(Scheduler&&) noexcept = default;

    ~Scheduler() = default;

    [[nodiscard]] constexpr std::size_t Size() const {
        std::lock_guard lock(graph_mutex_);
        return graph_.Size();
    }

    constexpr SystemIdType AddSystem(const SystemType& system) {
        std::lock_guard lock(graph_mutex_);
        return graph_.AddSystem(system);
    }

    constexpr void RemoveSystem(const SystemIdType id) {
        std::lock_guard lock(graph_mutex_);
        graph_.RemoveSystem(id);
    }

    constexpr void AddConstraint(const SystemIdType from_id, const SystemIdType to_id) {
        std::lock_guard lock(graph_mutex_);
        graph_.AddConstraint(from_id, to_id);
    }

    constexpr void RemoveConstraint(const SystemIdType from_id, const SystemIdType to_id) {
        std::lock_guard lock(graph_mutex_);
        graph_.RemoveConstraint(from_id, to_id);
    }

    constexpr bool ContainsConstraint(const SystemIdType from_id, const SystemIdType to_id) const {
        std::lock_guard lock(graph_mutex_);
        return graph_.ContainsConstraint(from_id, to_id);
    }

    constexpr bool ContainsSystem(const SystemIdType id) const {
        std::lock_guard lock(graph_mutex_);
        return graph_.ContainsSystem(id);
    }

    /// 检查是否有循环依赖，如果有的话返回 true
    [[nodiscard]] constexpr bool CheckCycle() const {
        std::lock_guard lock(graph_mutex_);
        return graph_.CheckCycle();
    }

    constexpr void Execute(SystemArgs&... args) {
        // 拷贝一份图，用于拓扑排序
        SystemGraphType graph_copy;

        {
            std::lock_guard graph_lock(graph_mutex_);

            if (graph_.CheckCycle()) {
                throw std::runtime_error("Cycle detected in SystemGraph");
            }

            graph_copy = graph_;
        }

        if (graph_copy.Empty()) {
            return;
        }

        // 先初始化线程池
        pool_.Restart();

        // 将没有依赖的 System 入队
        for (const auto& node : graph_copy.nodes()) {
            if (node.InDegree() == 0) {
                pool_.Enqueue(RunSystem, this, node.system, node.id, std::ref(args)...);
            }
        }



        // 等待所有 System 执行完毕
        while (true) {
            std::unique_lock lock(successes_mutex_);
            successes_condition_.wait(lock, [this] {
                return !successes_.empty();
            });

            // 这里不用担心线程安全问题，因为只有一个线程会修改这个图
            while (!successes_.empty()) {
                const auto id = successes_.front();
                const auto& node = graph_copy.FindSystem(id);
                successes_.pop();

                // 将没有依赖的 System 入队
                for (const auto tos = node.tos; const auto& next_id : tos) {
                    auto& next_node = graph_copy.FindSystem(next_id);
                    graph_copy.RemoveConstraint(id, next_id);
                    if (next_node.InDegree() == 0) {
                        pool_.Enqueue(RunSystem, this, next_node.system, next_node.id, std::ref(args)...);
                    }
                }

                // 从图中删除这个 System
                graph_copy.RemoveSystem(id);
            }

            if (graph_copy.Empty()) {
                break;
            }
        }

        // 等待所有线程结束
        pool_.Stop();
    }

private:
    static void RunSystem(Scheduler* scheduler, const SystemType system, const SystemIdType id, SystemArgs&... args) {
        // 先执行 System
        system(args...);

        // 将 id 放入成功队列
        {
            std::lock_guard lock(scheduler->successes_mutex_);
            scheduler->successes_.push(id);
        }

        // 通知主线程
        scheduler->successes_condition_.notify_one();
    }

private:
    SystemGraphType graph_;
    mutable std::mutex graph_mutex_;

    internal::ThreadPool pool_;

    std::queue<SystemIdType> successes_;
    std::condition_variable successes_condition_;
    mutable std::mutex successes_mutex_;
};
} // namespace ecs

#endif // SCHEDULER_HPP
