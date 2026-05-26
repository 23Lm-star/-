
#include "coro/Common.hpp"
#include "coro/Coroutine.hpp"
#include "coro/Hub.hpp"

void foo() {
    try {
        std::cout << "[Coroutine foo] First output" << std::endl;
        coro::yield();
    } catch (coro::ExitException const&) {
        std::cout << "[Coroutine foo] Caught exception" << std::endl;
        throw;
    }
    std::cout << "[Coroutine foo] Second output" << std::endl;

    coro::sleep(coro::Time::sec(.4));
    std::cout << "[Coroutine foo] Delayed output one" << std::endl;
    coro::sleep(coro::Time::sec(.4));
    std::cout << "[Coroutine foo] Delayed output two" << std::endl;
}

void bar() {
    for (auto i = 0; i < 2; ++i) {
        coro::sleep(coro::Time::millisec(1000));
        std::cout << "[Coroutine bar] Delayed output " << (i+1) << "/2" << std::endl;
    }
}

void baz() {
    for (auto i = 0; i < 20; ++i) {
        coro::sleep(coro::Time::millisec(100));
        std::cout << "[Coroutine baz] Fast output " << (i+1) << "/20" << std::endl;
    }
}

int main() {
    std::cout << "========== Coroutine Basic Test ==========" << std::endl;
    std::cout << "Starting 3 coroutines: foo, bar, baz" << std::endl;
    std::cout << "---" << std::endl;

    auto cbaz = coro::start(baz);
    auto cbar = coro::start(bar);
    auto cfoo = coro::start(foo);
    coro::run();

    std::cout << "---" << std::endl;
    std::cout << "All coroutines completed" << std::endl;
    return 0;
}