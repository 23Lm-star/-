
#pragma once

#include "coro/Common.hpp"

namespace coro {

class SocketAddr {
public:
    SocketAddr(std::string const& host, short port) : host_(host), port_(port) {}
    struct sockaddr_in sockaddr() const;
    struct in_addr inaddr() const;
    std::string const& host() const { return host_; }
    short port() const { return port_; }

private:
    std::string host_;
    short port_;
};

class SocketCloseException {
public:
    SocketCloseException() {}
};

#ifdef _WIN32
typedef SOCKET SocketHandle;
#else
typedef int SocketHandle;
#endif

class Socket {
public:
    Socket(int type=SOCK_STREAM, int protocol=IPPROTO_TCP);
    virtual ~Socket() { close(); }
    void bind(SocketAddr const& addr);
    void listen(int backlog);
    void shutdown(int how);
    void close();
    virtual void connect(SocketAddr const& addr);
    virtual Ptr<Socket> accept();
    virtual ssize_t write(char const* buf, size_t len, int flags=0);
    virtual ssize_t read(char* buf, size_t len, int flags=0);
    void writeAll(char const* buf, size_t len, int flags=0);
    void readAll(char* buf, size_t len, int flags=0);
    int fileno() const;
    void setsockopt(int level, int option, int value);

protected:
    Socket(SocketHandle sd, char const* bogus);
    SocketHandle acceptRaw();

private:
    SocketHandle sd_;
};

}