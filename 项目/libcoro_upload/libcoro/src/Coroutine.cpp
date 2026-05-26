
#include "coro/Common.hpp"
#include "coro/Coroutine.hpp"
#include "coro/Hub.hpp"
#include "coro/Error.hpp"
#include "coro/Event.hpp"

extern "C" {
coro::Coroutine* coroCurrent = coro::main().get();
}

void coroStart() throw() { coroCurrent->start(); }

#ifdef _WIN32
#include "Coroutine.win.inl"
#else
#include "Coroutine.unix.inl"
#endif


namespace coro {

#if defined(_WIN64)
struct StackFrame {
    void* gs16;
    void* gs8;
    void* gs0;
    void* r15;
    void* r14;
    void* r13;
    void* r12;
    void* r11;
    void* r10;
    void* r9;
    void* r8;
    void* rdi;
    void* rsi;
    void* rdx;
    void* rcx;
    void* rbx;
    void* rax;
    void* rbp;
    void* returnAddr;
    void* padding;
};
#elif defined(_WIN32)
struct StackFrame {
    void* fs8;
    void* fs4;
    void* fs0;
    void* rdi;
    void* rsi;
    void* rdx;
    void* rcx;
    void* rbx;
    void* rax;
    void* rbp;
    void* returnAddr;
};
#else
struct StackFrame {
    void* r15;
    void* r14;
    void* r13;
    void* r12;
    void* r11;
    void* r10;
    void* r9;
    void* r8;
    void* rdi;
    void* rsi;
    void* rdx;
    void* rcx;
    void* rbx;
    void* rax;
    void* rbp;
    void* returnAddr;
    void* padding;
};
#endif

uint64_t pageRound(uint64_t addr, uint64_t multiple) {
    return (addr/multiple)*multiple;
}

uint64_t pageSize() {
#ifdef _WIN32
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwPageSize*8;
#else
    return sysconf(_SC_PAGESIZE);
#endif
}

Stack::Stack(uint32_t size) : data_(0), size_(size) {
    if (size == 0) { return; }
    data_ = (uint8_t*)malloc(size);
    memset(data_, 0, size);
    assert(data_);
}

Stack::~Stack() {
    if (data_) {
        free(data_);
    }
}

Coroutine::Coroutine() : stack_(0) {
    status_ = Coroutine::RUNNING;
    stackPointer_ = 0;
    event_.reset(new Event);
}

Coroutine::~Coroutine() {
    if (!stack_.begin()) {
    } else if (status_ != Coroutine::EXITED && status_ != Coroutine::NEW) {
        assert(status_ != Coroutine::BLOCKED);
        status_ = Coroutine::DELETED;
        swap();
        assert(status_ == Coroutine::EXITED);
    }
}

void Coroutine::init(std::function<void()> const& func) {
    coro::main();
    event_.reset(new Event);
    func_ = func;
    status_ = Coroutine::NEW;
    assert((((uint8_t*)this)+2*sizeof(uint8_t*))==(uint8_t*)&stackPointer_);

    StackFrame frame;
    memset(&frame, 0, sizeof(frame));
#ifdef _WIN64
    frame.gs0 = (void*)-1;
    frame.gs8 = stack_.end();
    frame.gs16 = stack_.begin();
#elif defined(_WIN32)
    frame.fs0 = (void*)-1;
    frame.fs4 = stack_.end();
    frame.fs8 = stack_.begin();
#endif
    frame.returnAddr = (void*)coroStart;

    stackPointer_ = stack_.end();
    stackPointer_ -= sizeof(frame);
    memcpy(stackPointer_, &frame, sizeof(frame));
}

void Coroutine::yield() {
    assert(coroCurrent == this);
    switch (coroCurrent->status_) {
    case Coroutine::RUNNING: coroCurrent->status_ = Coroutine::RUNNABLE; break;
    case Coroutine::EXITED: break;
    case Coroutine::DELETED: break;
    case Coroutine::RUNNABLE:
    case Coroutine::BLOCKED:
    case Coroutine::NEW:
    default: assert(!"illegal state"); break;
    }
    main()->swap();
}

void Coroutine::block() {
    Ptr<Coroutine> anchor = shared_from_this();
    assert(coroCurrent == this);
    switch (status_) {
    case Coroutine::RUNNING: status_ = Coroutine::BLOCKED; break;
    case Coroutine::EXITED: break;
    case Coroutine::DELETED: break;
    case Coroutine::RUNNABLE:
    case Coroutine::BLOCKED:
    case Coroutine::NEW:
    default: assert(!"illegal state"); break;
    }
    hub()->blocked_++;
    main()->swap();
}

void Coroutine::unblock() {
    switch (status_) {
    case Coroutine::BLOCKED: status_ = Coroutine::RUNNABLE; break;
    case Coroutine::RUNNING:
    case Coroutine::EXITED:
    case Coroutine::DELETED:
    case Coroutine::RUNNABLE:
    case Coroutine::NEW:
    default: assert(!"illegal state"); break;
    }
    hub()->blocked_--;
    hub()->runnable_.push_back(shared_from_this());
}

void Coroutine::wait() {
    Ptr<Coroutine> anchor = shared_from_this();
    assert(coroCurrent == this);
    switch (status_) {
    case Coroutine::RUNNING: status_ = Coroutine::WAITING; break;
    case Coroutine::EXITED: break;
    case Coroutine::DELETED: break;
    case Coroutine::RUNNABLE:
    case Coroutine::BLOCKED:
    case Coroutine::NEW:
    default: assert(!"illegal state"); break;
    }
    hub()->waiting_++;
    main()->swap();
}

void Coroutine::notify() {
    switch (status_) {
    case Coroutine::WAITING: status_ = Coroutine::RUNNABLE; break;
    case Coroutine::RUNNABLE: return;
    case Coroutine::RUNNING:
    case Coroutine::EXITED:
    case Coroutine::DELETED:
    case Coroutine::NEW:
    default: assert(!"illegal state"); break;
    }
    hub()->waiting_--;
    hub()->runnable_.push_back(shared_from_this());
}

void Coroutine::swap() {
    Coroutine* current = coroCurrent;
    switch (status_) {
    case Coroutine::DELETED: break;
    case Coroutine::RUNNABLE: status_ = Coroutine::RUNNING; break;
    case Coroutine::NEW: status_ = Coroutine::RUNNING; break;
    case Coroutine::BLOCKED: status_ = Coroutine::RUNNING; break;
    case Coroutine::RUNNING: return;
    case Coroutine::EXITED: assert(!"coroutine is dead"); break;
    default: assert(!"illegal state"); break;
    }
    coroCurrent = this;
    coroSwapContext(current, this);
    switch (coroCurrent->status_) {
    case Coroutine::DELETED: if (!coroCurrent->isMain()) { throw ExitException(); } break;
    case Coroutine::RUNNING: break;
    case Coroutine::RUNNABLE:
    case Coroutine::BLOCKED:
    case Coroutine::NEW:
    case Coroutine::EXITED:
    default: assert(!"illegal state"); break;
    }
}

void Coroutine::start() throw() {
    try {
        func_();
        exit();
    } catch(ExitException const&) {
        exit();
    } catch(...) {
        assert(!"error: coroutine killed by exception");
    }
    assert(!"error: unreachable");
}

void Coroutine::exit() {
    assert(coroCurrent == this);
    switch (status_) {
    case Coroutine::DELETED: break;
    case Coroutine::RUNNING: status_ = Coroutine::EXITED; break;
    case Coroutine::EXITED:
    case Coroutine::RUNNABLE:
    case Coroutine::NEW:
    default: assert(!"illegal state"); break;
    }
    event_->notifyAll();
    main()->swap();
    assert(!"error: coroutine is dead");
}

void Coroutine::join() {
    assert(current().get() != this);
    while (status_ != Coroutine::EXITED) {
        event_->wait();
    }
}

Ptr<Coroutine> current() {
    return coroCurrent->shared_from_this();
}

Ptr<Coroutine> main() {
    static Ptr<Coroutine> main;
    if (!main) {
        main.reset(new Coroutine);
        registerSignalHandlers();
    }
    return main;
}

void yield() {
    current()->yield();
}

void sleep(Time const& time) {
    hub()->timeoutIs(Timeout(time, current()));
    current()->wait();
}

}