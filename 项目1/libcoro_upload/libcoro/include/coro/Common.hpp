
#ifndef CORO_COMMON_HPP
#define CORO_COMMON_HPP

#ifdef _WIN32
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <windows.h>
#undef ERROR
#undef Ptr
#define ssize_t SSIZE_T
#define SHUT_RD SD_RECEIVE
#define SHUT_RW SD_BOTH
#define SHUT_WR SD_SEND
#endif

#ifdef __APPLE__
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/event.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#endif

#ifdef __linux__
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#include <iostream>
#include <memory>
#include <functional>
#include <cstdint>
#include <unordered_set>
#include <set>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <queue>
#include <vector>
#include <mutex>

#ifndef CORO_STACK_SIZE
#define CORO_STACK_SIZE 1048576
#endif

namespace coro {
class Coroutine;
class Channel;
class Event;
class Hub;
class Selector;
class Socket;

template <typename T>
using Ptr = std::shared_ptr<T>;

template <typename T>
using WeakPtr = std::weak_ptr<T>;

}

#endif