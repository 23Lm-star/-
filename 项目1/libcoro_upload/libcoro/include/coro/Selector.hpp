

#pragma once

#include "coro/Common.hpp"
#include "coro/Event.hpp"

namespace coro {

typedef std::function<void()> SelectorFunc;

class SelectorRecord {
public:
    SelectorRecord(Ptr<Event> event, SelectorFunc func, EventWaitToken token);

    Ptr<Event> event() const { return event_; }
    SelectorFunc func() const { return func_; }
    EventWaitToken token() const { return token_; }

private:
    Ptr<Event> event_;
    SelectorFunc func_; 
    EventWaitToken token_;
};

class Selector {
public:
    ~Selector();
    Selector& on(Ptr<Event> event, SelectorFunc func);

private:
    std::vector<SelectorRecord> record_; 
};


}
