

#include "coro/Common.hpp"
#include "coro/Event.hpp"
#include "coro/Hub.hpp"

namespace coro {


EventRecord::EventRecord(Ptr<Coroutine> coro) {
    coroutine_ = coro;
}

EventRecord::EventRecord() {
}

void Event::notifyAll() {
    std::vector<EventRecord> waiter;
    waiter.swap(waiter_);
    for (auto record : waiter) {
        Ptr<Coroutine> coro = record.lock();
        if (coro) {
            coro->notify();
        }
    }
    assert(waiter_.size()==0);
}

void Event::wait() {
    waiter_.push_back(EventRecord(current()));
    current()->wait();
}

bool Event::timedWaitFor(const Time& timeout) {
    hub()->timeoutIs(Timeout(timeout, current()));
    waiter_.push_back(EventRecord(current()));
    current()->wait();
    for (size_t i = 0; i < waiter_.size(); ++i) {
        Ptr<Coroutine> coro = waiter_[i].lock();
        if (coro.get() == current().get()) {
            waiter_.erase(waiter_.begin() + i);
            return false;
        }
    }
    return true;
}

size_t Event::waitToken(Ptr<Coroutine> waiter) {
    waiter_.push_back(EventRecord(waiter));
    return waiter_.size()-1;
}

bool Event::waitTokenValid(Ptr<Coroutine> waiter, EventWaitToken token) {
    if (token < waiter_.size()) {
        Ptr<Coroutine> coro = waiter_[token].lock();
        return coro.get() == waiter.get();
    } else {
        return false;
    }
}

void Event::waitTokenDel(EventWaitToken token) {
    assert(size_t(token) < waiter_.size() && "invalid wait token");
    waiter_[token] = EventRecord();
}

}
