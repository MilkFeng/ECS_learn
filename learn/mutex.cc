#include "threadsafe_stack.hpp"

int main() {
    threadsafe_stack<int> stack;
    stack.push(1);
    stack.push(2);
    stack.push(3);

    std::shared_ptr<int> p1 = stack.pop();
    std::shared_ptr<int> p2 = stack.pop();
    std::shared_ptr<int> p3 = stack.pop();

    return 0;
}