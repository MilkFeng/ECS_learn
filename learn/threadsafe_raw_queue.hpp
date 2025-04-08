#ifndef THREADSAFE_RAW_QUEUE_HPP
#define THREADSAFE_RAW_QUEUE_HPP

#include <condition_variable>
#include <memory>
#include <mutex>

template <typename T>
class threadsafe_raw_queue {
private:
    struct node {
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
    };

public:
    // 这里创建了一个虚位节点作为头结点，并且 tail_ 指向这个虚位节点
    // 这样可以避免空队列的特殊处理，并且可以支持更细粒度的锁
    //
    // 如果不使用虚位节点，那么在 push 的时候，如果队列为空那么头尾指针都应该修改成新节点
    // 但是如果使用虚位节点，那么直接修改尾指针就可以了
    // 这样做到了数据的分离
    threadsafe_raw_queue() :
        head_(std::make_unique<node>()),
        tail_(head_.get()) {
    }

    void push(T new_value) {
        // 这里创建了一个新的虚位节点
        std::shared_ptr<T> new_data = std::make_shared<T>(std::move(new_value));
        std::unique_ptr<node> p = std::make_unique<node>();
        const node* new_tail = p.get();

        {
            std::lock_guard lock(tail_m_);
            tail_->data = new_data;
            tail_->next = std::move(p);
            tail_ = new_tail;
        }

        data_cond_.notify_one();
    }

    std::shared_ptr<T> wait_and_pop() {
        const std::unique_ptr<node> old_head = wait_pop_head();
        return old_head->data;
    }

    void wait_and_pop(T& value) {
        const std::unique_ptr<node> old_head = wait_pop_head(value);
    }

    std::shared_ptr<T> try_pop() {
        const std::unique_ptr<node> old_head = try_pop_head();
        return old_head ? old_head->data : nullptr;
    }

    bool try_pop(T& value) {
        const std::unique_ptr<node> old_head = try_pop_head(value);
        return old_head != nullptr;
    }

    [[nodiscard]] bool empty() const {
        std::lock_guard lock(head_m_);
        return head_.get() == get_tail();
    }

private:
    node* get_tail() {
        std::lock_guard lock(tail_m_);
        return tail_;
    }

    std::unique_ptr<node> pop_head_no_threadsafe() {
        std::unique_ptr<node> old_head = std::move(head_);
        head_ = std::move(old_head->next);
        return old_head;
    }

    std::unique_lock<std::mutex> wait_for_data() {
        // 等待队列不为空
        std::unique_lock lock(head_m_);
        data_cond_.wait(lock, [this] {
            // 其实和下面的做法是等价的：
            // 先对 head_ 和 tail_ 加锁，
            // 如果 head_ 和 tail_ 指向同一个节点，那么就说明队列为空，释放这两个锁并等待
            // 否则说明队列不为空，继续执行
            return head_.get() != get_tail();
        });
        return lock;
    }

    std::unique_ptr<node> wait_pop_head() {
        // 最大的问题是：如果队列为空，那么 head_ 和 tail_ 会指向同一个节点，可能会出现问题
        // 但是这里巧妙地规避了这个问题

        // 当队列为空时，这个函数是可能访问 dummy 节点的函数，所以：
        // - 它与修改 head_ 的函数是互斥的
        // - 它与修改 tail_ 的函数是互斥的
        // - 它与修改 head_ 和 tail_ 的函数是互斥的
        // 所以函数内部对 head_ 和 tail_ 都加了锁
        std::unique_lock lock = wait_for_data();

        // 这里是没有问题的，因为这里保证了队列不为空，此时 head_ 和 tail_ 不会指向同一个节点
        return pop_head_no_threadsafe();
    }

    std::unique_ptr<node> wait_pop_head(T& value) {
        std::unique_lock lock = wait_for_data();

        // 这里先拿到数据，然后再 pop，pop 得到的是一个空节点
        value = std::move(*head_->data);
        return pop_head_no_threadsafe();
    }

    std::unique_ptr<node> try_pop_head() {
        std::lock_guard lock(head_m_);

        // 这里 get_tail 会对 tail_m_ 加锁
        // 先对 head_ 加锁，然后再对 tail_ 加锁
        // 没有反着加锁的方法，所以不会发生死锁
        if (head_.get() == get_tail()) return nullptr;
        return pop_head_no_threadsafe();
    }

    std::unique_ptr<node> try_pop_head(T& value) {
        std::lock_guard lock(head_m_);

        // 这里和上面的 wait_for_data 一样，也是可能访问 dummy 节点，我们需要对 head_ 和 tail_ 都加锁
        if (head_.get() == get_tail()) return nullptr;

        value = std::move(*head_->data);
        return pop_head_no_threadsafe();
    }

private:
    // 更细粒度的锁，可以减少锁的竞争
    std::mutex head_m_;
    std::mutex tail_m_;

    std::unique_ptr<node> head_;

    // 注意 tail_ 不能是智能指针
    node* tail_;

    std::condition_variable data_cond_;
};

#endif // THREADSAFE_RAW_QUEUE_HPP
