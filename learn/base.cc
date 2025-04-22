#include <cstdint>
#include <deque>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
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

class AAA;


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

class Test {
public:
    void* operator new(std::size_t) = delete;
};

struct alignas(64) AlignTest {
    uint8_t a; // 1
    __uint128_t b; // 16
};


struct SSS {
    int x;

private:
    int y = 2;
};

int&& rref() {
    static int a{3};
    return static_cast<int&&>(a);
}

template <typename T>
constexpr bool is_smaller_than_pointer_v = sizeof(std::decay_t<T>) < sizeof(std::decay_t<T>*);

template <typename T>
using enable_if_smaller_than_pointer_t = std::enable_if_t<is_smaller_than_pointer_v<T>>;

template <typename T>
using enable_if_not_smaller_than_pointer_t = std::enable_if_t<!is_smaller_than_pointer_v<T>>;

template <typename T, typename = enable_if_smaller_than_pointer_t<T>>
void print(T t) {
    std::cout << "print(T t)" << std::endl;
}

template <typename T, typename = enable_if_not_smaller_than_pointer_t<T>>
void print(const T& t) {
    std::cout << "print(const T& t)" << std::endl;
}

class TEEEE {
public:


private:
    int x;
};


void funcccc(std::vector<int>& v, int z = 0) {
}

int main() {
    // Derived der;
    // der.run();
    //
    // Base* b = &der;
    // b->run();
    //
    // // f();
    //
    // Stack<int, AInt> t{};
    // Stack<int, std::vector> t2;
    //
    // f3(func2);

    TEEEE trl;
    TEEEE ttt = trl;
    TEEEE ttt2 = std::move(trl);


    AlignTest a{};
    std::cout << "sizeof(AlignTest): " << sizeof(AlignTest) << std::endl;
    std::cout << "alignof(AlignTest): " << alignof(AlignTest) << std::endl;
    std::cout << "offsetof(AlignTest, a): " << offsetof(AlignTest, a) << std::endl;
    std::cout << "offsetof(AlignTest, b): " << offsetof(AlignTest, b) << std::endl;

    auto* pp = &rref;
    int x = 42; // x 是 int，值是 42

    int&& y = std::move(x); // 亡值

    std::shared_ptr<int> ptr(new int(10));
    std::unique_ptr<int> uptr(new int(10));

    std::string_view str = "hello";

    std::function<int()> f;

    std::vector<bool> v{false, true};

    auto r = v[1];
    r = false;

    std::reverse_iterator<std::list<int>::iterator> u;

    int arr[10];
    int (&rarr)[10] = arr;

    print(42);
    print(42.0);
    print("hello");
    print(3.14f);
    print(std::string("hello"));

    return 0;
}
