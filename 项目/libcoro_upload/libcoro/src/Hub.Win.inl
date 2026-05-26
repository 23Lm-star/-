
namespace coro {

void Hub::poll() {
    size_t tasks = runnable_.size()+timeout_.size();
    DWORD timeout = 0;
    if (!timeout_.empty() && runnable_.empty()) {
        auto const diff = timeout_.top().time()-Time::now();
        if (diff > Time::sec(0)) {
            timeout = DWORD(diff.millisec());
        }
    }
    if (tasks <= 0) {
        timeout = INFINITE;
    }
    if (timeout == 0 && blocked_ == 0) {
        return;
    }
    SetLastError(ERROR_SUCCESS);
    ULONG_PTR udata = 0;
    Overlapped* op = 0;
    OVERLAPPED** evt = (OVERLAPPED**)&op;
    DWORD bytes = 0;
    BOOL ret = GetQueuedCompletionStatus(handle_, (LPDWORD)&bytes, &udata, evt, timeout);
    if (!op) { return; }
    if (ret) {
        op->bytes = bytes;
        op->error = ERROR_SUCCESS;
    } else {
        op->bytes = 0;
        op->error = GetLastError();
    }
    auto const coro = (Coroutine*)op->coroutine;
    assert(coro->status()!=Coroutine::EXITED);
    coro->unblock();
}

}