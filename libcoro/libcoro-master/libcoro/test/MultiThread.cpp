/**
 * @file MultiThread.cpp
 * @brief Multi-thread and multi-scheduler test file
 */

#include "coro/Common.hpp"
#include "coro/coro.hpp"
#include <thread>
#include <atomic>
#include <iostream>
#include <chrono>

void testSingleCoroutine() {
    std::cout << "========== Test: Single Coroutine in Worker Thread ===========" << std::endl;
    std::cout << "---" << std::endl;

    std::atomic<int> counter{0};

    std::thread worker([&]() {
        coro::main();
        std::cout << "[Worker] Thread started, creating coroutine" << std::endl;

        coro::start([&]() {
            std::cout << "[Worker] Coroutine running! Incrementing counter" << std::endl;
            counter++;
            coro::yield();
            std::cout << "[Worker] Coroutine resumed, counter = " << counter.load() << std::endl;
        });

        std::cout << "[Worker] Calling coro::run()" << std::endl;
        coro::run();
        std::cout << "[Worker] coro::run() returned" << std::endl;
        
        coro::cleanupHub();
    });

    worker.join();

    std::cout << "---" << std::endl;
    std::cout << "Counter value: " << counter.load() << " (expected: 1)" << std::endl;
    std::cout << "Test " << (counter.load() == 1 ? "PASSED" : "FAILED") << std::endl;
    std::cout << std::endl;
}

void testTwoCoroutines() {
    std::cout << "========== Test: Two Coroutines in Worker Thread ===========" << std::endl;
    std::cout << "---" << std::endl;

    std::atomic<int> counter{0};

    std::thread worker([&]() {
        coro::main();
        std::cout << "[Worker] Thread started" << std::endl;

        coro::start([&]() {
            std::cout << "[Worker] Coroutine A started" << std::endl;
            counter++;
            std::cout << "[Worker] Coroutine A incremented counter to " << counter.load() << std::endl;
            coro::yield();
            std::cout << "[Worker] Coroutine A resumed" << std::endl;
        });

        coro::start([&]() {
            std::cout << "[Worker] Coroutine B started" << std::endl;
            counter++;
            std::cout << "[Worker] Coroutine B incremented counter to " << counter.load() << std::endl;
            coro::yield();
            std::cout << "[Worker] Coroutine B resumed" << std::endl;
        });

        std::cout << "[Worker] Two coroutines created" << std::endl;
        std::cout << "[Worker] Calling coro::run()" << std::endl;
        coro::run();
        std::cout << "[Worker] coro::run() returned" << std::endl;
        
        coro::cleanupHub();
    });

    worker.join();

    std::cout << "---" << std::endl;
    std::cout << "Counter value: " << counter.load() << " (expected: 2)" << std::endl;
    std::cout << "Test " << (counter.load() == 2 ? "PASSED" : "FAILED") << std::endl;
    std::cout << std::endl;
}

void testSleepCoroutine() {
    std::cout << "========== Test: Coroutine with Sleep ===========" << std::endl;
    std::cout << "---" << std::endl;

    std::atomic<int> counter{0};
    auto startTime = std::chrono::steady_clock::now();

    std::thread worker([&]() {
        coro::main();
        std::cout << "[Worker] Thread started" << std::endl;

        coro::start([&]() {
            std::cout << "[Worker] Coroutine started, will sleep for 100ms" << std::endl;
            counter++;
            coro::sleep(coro::Time::millisec(100));
            std::cout << "[Worker] Coroutine woke up after sleep" << std::endl;
            counter++;
        });

        std::cout << "[Worker] Calling coro::run()" << std::endl;
        coro::run();
        std::cout << "[Worker] coro::run() returned" << std::endl;
        
        coro::cleanupHub();
    });

    worker.join();

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    std::cout << "---" << std::endl;
    std::cout << "Counter value: " << counter.load() << " (expected: 2)" << std::endl;
    std::cout << "Elapsed time: " << duration.count() << "ms (expected: ~100ms)" << std::endl;
    std::cout << "Test " << (counter.load() == 2 ? "PASSED" : "FAILED") << std::endl;
    std::cout << std::endl;
}

void testMultipleThreads() {
    std::cout << "========== Test: Multiple Threads with Coroutines ===========" << std::endl;
    std::cout << "---" << std::endl;

    std::atomic<int> totalCounter{0};
    const int numThreads = 3;
    const int coroutinesPerThread = 2;
    std::vector<std::thread> threads;

    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&totalCounter, coroutinesPerThread]() {
            coro::main();
            
            for (int c = 0; c < coroutinesPerThread; ++c) {
                coro::start([&totalCounter]() {
                    totalCounter++;
                    coro::yield();
                });
            }
            
            coro::run();
            coro::cleanupHub();
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    int expected = numThreads * coroutinesPerThread;
    std::cout << "---" << std::endl;
    std::cout << "Total counter value: " << totalCounter.load() << " (expected: " << expected << ")" << std::endl;
    std::cout << "Test " << (totalCounter.load() == expected ? "PASSED" : "FAILED") << std::endl;
    std::cout << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "   Multi-Thread Coroutine Test          " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    try {
        testSingleCoroutine();
        testTwoCoroutines();
        testSleepCoroutine();
        testMultipleThreads();

        std::cout << "========================================" << std::endl;
        std::cout << "          All Tests Completed           " << std::endl;
        std::cout << "========================================" << std::endl;
    } catch (std::exception const& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
