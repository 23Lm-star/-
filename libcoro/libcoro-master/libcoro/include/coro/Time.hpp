

#pragma once

#include "coro/Common.hpp"

namespace coro {

class Time {
public:
    Time() : Time(0) {}
    static Time sec(double sec) { return Time((int64_t)(sec*1000000)); }
    static Time millisec(int64_t ms) { return Time(ms*1000); }
    static Time microsec(int64_t us) { return Time(us); }
    static Time now();

    bool operator<(Time const& rhs) const { return microsec_ < rhs.microsec_; }
    bool operator>(Time const& rhs) const { return microsec_ > rhs.microsec_; }
    bool operator==(Time const& rhs) const { return microsec_ == rhs.microsec_; }
    bool operator<=(Time const& rhs) const { return !operator>(rhs); }
    bool operator>=(Time const& rhs) const { return !operator<(rhs); }
    bool operator!=(Time const& rhs) const { return !operator==(rhs); }
    Time operator-(Time const& rhs) const { return Time(microsec_ - rhs.microsec_); }
    Time operator+(Time const& rhs) const { return Time(microsec_ + rhs.microsec_); }
    Time& operator+=(Time const& rhs) { microsec_ += rhs.microsec_; return *this; }
    Time& operator-=(Time const& rhs) { microsec_ -= rhs.microsec_; return *this; }

#ifndef _WIN32
    struct timespec timespec() const;
#endif
    int64_t millisec() const { return microsec_/1000; }
    int64_t microsec() const { return microsec_; }
    double sec() const { return (double)microsec_/1000000.; }
private:
    Time(int64_t microsec) : microsec_(microsec) {}
    int64_t microsec_;
};

}
