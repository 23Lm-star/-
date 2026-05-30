#include <coro/Common.hpp>
#include <coro/coro.hpp>

using namespace coro;

void recurse(int n) {
    char data[1024];
    data[0] = char(n);
    if (n == 0) { return; }
    else { recurse(n-1); }
}

void deepRecurse(int n) {
    char data[4096];
    memset(data, n & 0xFF, sizeof(data));
    if (n == 0) return;
    deepRecurse(n - 1);
}

int main() {
    std::cout << "========== Coroutine Stack Memory Test ==========" << std::endl;
    std::cout << "Testing coroutine stack memory under heavy load" << std::endl;
    std::cout << "---" << std::endl;
    
    std::vector<Ptr<Coroutine>> coros;

    std::cout << "\nTest 1: 500 coroutines with 50 recursion layers (1KB each)..." << std::endl;
    for (auto j = 0; j < 500; ++j) {
        coros.push_back(coro::start(std::bind(recurse, 50)));
    }
    hub()->quiesce();
    coros.clear();
    std::cout << "Test 1 passed" << std::endl;
    
    std::cout << "\nTest 2: 200 coroutines with 200 recursion layers (1KB each)..." << std::endl;
    for (auto j = 0; j < 200; ++j) {
        coros.push_back(coro::start(std::bind(recurse, 200)));
    }
    hub()->quiesce();
    coros.clear();
    std::cout << "Test 2 passed" << std::endl;
    
    std::cout << "\nTest 3: 100 coroutines with 500 recursion layers (1KB each)..." << std::endl;
    for (auto j = 0; j < 100; ++j) {
        coros.push_back(coro::start(std::bind(recurse, 500)));
    }
    hub()->quiesce();
    coros.clear();
    std::cout << "Test 3 passed" << std::endl;
    
    std::cout << "\nTest 4: 50 coroutines with 4KB stack and 100 recursion layers..." << std::endl;
    for (auto j = 0; j < 50; ++j) {
        coros.push_back(coro::start(std::bind(deepRecurse, 100)));
    }
    hub()->quiesce();
    coros.clear();
    std::cout << "Test 4 passed" << std::endl;
    
    std::cout << "\nTest 5: Mixed workload - interleaved coroutines with varying stack usage..." << std::endl;
    for (auto j = 0; j < 100; ++j) {
        if (j % 3 == 0) {
            coros.push_back(coro::start(std::bind(recurse, 100)));
        } else if (j % 3 == 1) {
            coros.push_back(coro::start(std::bind(deepRecurse, 50)));
        } else {
            coros.push_back(coro::start(std::bind(recurse, 300)));
        }
    }
    hub()->quiesce();
    coros.clear();
    std::cout << "Test 5 passed" << std::endl;
    
    std::cout << "\nTest 6: Stress test - 1000 coroutines with moderate recursion...\n" << std::endl;
    for (auto j = 0; j < 1000; ++j) {
        coros.push_back(coro::start(std::bind(recurse, 10), 65536));
    }
    hub()->quiesce();
    coros.clear();
    std::cout << "Test 6 passed" << std::endl;

    std::cout << "---" << std::endl;
    std::cout << "Stack memory test completed" << std::endl;
    std::cout << "  All tests passed successfully" << std::endl;

    return 0;
}
