
#pragma once

#include "coro/Common.hpp"

namespace coro {

class SystemError {
public:
    SystemError(int error);
    SystemError(std::string const& msg) : error_(0), msg_(msg) {}
    SystemError();

    int error() const { return error_; }
    std::string const& what() const { return msg_; }
private:
    void operator=(SystemError const&) {}

    int error_;
    std::string const msg_;
};

}