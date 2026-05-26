

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
        if (record.coroutine()) {
            record.coroutine()->notify();
        }
    }
    assert(waiter_.size()==0);
}

void Event::wait() {
    waiter_.push_back(EventRecord(current()));
    current()->wait();
}

size_t Event::waitToken(Ptr<Coroutine> waiter) {
    waiter_.push_back(EventRecord(current()));
    return waiter_.size()-1;
}

bool Event::waitTokenValid(Ptr<Coroutine> waiter, EventWaitToken token) {
    if (token < waiter_.size()) {
        return waiter_[token].coroutine() == waiter;
    } else {
        return false;
    }
}

void Event::waitTokenDel(EventWaitToken token) {
    assert(size_t(token) < waiter_.size() && "invalid wait token");
    waiter_[token] = EventRecord();
}

}
