#ifndef THREADSAFE_QUEUE_HPP
#define THREADSAFE_QUEUE_HPP

#include <condition_variable>
#include <queue>


template <typename T>
class threadsafe_queue {
public:
    threadsafe_queue() = default;

    void wait_and_pop(T& value) {
        std::unique_lock lock(m_);

        // 等待队列不为空
        cv_.wait(lock, [this] { return !queue_.empty(); });
        value = std::move(*queue_.front());
        queue_.pop();
    }

    bool try_pop(T& value) {
        std::lock_guard lock(m_);
        if (queue_.empty()) return false;
        value = std::move(*queue_.front());
        queue_.pop();
        return true;
    }

    std::shared_ptr<T> wait_and_pop() {
        std::unique_lock lock(m_);
        cv_.wait(lock, [this] { return !queue_.empty(); });
        std::shared_ptr<T> res = queue_.front();
        queue_.pop();
        return res;
    }

    void push(T new_value) {
        // 由于 shared_ptr 的构造函数并不需要跨线程，所以可以在加锁之前构造 shared_ptr
        // 内存分配是非常耗时的，所以尽量减少加锁期间的内存分配
        std::shared_ptr<T> data = std::make_shared<T>(std::move(new_value));
        std::lock_guard lock(m_);
        queue_.push(data);
        cv_.notify_one();
    }

    bool empty() const {
        std::lock_guard lock(m_);
        return queue_.empty();
    }

private:
    // 使用 std::shared_ptr<T> 作为队列元素，可以避免拷贝构造函数
    std::queue<std::shared_ptr<T>> queue_;
    mutable std::condition_variable cv_;
    mutable std::mutex m_;
};

#endif // THREADSAFE_QUEUE_HPP
