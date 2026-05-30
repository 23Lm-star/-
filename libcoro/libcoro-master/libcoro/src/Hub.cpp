
#include "coro/Common.hpp"
#include "coro/Hub.hpp"
#include "coro/Error.hpp"

#ifdef __APPLE__
#include "Hub.osx.inl"
#elif defined(_WIN32)
#include "Hub.win.inl"
#else
#include "Hub.linux.inl"
#endif

namespace coro {

// Hub hash map: key is thread ID, value is corresponding Hub pointer
static std::unordered_map<std::thread::id, Ptr<Hub>> hubMap;
// Mutex to protect hubMap, only used during registration
static std::mutex hubMapMutex;

Ptr<Hub> hub() {
    std::lock_guard<std::mutex> lock(hubMapMutex);
    
    auto tid = std::this_thread::get_id();
    auto it = hubMap.find(tid);
    
    if (it != hubMap.end()) {
        // Current thread already has Hub, return it directly
        return it->second;
    }
    
    // Current thread doesn't have Hub, create new and register to hash map
    auto newHub = Ptr<Hub>(new Hub);
    hubMap[tid] = newHub;
    return newHub;
}

void cleanupHub() {
    std::lock_guard<std::mutex> lock(hubMapMutex);
    auto tid = std::this_thread::get_id();
    hubMap.erase(tid);
}

void run() {
    hub()->run();
}

Hub::Hub() : blocked_(0), waiting_(0), handle_(0) {
#if defined(_WIN32)
    WORD version = MAKEWORD(2, 2);
    WSADATA data;
    SetErrorMode(GetErrorMode()|SEM_NOGPFAULTERRORBOX);
    if (WSAStartup(version, &data) != 0) {
        abort();
    }
    handle_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
#elif defined(__APPLE__)
    handle_ = kqueue();
#elif defined(__linux__)
    handle_ = epoll_create(1);
#endif
    if (!handle_) {
        throw SystemError();
    }
    now_ = Time::now();
}

void Hub::timeoutIs(Timeout const& timeout) {
    timeout_.push(Timeout(timeout.time()+now_, timeout.coroutine()));
}

void Hub::quiesce() {
    std::vector<Ptr<Coroutine>> runnable;
    runnable.swap(runnable_);
    for (auto& coroutine : runnable) {
        main()->status_ = Coroutine::RUNNABLE;
        assert(coroutine->status()!=Coroutine::EXITED);
        coroutine->swap();
        switch (coroutine->status()) {
        case Coroutine::EXITED: break;
        case Coroutine::DELETED: break;
        case Coroutine::RUNNABLE:
            runnable_.push_back(coroutine);
            break;
        case Coroutine::BLOCKED:
        case Coroutine::WAITING:
            break;
        case Coroutine::NEW:
        case Coroutine::RUNNING:
        default: assert(!"illegal coroutine state"); break;
        }
    }
}

void Hub::run() {
    assert(coro::current() == coro::main());
    for (;;) {
        now_ = Time::now();
        while (!timeout_.empty() && timeout_.top().time() <= now_) {
            auto const timeout = timeout_.top();
            auto const coro = timeout.coroutine();
            coro->notify();
            timeout_.pop();
        }
        quiesce();
        if (runnable_.size()+blocked_+waiting_ <= 0) {
            return;
        }
        poll();
    }
}

}