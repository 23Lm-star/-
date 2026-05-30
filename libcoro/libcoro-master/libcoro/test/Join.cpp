
#include <coro/Common.hpp>
#include <coro/coro.hpp>
#include <thread>
#include <chrono>

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
    std::cout << std::endl;

    std::cout << "========== Timed Join Test ==========" << std::endl;
    std::cout << "Testing timed join: join with timeout shorter than coroutine execution time" << std::endl;
    std::cout << "---" << std::endl;

    std::atomic<int> atomicCounter{0};

    std::cout << "Creating coroutine three (will run for ~200ms then exit)..." << std::endl;
    auto three = coro::start([&]{
        std::cout << "[Coroutine three] Starting, will run for 200ms..." << std::endl;
        auto startTime = std::chrono::steady_clock::now();
        while (true) {
            coro::yield();
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
            if (elapsed >= 200) {
                break;
            }
        }
        atomicCounter = 999;
        std::cout << "[Coroutine three] Completed after 200ms, atomicCounter = " << atomicCounter.load() << std::endl;
    });

    std::cout << "Creating coroutine four (will join with 50ms timeout - will timeout)..." << std::endl;
    bool joinResult = false;
    auto four = coro::start([&]{
        std::cout << "[Coroutine four] Starting, will join with 50ms timeout..." << std::endl;
        auto startTime = std::chrono::steady_clock::now();
        joinResult = three->join(coro::Time::millisec(50));
        auto endTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        std::cout << "[Coroutine four] join() returned: " << (joinResult ? "true" : "false") << " (expected: false - timeout)" << std::endl;
        std::cout << "[Coroutine four] Elapsed time: " << elapsed << "ms (expected: ~50ms)" << std::endl;
        assert(joinResult == false);
        std::cout << "[Coroutine four] Timeout correctly detected! Continuing execution..." << std::endl;
        std::cout << "[Coroutine four] This proves timed join prevents infinite blocking" << std::endl;
    });

    std::cout << "---" << std::endl;
    std::cout << "Starting coroutine scheduler for timed join test..." << std::endl;
    coro::run();

    std::cout << "---" << std::endl;
    std::cout << "Final atomicCounter: " << atomicCounter.load() << " (should be 999 - coroutine completed after timeout)" << std::endl;
    std::cout << "Timed Join test completed: Timeout correctly prevented infinite blocking" << std::endl;
    std::cout << std::endl;

    std::cout << "========== All Join Tests Passed ==========" << std::endl;
    return 0;
}