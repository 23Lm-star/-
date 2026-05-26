
#pragma once

#include "coro/Common.hpp"

namespace coro {

class EventRecord {
public:
    EventRecord(Ptr<Coroutine> coro);
    EventRecord();
    Ptr<Coroutine> coroutine() const { return coroutine_; }

private:
    Ptr<Coroutine> coroutine_;
};

typedef size_t EventWaitToken;

class Event {
public:
    virtual ~Event() {}
    void notifyAll();
    void wait();

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