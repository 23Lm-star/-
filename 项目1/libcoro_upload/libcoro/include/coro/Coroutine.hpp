
#pragma once

#include "coro/Common.hpp"
#include "coro/Time.hpp"

extern "C" {
void __cdecl coroSwapContext(coro::Coroutine* from, coro::Coroutine* to);
void __cdecl coroStart() throw();
}

namespace coro {

Ptr<Coroutine> current();
Ptr<Coroutine> main();
void yield();
void sleep(Time const& time);
#ifdef _WIN32
LONG WINAPI fault(LPEXCEPTION_POINTERS info);
#else
void fault(int signo, siginfo_t* info, void* context);
#endif


class Stack {
public:
    Stack(uint32_t size);
    ~Stack();
    uint8_t* end() { return data_+size_; }
    uint8_t* begin() { return data_; }
private:
    uint8_t* data_;
    uint64_t size_;
    friend class Coroutine;
};

class ExitException {
};


class Coroutine : public std::enable_shared_from_this<Coroutine> {
public:
    enum Status {
        NEW,
        RUNNABLE,
        RUNNING,
        BLOCKED,
        WAITING,
        DELETED,
        EXITED,
    };

    ~Coroutine();

    template <typename F>
    Coroutine(F func) : stack_(CORO_STACK_SIZE) { init(func); }
    Status status() const { return status_; }
    void join();

private:
    Coroutine();
    void init(std::function<void()> const& func);
    void commit(uint64_t addr);
    void exit();
    void start() throw();
    void swap();
    void yield();
    void block();
    void unblock();
    void wait();
    void notify();
    bool isMain() { return !stack_.begin(); }

    uint8_t* stackPointer_;
    std::function<void()> func_;
    Status status_;
    Stack stack_;
    Ptr<Event> event_;

    friend Ptr<Coroutine> coro::current();
    friend Ptr<Coroutine> coro::main();
    friend void coro::yield();
    friend void coro::sleep(Time const& time);
    friend void ::coroStart() throw();
#ifdef _WIN32
    friend LONG WINAPI coro::fault(LPEXCEPTION_POINTERS info);
#else
    friend void coro::fault(int signo, siginfo_t* info, void* context);
#endif
    friend class coro::Hub;
    friend class coro::Socket;
    friend class coro::Event;
    friend class coro::Selector;
};

}