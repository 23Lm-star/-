
#pragma once

#include "coro/Common.hpp"
#include "coro/Coroutine.hpp"

namespace coro {

Ptr<Hub> hub();
void run();

class Timeout {
public:
    Timeout(Time const& time, Ptr<Coroutine> coro) : time_(time), coroutine_(coro) {}
    bool operator<(Timeout const& rhs) const { return time_ > rhs.time_; }
    bool operator==(Timeout const& rhs) const { return time_ == rhs.time_; }

    Time const& time() const { return time_; }
    Ptr<Coroutine> coroutine() const { return coroutine_; }
private:
    Time time_;
    Ptr<Coroutine> coroutine_;
};

#ifdef _WIN32
struct Overlapped {
    OVERLAPPED overlapped;
    Coroutine* coroutine;
    DWORD bytes;
    DWORD error;
};
#endif

class Hub {
public:
    template <typename F>
    Ptr<Coroutine> start(F func) {
        Ptr<Coroutine> coro(new Coroutine(func));
        runnable_.push_back(coro);
        return coro;
    }
    void quiesce();
    void poll();
    void run();
#ifdef _WIN32
    HANDLE handle() const { return handle_; }
#else
    int handle() const { return handle_; }
#endif
    std::mutex const& mutex() const { return mutex_; }

private:
    Hub();
    std::vector<WeakPtr<Coroutine>> runnable_;
    std::priority_queue<Timeout, std::vector<Timeout>> timeout_;
    int blocked_;
    int waiting_;
#ifdef _WIN32
    HANDLE handle_;
#else
    int handle_;
#endif
    std::mutex mutex_;
    Time now_;

    void timeoutIs(Timeout const& timeout);

    friend Ptr<Hub> coro::hub();
    friend void coro::sleep(Time const& time);
    friend class Coroutine;
    friend class Event;
};

template <typename F>
Ptr<Coroutine> start(F func) {
    return hub()->start(func);
}

template <typename F>
Ptr<Coroutine> timer(F func, Time const& time) {
    return start([=] {
        std::function<void()> f = func;
        while (true) {
            f();
            coro::sleep(time);
        }
    });
}

}