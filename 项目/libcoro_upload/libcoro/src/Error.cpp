
#include "coro/Common.hpp"
#include "coro/Error.hpp"

namespace coro {

#ifdef _WIN32
std::string winErrorMsg(DWORD error) {
    std::string buffer(1024,'0');
    LPSTR buf = (LPSTR)buffer.c_str();
    DWORD flags = FORMAT_MESSAGE_FROM_SYSTEM;
    DWORD len = FormatMessageA(flags, 0, error, 0, buf, (DWORD)buffer.size(), 0);
    buffer.resize(len);
    return buffer;
}
#endif

SystemError::SystemError() :
#ifdef _WIN32
    SystemError(GetLastError()) {
#else
    SystemError(errno) {
#endif

}

SystemError::SystemError(int error) :
#ifdef _WIN32
    error_(error),
    msg_(winErrorMsg(error_)) {
#else
    error_(error),
    msg_(strerror(error_)) {

#endif
}

}