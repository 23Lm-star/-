
#include <coro/Common.hpp>
#include <coro/coro.hpp>

using namespace coro;

Ptr<Event> e1(new Event);
Ptr<Event> e2(new Event);
Ptr<Event> e3(new Event);

void publisher() {
    std::cout << "[Publisher] Preparing to publish events..." << std::endl;
    std::cout << "[Publisher] Triggering event e1" << std::endl;
    e1->notifyAll();
    std::cout << "[Publisher] Triggering event e2" << std::endl;
    e2->notifyAll();
    std::cout << "[Publisher] Publishing completed" << std::endl;
}

void consumer() {
    int count = 2;
    std::cout << "[Consumer] Starting to wait for events, need to receive " << count << " events" << std::endl;
    while (count > 0) {
        coro::Selector()
            .on(e1, [&](){
                count--;
                std::cout << "[Consumer] Received event e1 (" << count << " remaining)" << std::endl;
            })
            .on(e2, [&](){
                count--;
                std::cout << "[Consumer] Received event e2 (" << count << " remaining)" << std::endl;
            })
            .on(e3, [&](){
                std::cout << "[Consumer] Received event e3 (will not be triggered)" << std::endl;
            });
    }
    std::cout << "[Consumer] All events received" << std::endl;
}

int main() {
    std::cout << "========== Event Selector Test ==========" << std::endl;
    std::cout << "Testing Selector: listening to multiple events simultaneously" << std::endl;
    std::cout << "---" << std::endl;

    std::cout << "Creating consumer coroutine (listening to e1 and e2 events)..." << std::endl;
    auto b = coro::start(consumer);

    std::cout << "Creating publisher coroutine (triggering e1 and e2 events)..." << std::endl;
    auto a = coro::start(publisher);

    std::cout << "---" << std::endl;
    std::cout << "Starting coroutine scheduler..." << std::endl;
    coro::run();

    std::cout << "---" << std::endl;
    std::cout << "Selector test completed" << std::endl;

    return 0;
}