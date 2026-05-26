
#include <coro/Common.hpp>
#include <coro/coro.hpp>

int main() {
    std::cout << "========== Coroutine Join Test ==========" << std::endl;
    std::cout << "Testing coroutine join: one coroutine waits for another to complete" << std::endl;
    std::cout << "---" << std::endl;

    auto counter = 0;

    std::cout << "Creating coroutine one (will yield CPU)..." << std::endl;
    auto one = coro::start([&]{
        std::cout << "[Coroutine one] Starting, executing yield..." << std::endl;
        coro::yield();
        std::cout << "[Coroutine one] counter = " << counter << " (expected: 0)" << std::endl;
        assert(counter==0);
        counter++;
        std::cout << "[Coroutine one] counter++ now equals " << counter << std::endl;
        std::cout << "[Coroutine one] Execution completed" << std::endl;
    });

    std::cout << "Creating coroutine two (will join coroutine one)..." << std::endl;
    auto two = coro::start([&]{
        std::cout << "[Coroutine two] Starting, waiting for coroutine one to complete..." << std::endl;
        one->join();
        std::cout << "[Coroutine two] Coroutine one completed, continuing execution" << std::endl;
        std::cout << "[Coroutine two] counter = " << counter << " (expected: 1)" << std::endl;
        assert(counter==1);
        std::cout << "[Coroutine two] Execution completed" << std::endl;
    });

    std::cout << "---" << std::endl;
    std::cout << "Starting coroutine scheduler..." << std::endl;
    coro::run();

    std::cout << "---" << std::endl;
    std::cout << "Join test completed: Coroutine two correctly waited for coroutine one" << std::endl;
    return 0;
}