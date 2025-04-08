#include <deque>
#include <functional>
#include <iostream>
#include <vector>

class Base {
public:
    virtual ~Base() = default;

    virtual void run() {
        run2();
    }

    virtual void run2() {
        std::cout << "Base run2()" << std::endl;
    }
};

class Derived final : public Base {
public:
    void run2() override {
        std::cout << "Derived run2()" << std::endl;
    }

    void run() override {
        Base::run();
    }
};


class background_task {
public:
    void operator()() const {
        std::cout << "background_task()" << std::endl;
    }
};

class NotCopyable {
public:
    NotCopyable() = default;
    NotCopyable(const NotCopyable&) = delete;
    NotCopyable& operator=(const NotCopyable&) = delete;
};

void func(NotCopyable& c) {
}

NotCopyable& get_NotCopyable() {
    static NotCopyable c;
    return c;
}

// void f() {
//     auto ff = std::bind(func, std::ref(get_NotCopyable()));
//     ff();
// }

template <typename T, typename U>
struct A {
    T t_;
    U u_;
};

template <typename T>
using AInt = A<T, int>;

template <typename T, template <typename Elem> typename Cont = std::vector>
class Stack {
private:
    Cont<T> elems;

public:
};


template <typename T>
void f1(T param) {

}

template <typename T>
void f2(T& param) {

}

template <typename T>
void f3(T&& param) {
    std::cout << std::is_same_v<T&&, void(&)(int, double)> << std::endl;
}

void func2(int, double) {

}


int main() {
    Derived der;
    der.run();

    Base* b = &der;
    b->run();

    // f();

    Stack<int, AInt> t{};
    Stack<int, std::vector> t2;

    f3(func2);

    return 0;
}
