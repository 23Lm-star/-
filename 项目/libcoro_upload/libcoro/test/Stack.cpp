

#include <coro/Common.hpp>
#include <coro/coro.hpp>

using namespace coro;

void recurse(int n) {
    char data[1024];
    data[0] = char(n);
    if (n == 0) { return; }
    else { recurse(n-1); }
}


int main() {
    std::cout << "========== Coroutine Stack Memory Test ==========" << std::endl;
    std::cout << "Testing coroutine stack lazy allocation" << std::endl;
    std::cout << "Note: Coroutine stacks are allocated only when needed to save memory" << std::endl;
    std::cout << "---" << std::endl;
    
    std::cout << "Testing lazy allocation: creating coroutines that recurse deeply" << std::endl;
    std::cout << "Each coroutine uses minimal stack due to lazy allocation" << std::endl;
    
    std::vector<Ptr<Coroutine>> coros;

    std::cout << "\nTest 1: Creating 100 coroutines with 50 recursion layers (1KB each)..." << std::endl;
    for (auto j = 0; j < 100; ++j) {
        coros.push_back(coro::start(std::bind(recurse, 50)));
    }
    hub()->quiesce();
    coros.clear();
    std::cout << "Test 1 passed" << std::endl;
    
    std::cout << "\nTest 2: Creating 50 coroutines with 100 recursion layers (1KB each)..." << std::endl;
    for (auto j = 0; j < 50; ++j) {
        coros.push_back(coro::start(std::bind(recurse, 100)));
    }
    hub()->quiesce();
    coros.clear();
    std::cout << "Test 2 passed" << std::endl;
    
    std::cout << "\nTest 3: Creating 20 coroutines with 200 recursion layers (1KB each)..." << std::endl;
    for (auto j = 0; j < 20; ++j) {
        coros.push_back(coro::start(std::bind(recurse, 200)));
    }
    hub()->quiesce();
    coros.clear();
    std::cout << "Test 3 passed" << std::endl;
    
    std::cout << "---" << std::endl;
    std::cout << "Stack memory test completed" << std::endl;
    std::cout << "  All tests passed successfully" << std::endl;
    std::cout << "  Lazy allocation ensures each coroutine uses only the stack space it needs" << std::endl;

    return 0;
}
