
namespace coro {

void Hub::poll() {
    size_t tasks = runnable_.size()+timeout_.size();
    struct timespec timeout{0};
    struct kevent event{0};

    if (!timeout_.empty() && runnable_.empty()) {
        auto const diff = timeout_.top().time()-Time::now();
        if (diff > Time::sec(0)) {
            timeout = diff.timespec();
        }
    }
    if (timeout.tv_nsec < 1000 && timeout.tv_sec == 0 && blocked_ == 0) {
        return;
    }

    int res = kevent(handle_, 0, 0, &event, 1, (tasks <= 0 ? 0 : &timeout));
    if (res < 0) {
        throw SystemError();
    } else if (res == 0) {
    } else {
        auto const coro = (Coroutine*)event.udata;
        assert(coro->status()!=Coroutine::EXITED);
        coro->unblock();
    }
}

}