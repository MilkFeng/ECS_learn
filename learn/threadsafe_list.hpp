#ifndef THREADSAFE_LIST_HPP
#define THREADSAFE_LIST_HPP

#include <functional>
#include <memory>
#include <mutex>

template <typename T>
class threadsafe_list {
private:
    struct node {
        std::mutex m;
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;

        node() : next() {
        }

        explicit node(const T& value) : data(std::make_shared<T>(value)) {
        }
    };

public:
    threadsafe_list() = default;
    threadsafe_list(const threadsafe_list&) = delete;
    threadsafe_list& operator=(const threadsafe_list&) = delete;

    void push_front(const T& value) {
        std::unique_ptr<node> new_node = std::make_unique<node>(value);

        // 由于只需要修改 head_，所以只需要对 head_ 加锁
        std::lock_guard lock(head_.m);
        new_node->next = std::move(head_.next);
        head_.next = std::move(new_node);
    }

    void for_each(std::function<void(T&)> f) {
        node* current = &head_;

        // 先对 current（head） 加锁
        std::unique_lock lock(head_.m);
        while (node* const next = current->next.get()) {
            std::unique_lock next_lock(next->m);

            // 由于已经从 current 拿到了 next，所以可以释放 current 的锁
            lock.unlock();

            f(*next->data);
            current = next;

            // 对 next 加锁，这时不持有锁，所以不会发生死锁
            lock = std::move(next_lock);
        }
    }

    std::shared_ptr<T> find_first_if(std::function<bool(const T&)> f) {
        node* current = &head_;
        std::unique_lock lock(head_.m);
        while (node* const next = current->next.get()) {
            std::unique_lock next_lock(next->m);
            lock.unlock();

            if (f(*next->data)) {
                return next->data;
            }

            current = next;
            lock = std::move(next_lock);
        }

        return nullptr;
    }

    void remove_if(std::function<bool(const T&)> f) {
        node* current = &head_;
        std::unique_lock lock(head_.m);
        while (node* const next = current->next.get()) {
            std::unique_lock next_lock(next->m);

            if (f(*next->data)) {
                std::unique_ptr<node> old_next = std::move(current->next);
                current->next = std::move(next->next);
            } else {
                lock.unlock();
                current = next;
                lock = std::move(next_lock);
            }
        }
    }

private:
    node head_;
};

#endif // THREADSAFE_LIST_HPP
