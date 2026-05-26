
#include <coro/Common.hpp>
#include <coro/coro.hpp>

int main() {
    std::cout << "========== Event Synchronization Test ==========" << std::endl;

    auto event = coro::Event();
    auto trigger = false;

    std::cout << "Creating notifier coroutine..." << std::endl;
    auto notifier = coro::start([&]() {
        std::cout << "[Notifier] Triggering event" << std::endl;
        trigger = true;
        event.notifyAll();
        std::cout << "[Notifier] Event triggered, notifying all waiters" << std::endl;
    });

    std::cout << "Creating waiter coroutine..." << std::endl;
    auto waiter = coro::start([&]() {
        std::cout << "[Waiter] Waiting for event trigger..." << std::endl;
        event.wait([&]() { return trigger; });
        std::cout << "[Waiter] Event triggered, condition met!" << std::endl;
    });

    std::cout << "---" << std::endl;
    std::cout << "Starting event loop..." << std::endl;
    coro::run();

    std::cout << "---" << std::endl;
    std::cout << "Event synchronization test completed" << std::endl;

    return 0;
}