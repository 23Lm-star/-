
#include "coro/Common.hpp"
#include "coro/Socket.hpp"
#include "coro/Coroutine.hpp"
#include "coro/Hub.hpp"
#include "coro/Error.hpp"

#ifdef __APPLE__
#include "Socket.osx.inl"
#elif defined(_WIN32)
#include "Socket.win.inl"
#else
#include "Socket.linux.inl"
#endif

namespace coro {

struct in_addr SocketAddr::inaddr() const {
    struct in_addr in{0};
    if (host().empty()) {
        return in;
    }
    if (inet_pton(AF_INET, (char*)host().c_str(), &in) == 1) {
        return in;
    }
    struct addrinfo* res = 0;
    auto ret = getaddrinfo(host().c_str(), 0, 0, &res);
    if (ret) {
        throw SystemError(gai_strerrorA(ret));
    }
    for(struct addrinfo* addr = res; addr; addr = addr->ai_next) {
        struct sockaddr_in* sin = (struct sockaddr_in*)addr->ai_addr;
        if (sin->sin_addr.s_addr) {
            in = sin->sin_addr;
            freeaddrinfo(res);
            return in;
        }
    }
    freeaddrinfo(res);
    assert(!"no addresses found");
    return in;
}

struct sockaddr_in SocketAddr::sockaddr() const {
    struct sockaddr_in sin{0};
    sin.sin_family = AF_INET;
    sin.sin_addr = inaddr();
    sin.sin_port = htons(port());
    return sin;
}

Socket::Socket(int type, int protocol) : sd_(CORO_INVALID_SOCKET) {
    hub();
    sd_ = socket(AF_INET, type, protocol);
    if(sd_<0) {
        throw SystemError();
    }
#ifdef _WIN32
    if(!CreateIoCompletionPort((HANDLE)sd_, hub()->handle(), 0, 0)) {
        throw SystemError();
    }
#else
    setsockopt(SOL_SOCKET, SO_NOSIGPIPE, true);
#endif
}

Socket::Socket(SocketHandle sd, char const* ) : sd_(sd) {
#ifdef _WIN32
    if(!CreateIoCompletionPort((HANDLE)sd_, hub()->handle(), 0, 0)) {
        throw SystemError();
    }
#else
    setsockopt(SOL_SOCKET, SO_NOSIGPIPE, true);
#endif
}

Ptr<Socket> Socket::accept() {
    return Ptr<Socket>(new Socket(acceptRaw(), ""));
}

void Socket::bind(SocketAddr const& addr) {
    struct sockaddr_in sin = addr.sockaddr();
    if (::bind(sd_, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
        throw SystemError();
    }
}

void Socket::listen(int backlog) {
    if (::listen(sd_, backlog)) {
        throw SystemError();
    }
}

void Socket::setsockopt(int level, int option, int value) {
    if (::setsockopt(sd_, level, option, (char*)&value, sizeof(value))) {
        throw SystemError();
    }
}

void Socket::writeAll(char const* buf, size_t len, int flags) {
    while(len > 0) {
        ssize_t bytes = write(buf, len, flags);
        if (bytes == 0) {
            throw SocketCloseException();
        } else if (bytes > 0) {
            buf += bytes;
            len -= bytes;
        } else {
            assert(!"unexpected negative byte value");
        }
    }
}

void Socket::readAll(char* buf, size_t len, int flags) {
    while(len > 0) {
        ssize_t bytes = read(buf, len, flags);
        if (bytes == 0) {
            throw SocketCloseException();
        } else if (bytes > 0) {
            buf += bytes;
            len -= bytes;
        } else {
            assert(!"unexpected negative byte value");
        }
    }
}

void Socket::shutdown(int how) {
    ::shutdown(sd_, how);
}

void Socket::close() {
    if (sd_ == CORO_INVALID_SOCKET) {
        return;
    }
#ifdef _WIN32
    ::closesocket(sd_);
#else
    ::close(sd_);
#endif
    sd_ = CORO_INVALID_SOCKET;
}

}