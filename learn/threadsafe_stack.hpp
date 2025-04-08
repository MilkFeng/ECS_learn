#ifndef THREADSAFE_STACK_HPP
#define THREADSAFE_STACK_HPP

#include <exception>
#include <memory>
#include <mutex>
#include <stack>
#include <vector>

struct empty_stack final : std::exception {
    [[nodiscard]] const char* what() const noexcept override {
        return "empty stack";
    }
};

template <typename T>
class threadsafe_stack {
public:
    threadsafe_stack() = default;

    threadsafe_stack(const threadsafe_stack& other) {
        std::lock_guard lock(other.m_);
        data_ = other.data_;
    }

    threadsafe_stack& operator=(const threadsafe_stack&) = delete;

    // 这里使用值传递，调用者可以使用 std::move 来避免拷贝构造函数
    void push(T new_value) {
        // 加锁可能抛出异常，但是即使抛出异常，也不会有问题，因为并没有修改数据
        std::lock_guard lock(m_);

        // push 操作也有可能抛出异常，例如在拷贝、移动过程中，或者在扩展容器的过程中
        // 但是标准库容器可以处理这种异常，所以不需要额外处理
        data_.push(std::move(new_value));
    }

    std::shared_ptr<T> pop() {
        std::lock_guard lock(m_);
        if (data_.empty()) throw empty_stack();

        // 智能指针的创建也可能抛出异常，如内存不足
        // 标准库可以处理这种异常，所以不需要额外处理

        // 这里是移动了栈顶元素
        // 如果发生异常的话，那么状态的恢复责任在移动构造函数上
        // 所以不需要额外处理
        const std::shared_ptr res = std::make_shared<T>(std::move(data_.top()));

        // 底层使用 std::vector 实现，所以 pop 不会抛出异常
        data_.pop();
        return res;
    }

    void pop(T& value) {
        std::lock_guard lock(m_);
        if (data_.empty()) throw empty_stack();

        // std::move 用于将左值转换为右值引用，这样可以避免拷贝
        value = std::move(data_.top());

        data_.pop();
    }

    bool empty() const {
        std::lock_guard lock(m_);
        return data_.empty();
    }

private:
    std::stack<T> data_;

    // mutable 修饰的成员变量可以在 const 成员函数中修改
    mutable std::mutex m_;
};

#endif // THREADSAFE_STACK_HPP
