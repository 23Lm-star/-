
namespace coro {

void Socket::connect(SocketAddr const& addr) {
    if (fcntl(sd_, F_SETFL, O_NONBLOCK) < 0) {
        throw SystemError();
    }

    struct sockaddr_in sin = addr.sockaddr();
    int ret = ::connect(sd_, (struct sockaddr*)&sin, sizeof(sin));
    if (ret < 0 && errno != EINPROGRESS) {
        throw SystemError();
    }

    int kqfd = hub()->handle();
    int flags = EV_ADD|EV_ONESHOT|EV_EOF;
    struct kevent ev{0};
    EV_SET(&ev, sd_, EVFILT_WRITE, flags, 0, 0, current().get());
    if (kevent(kqfd, &ev, 1, 0, 0, 0) < 0) {
        throw SystemError();
    }

    current()->block();

    if (::read(sd_, 0, 0) < 0) {
        throw SystemError();
    }
}

int Socket::acceptRaw() {
    int kqfd = hub()->handle();
    int flags = EV_ADD|EV_ONESHOT;
    struct kevent ev{0};
    EV_SET(&ev, sd_, EVFILT_READ, flags, 0, 0, current().get());
    if (kevent(kqfd, &ev, 1, 0, 0, 0) < 0) {
        throw SystemError();
    }
    current()->block();

    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    int sd = ::accept(sd_, (struct sockaddr*)&sin, &len);
    if (sd < 0) {
        throw SystemError();
    }
    if (fcntl(sd, F_SETFL, O_NONBLOCK) < 0) {
        throw SystemError();
    }
    return sd;
}

bool isSocketCloseError(int error) {
    switch (error) {
    case EPIPE:
    case ENETRESET:
    case ECONNABORTED:
    case ECONNRESET:
    case ESHUTDOWN:
        return true;
    default:
        return false;
    }
}

ssize_t Socket::read(char* buf, size_t len, int flags) {
    if (sd_ == CORO_INVALID_SOCKET) {
        throw SocketCloseException();
    }

    ssize_t ret = recv(sd_, buf, len, flags);
    if (ret < 0) {
        if (isSocketCloseError(errno)) {
            throw SocketCloseException();
        } else if (EAGAIN != errno) {
            throw SystemError();
        }
    } else {
        return ret;
    }

    int const kqfd = hub()->handle();
    int const kqflags = EV_ADD|EV_ONESHOT|EV_EOF;
    struct kevent ev{0};
    EV_SET(&ev, sd_, EVFILT_READ, kqflags, 0, 0, current().get());
    if (kevent(kqfd, &ev, 1, 0, 0, 0) < 0) {
        throw SystemError();
    }
    current()->block();

    if (sd_ == CORO_INVALID_SOCKET) {
        throw SocketCloseException();
    }

    ret = recv(sd_, buf, len, flags);
    if (ret < 0) {
        if (isSocketCloseError(errno)) {
            throw SocketCloseException();
        } else {
            throw SystemError();
        }
    }
    assert(ret >= 0);
    return ret;
}

ssize_t Socket::write(char const* buf, size_t len, int flags) {
    if (sd_ == CORO_INVALID_SOCKET) {
        throw SocketCloseException();
    }

    ssize_t ret = send(sd_, buf, len, flags);
    if (ret < 0) {
        if (isSocketCloseError(errno)) {
            throw SocketCloseException();
        } else if (EAGAIN != errno) {
            throw SystemError();
        }
    } else {
        return ret;
    }

    int const kqfd = hub()->handle();
    int const kqflags = EV_ADD|EV_ONESHOT|EV_EOF;
    struct kevent ev{0};
    EV_SET(&ev, sd_, EVFILT_WRITE, kqflags, 0, 0, current().get());
    if (kevent(kqfd, &ev, 1, 0, 0, 0) < 0) {
        throw SystemError();
    }
    current()->block();

    if (sd_ == CORO_INVALID_SOCKET) {
        throw SocketCloseException();
    }

    ret = send(sd_, buf, len, flags);
    if (ret < 0) {
        if (isSocketCloseError(errno)) {
            throw SocketCloseException();
        } else {
            throw SystemError();
        }
    }
    assert(ret >= 0);
    return ret;
}


}