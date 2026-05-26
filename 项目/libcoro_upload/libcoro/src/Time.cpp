
#include "coro/Common.hpp"
#include "coro/Time.hpp"

namespace coro {

Time Time::now() {
#ifdef _WIN32
    LARGE_INTEGER count{0};
    QueryPerformanceCounter(&count);
    LARGE_INTEGER freq{0};
    QueryPerformanceFrequency(&freq);
    return Time::microsec(1000000 * count.QuadPart / freq.QuadPart);
#else
    struct timeval ts{0};
    gettimeofday(&ts, 0);
    return Time::sec(ts.tv_sec)+Time::microsec(ts.tv_usec);
#endif
}

#ifndef _WIN32
struct timespec Time::timespec() const {
    struct timespec out{0};
    out.tv_sec = microsec_/1000000;
    out.tv_nsec = (microsec_%1000000)*1000;
    return out;
}
#endif

}