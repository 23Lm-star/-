
#pragma once

#include "coro/Common.hpp"
#include "coro/Time.hpp"

namespace coro {

class EventRecord {
public:
    EventRecord(Ptr<Coroutine> coro);
    EventRecord();
    Ptr<Coroutine> lock() const { return coroutine_.lock(); }

private:
    WeakPtr<Coroutine> coroutine_;
};

typedef size_t EventWaitToken;

class Event {
public:
    virtual ~Event() {}
    void notifyAll();
    void wait();
    bool timedWaitFor(const Time& timeout);

    template <typename F>
    void wait(F cond) {
        while (!cond()) {
            wait();
        }
    }

private:
    size_t waiters() const { return waiter_.size(); }
    EventWaitToken waitToken(Ptr<Coroutine> waiter);
    bool waitTokenValid(Ptr<Coroutine> waiter, EventWaitToken token);
    void waitTokenDel(EventWaitToken token);

    std::vector<EventRecord> waiter_;
    friend class Selector;
};

}