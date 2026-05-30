#include <coro/Common.hpp>
#include <coro/coro.hpp>
#include <atomic>
#include <vector>
#include <iostream>
#include <iomanip>
#include <chrono>

/**
 * @brief Event 同步机制增强测试
 */

int main() {
    std::cout << "========== Event Synchronization Enhanced Test ==========" << std::endl;
    
    // Test 1: Multiple coroutines waiting for single event (broadcast)
    std::cout << "\n[Test 1] Multiple coroutines waiting for single event..." << std::endl;
    {
        coro::Event event;
        std::atomic<int> counter{0};
        const int numWaiters = 100;
        
        std::vector<coro::Ptr<coro::Coroutine>> waiters;
        for (int i = 0; i < numWaiters; ++i) {
            waiters.push_back(coro::start([&event, &counter, i]() {
                std::cout << "[Waiter " << std::setw(3) << i << "] Waiting for event..." << std::endl;
                event.wait();
                counter.fetch_add(1);
                std::cout << "[Waiter " << std::setw(3) << i << "] Event received!" << std::endl;
            }));
        }
        
        auto notifier = coro::start([&event]() {
            coro::sleep(coro::Time::millisec(100));
            std::cout << "[Notifier] Triggering event for all waiters..." << std::endl;
            event.notifyAll();
        });
        
        coro::run();
        
        std::cout << "[Test 1 Result] Expected: " << numWaiters << ", Actual: " << counter.load() << std::endl;
        assert(counter.load() == numWaiters);
        std::cout << "[Test 1] PASSED" << std::endl;
    }
    
    // Test 2: Multiple independent events with multiple waiters each
    std::cout << "\n[Test 2] Multiple independent events with multiple waiters each..." << std::endl;
    {
        const int numEvents = 5;
        const int waitersPerEvent = 20;
        std::vector<coro::Event> events(numEvents);
        std::vector<std::atomic<int>> counters(numEvents);
        
        std::vector<coro::Ptr<coro::Coroutine>> allCoroutines;
        for (int e = 0; e < numEvents; ++e) {
            counters[e] = 0;
            for (int w = 0; w < waitersPerEvent; ++w) {
                allCoroutines.push_back(coro::start([&events, &counters, e, w]() {
                    std::cout << "[Event " << e << ", Waiter " << w << "] Waiting..." << std::endl;
                    events[e].wait();
                    counters[e].fetch_add(1);
                    std::cout << "[Event " << e << ", Waiter " << w << "] Notified!" << std::endl;
                }));
            }
        }
        
        for (int e = 0; e < numEvents; ++e) {
            allCoroutines.push_back(coro::start([&events, e]() {
                coro::sleep(coro::Time::millisec(50 + e * 20));
                std::cout << "[Notifier] Triggering event " << e << std::endl;
                events[e].notifyAll();
            }));
        }
        
        coro::run();
        
        bool allPassed = true;
        for (int e = 0; e < numEvents; ++e) {
            std::cout << "[Event " << e << "] Expected: " << waitersPerEvent 
                      << ", Actual: " << counters[e].load() << std::endl;
            if (counters[e].load() != waitersPerEvent) {
                allPassed = false;
            }
        }
        std::cout << "[Test 2] " << (allPassed ? "PASSED" : "FAILED") << std::endl;
    }
    
    // Test 3: Event wait with timeout
    std::cout << "\n[Test 3] Event wait with timeout..." << std::endl;
    {
        coro::Event event;
        std::atomic<bool> timeoutOccurred{false};
        
        auto waiter = coro::start([&event, &timeoutOccurred]() {
            std::cout << "[Timeout Waiter] Waiting with 200ms timeout..." << std::endl;
            bool result = event.timedWaitFor(coro::Time::millisec(200));
            if (!result) {
                timeoutOccurred = true;
                std::cout << "[Timeout Waiter] Timeout occurred!" << std::endl;
            }
        });
        
        coro::run();
        
        std::cout << "[Test 3 Result] Timeout occurred: " << std::boolalpha << timeoutOccurred.load() << std::endl;
        assert(timeoutOccurred.load() == true);
        std::cout << "[Test 3] PASSED" << std::endl;
    }
    
    // Test 4: Conditional wait with predicate
    std::cout << "\n[Test 4] Conditional wait with predicate..." << std::endl;
    {
        coro::Event event;
        std::atomic<int> sharedValue{0};
        std::atomic<int> waiterCount{0};
        const int target = 10;
        
        std::vector<coro::Ptr<coro::Coroutine>> producers;
        for (int i = 0; i < 5; ++i) {
            producers.push_back(coro::start([&event, &sharedValue, target]() {
                while (sharedValue.load() < target) {
                    coro::sleep(coro::Time::millisec(10));
                    sharedValue.fetch_add(1);
                    event.notifyAll();
                    std::cout << "[Producer] Value updated to: " << sharedValue.load() << std::endl;
                }
            }));
        }
        
        auto waiter = coro::start([&event, &sharedValue, &waiterCount, target]() {
            std::cout << "[Conditional Waiter] Waiting for value >= " << target << std::endl;
            event.wait([&]() {
                return sharedValue.load() >= target;
            });
            waiterCount.fetch_add(1);
            std::cout << "[Conditional Waiter] Condition met! Value = " << sharedValue.load() << std::endl;
        });
        
        coro::run();
        
        std::cout << "[Test 4 Result] Waiter count: " << waiterCount.load() << std::endl;
        assert(waiterCount.load() == 1);
        assert(sharedValue.load() >= target);
        std::cout << "[Test 4] PASSED" << std::endl;
    }
    
    // Test 5: Massive concurrent coroutine stress test
    std::cout << "\n[Test 5] Massive concurrent coroutine stress test..." << std::endl;
    {
        const int numEvents = 10;
        const int waitersPerEvent = 100;
        std::vector<coro::Event> events(numEvents);
        std::vector<std::atomic<int>> counters(numEvents);
        
        std::vector<coro::Ptr<coro::Coroutine>> allCoroutines;
        for (int e = 0; e < numEvents; ++e) {
            counters[e] = 0;
            for (int w = 0; w < waitersPerEvent; ++w) {
                allCoroutines.push_back(coro::start([&events, &counters, e]() {
                    events[e].wait();
                    counters[e].fetch_add(1);
                }));
            }
        }
        
        for (int e = 0; e < numEvents; ++e) {
            allCoroutines.push_back(coro::start([&events, e]() {
                coro::sleep(coro::Time::millisec(10));
                events[e].notifyAll();
            }));
        }
        
        std::cout << "[Stress Test] Created " << allCoroutines.size() << " coroutines..." << std::endl;
        auto start = std::chrono::high_resolution_clock::now();
        
        coro::run();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        bool allPassed = true;
        int totalNotified = 0;
        for (int e = 0; e < numEvents; ++e) {
            totalNotified += counters[e].load();
            if (counters[e].load() != waitersPerEvent) {
                allPassed = false;
            }
        }
        
        std::cout << "[Stress Test Result] Total notified: " << totalNotified 
                  << " in " << duration.count() << "ms" << std::endl;
        std::cout << "[Test 5] " << (allPassed ? "PASSED" : "FAILED") << std::endl;
    }
    
    std::cout << "\n---" << std::endl;
    std::cout << "All Event synchronization tests completed!" << std::endl;
    
    return 0;
}