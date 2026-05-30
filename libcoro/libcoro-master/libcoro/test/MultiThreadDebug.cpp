/**
 * @file MultiThreadDebug.cpp
 * @brief Debug test for multi-thread coroutine scheduling
 */

#include <coro/Common.hpp>
#include <coro/coro.hpp>
#include <thread>
#include <atomic>
#include <vector>
#include <iostream>

void testSimpleCoroutineInWorker() {
    std::cout << "========== Test: Simple Coroutine in Worker Thread ==========" << std::endl;
    std::cout << "---" << std::endl;

    std::atomic<int> counter{0};

    std::thread worker([&]() {
        std::cout << "[Worker] Before coro::main()" << std::endl;
        coro::main();
        std::cout << "[Worker] After coro::main()" << std::endl;

        std::cout << "[Worker] Before coro::start()" << std::endl;
        auto coro = coro::start([&]() {
            std::cout << "[Worker] Coroutine started" << std::endl;
            counter++;
            std::cout << "[Worker] Counter incremented to " << counter.load() << std::endl;
        });
        std::cout << "[Worker] After coro::start()" << std::endl;

        std::cout << "[Worker] Before coro::run()" << std::endl;
        coro::run();
        std::cout << "[Worker] After coro::run()" << std::endl;
    });

    worker.join();

    std::cout << "---" << std::endl;
    std::cout << "Counter value: " << counter.load() << " (expected: 1)" << std::endl;
    std::cout << "Test " << (counter.load() == 1 ? "PASSED" : "FAILED") << std::endl;
    std::cout << std::endl;
}

void testCoroutineWithYield() {
    std::cout << "========== Test: Coroutine with Yield ==========" << std::endl;
    std::cout << "---" << std::endl;

    std::atomic<int> counter{0};

    std::thread worker([&]() {
        coro::main();

        auto coro = coro::start([&]() {
            std::cout << "[Worker] Step 1" << std::endl;
            counter++;
            coro::yield();
            std::cout << "[Worker] Step 2" << std::endl;
            counter++;
        });

        std::cout << "[Worker] Before run()" << std::endl;
        coro::run();
        std::cout << "[Worker] After run()" << std::endl;
    });

    worker.join();

    std::cout << "---" << std::endl;
    std::cout << "Counter value: " << counter.load() << " (expected: 2)" << std::endl;
    std::cout << "Test " << (counter.load() == 2 ? "PASSED" : "FAILED") << std::endl;
    std::cout << std::endl;
}

void testCoroutineWithSleep() {
    std::cout << "========== Test: Coroutine with Sleep ==========" << std::endl;
    std::cout << "---" << std::endl;

    std::atomic<int> counter{0};

    std::thread worker([&]() {
        coro::main();

        std::cout << "[Worker] Creating coroutine..." << std::endl;
        auto coro = coro::start([&]() {
            std::cout << "[Worker] Coroutine running, sleeping 10ms..." << std::endl;
            counter++;
            coro::sleep(coro::Time::millisec(10));
            std::cout << "[Worker] After sleep, counter=" << counter.load() << std::endl;
            counter++;
        });

        std::cout << "[Worker] Before run()" << std::endl;
        coro::run();
        std::cout << "[Worker] After run(), counter=" << counter.load() << std::endl;
    });

    worker.join();

    std::cout << "---" << std::endl;
    std::cout << "Counter value: " << counter.load() << " (expected: 2)" << std::endl;
    std::cout << "Test " << (counter.load() == 2 ? "PASSED" : "FAILED") << std::endl;
    std::cout << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "    Multi-Thread Debug Test Suite       " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    try {
        testSimpleCoroutineInWorker();
        testCoroutineWithYield();
        testCoroutineWithSleep();

        std::cout << "========================================" << std::endl;
        std::cout << "          All Tests Completed           " << std::endl;
        std::cout << "========================================" << std::endl;
    } catch (std::exception const& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}