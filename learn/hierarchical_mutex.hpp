#ifndef HIERARCHICAL_MUTEX_HPP
#define HIERARCHICAL_MUTEX_HPP

#include <cstddef>
#include <mutex>
#include <stdexcept>

class hierarchical_mutex {
public:
    explicit hierarchical_mutex(const std::size_t hierarchical) :
        hierarchy_value_(hierarchical),
        previous_hierarchy_value_(0) {
    }

    void lock() {
        // 检查是否违反层级规则
        // 这里由于只访问 thread_local 变量（hierarchy_value_ 不会变），所以不需要加锁
        check_for_hierarchy_violation();

        // 加锁
        internal_mutex_.lock();

        // 更新层级信息，这里需要加锁
        update_hierarchy_value();
    }

    void unlock() {
        // 只能依次从最低层级往高层级解锁
        if (this_thread_hierarchy_value_ != hierarchy_value_) {
            throw std::logic_error("mutex hierarchy violated");
        }

        // 复原层级信息，其实就是把 head 指向 next
        this_thread_hierarchy_value_ = previous_hierarchy_value_;

        // 解锁
        internal_mutex_.unlock();
    }

    bool try_lock() {
        check_for_hierarchy_violation();
        if (!internal_mutex_.try_lock()) return false;
        update_hierarchy_value();
        return true;
    }

private:
    void check_for_hierarchy_violation() const {
        if (this_thread_hierarchy_value_ <= hierarchy_value_) {
            // 不能往高层加锁，只能逐渐往低层加锁
            throw std::logic_error("mutex hierarchy violated");
        }
    }

    void update_hierarchy_value() {
        // 保存之前的最低层级到 previous_hierarchy_value_
        // 由于一个锁只能由一个线程加锁，所以我们可以一直根据 previous_hierarchy_value_ 找到当前线程所有的锁
        // this_thread_hierarchy_value_ 就相当于 head，previous_hierarchy_value_ 就相当于 next
        previous_hierarchy_value_ = this_thread_hierarchy_value_;

        // 更新当前线程所拥有的最低层级
        this_thread_hierarchy_value_ = hierarchy_value_;
    }

private:
    std::mutex internal_mutex_;

    // 该层级锁的层级值
    const std::size_t hierarchy_value_;

    // 保存之前的该层级锁的上一个加锁线程的 this_thread_hierarchy_value_
    std::size_t previous_hierarchy_value_;

    // 注意这里是 thread_local 的，每个线程都记录当前线程所加锁的最低层级
    static thread_local std::size_t this_thread_hierarchy_value_;
};
#endif // HIERARCHICAL_MUTEX_HPP
