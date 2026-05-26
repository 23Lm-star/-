

#include "coro/Common.hpp"
#include "coro/Selector.hpp"
#include "coro/Coroutine.hpp"
#include "coro/Event.hpp"

namespace coro {

SelectorRecord::SelectorRecord(Ptr<Event> event, SelectorFunc func, EventWaitToken token) {
    event_ = event;
    func_ = func;
    token_ = token;
}

Selector::~Selector() {
    current()->wait();
    for (auto record : record_) {
        if (record.event()->waitTokenValid(current(), record.token())) {
            record.event()->waitTokenDel(record.token()); 
        } else {
            record.func()();
        }
    }
}

Selector& Selector::on(Ptr<Event> event, SelectorFunc func) {
    record_.push_back(SelectorRecord(event, func, event->waitToken(current())));
    return *this;
}


}
