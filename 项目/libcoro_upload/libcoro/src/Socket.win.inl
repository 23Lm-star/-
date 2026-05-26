
namespace coro {

void Socket::connect(SocketAddr const& addr) {
    DWORD code = SIO_GET_EXTENSION_FUNCTION_POINTER;
    GUID guid = WSAID_CONNECTEX;
    LPFN_CONNECTEX ConnectEx = 0;
    DWORD bytes = 0;
    DWORD len = sizeof(ConnectEx);
    WSAIoctl(sd_, code, &guid, sizeof(guid), &ConnectEx, len, &bytes, 0, 0);

    struct sockaddr_in sin{0};
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = 0;
    if (::bind(sd_, (struct sockaddr*)&sin, sizeof(sin)) != 0) {
        throw SystemError();
    }

    Overlapped op{0};
    OVERLAPPED* evt = &op.overlapped;
    op.coroutine = current().get();

    sin = addr.sockaddr();
    if (!ConnectEx(sd_, (struct sockaddr*)&sin, sizeof(sin), 0, 0, 0, evt)) {
        if (ERROR_IO_PENDING != GetLastError()) {
            throw SystemError();
        }
    }
    current()->block();
    if (ERROR_SUCCESS != op.error) {
        throw SystemError(op.error);
    }

    if (::setsockopt(sd_, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0)) {
         throw SystemError();
    }
}

SocketHandle Socket::acceptRaw() {
    SOCKET ls = sd_;
    SOCKET sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sd < 0) {
        throw SystemError();
    }

    DWORD code = SIO_GET_EXTENSION_FUNCTION_POINTER;
    GUID guid = WSAID_ACCEPTEX;
    LPFN_ACCEPTEX AcceptEx = 0;
    DWORD bytes = 0;
    DWORD len = sizeof(AcceptEx);
    WSAIoctl(sd, code, &guid, sizeof(guid), &AcceptEx, len, &bytes, 0, 0);

    Overlapped op{0};
    op.coroutine = current().get();
    OVERLAPPED* evt = &op.overlapped;

    char buffer[(sizeof(struct sockaddr_in) + 16) * 2] = {0};
    DWORD socklen = sizeof(struct sockaddr_in)+16;
    DWORD read = 0;
    if (!AcceptEx(ls, sd, buffer, 0, socklen, socklen, &read, evt)) {
        if (ERROR_IO_PENDING != GetLastError()) {
            throw SystemError();
        }
    }
    current()->block();
    if (ERROR_SUCCESS != op.error) {
        throw SystemError(op.error);
    }

    char const* opt = (char const*)&ls;
    int const optlen = int(sizeof(ls));
    if (::setsockopt(sd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, opt, optlen)) {
        throw SystemError();
    }

    return sd;
}

bool isSocketCloseError(DWORD error) {
    switch (error) {
    case ERROR_NETNAME_DELETED:
    case ERROR_CONNECTION_ABORTED:
    case WSAENETRESET:
    case WSAECONNABORTED:
    case WSAECONNRESET:
        return true;
    default:
        return false;
    }
}

ssize_t Socket::read(char* buf, size_t len, int flags) {
    WSABUF wsabuf = { ULONG(len), buf };
    Overlapped op{0};
    op.coroutine = current().get();
    OVERLAPPED* evt = &op.overlapped;
    DWORD flg = flags;

    if (sd_ == -1) {
        throw SocketCloseException();
    }

    if(WSARecv(sd_, &wsabuf, 1, NULL, &flg, evt, NULL)) {
        if (isSocketCloseError(GetLastError())) {
            throw SocketCloseException();
        } else if (ERROR_IO_PENDING != GetLastError()) {
            throw SystemError();
        }
    }
    current()->block();
    if (ERROR_SUCCESS != op.error) {
        if (isSocketCloseError(op.error)) {
            throw SocketCloseException();
        } else {
            throw SystemError(op.error);
        }
    }
    assert(op.bytes >= 0);
    return op.bytes;
}

ssize_t Socket::write(char const* buf, size_t len, int flags) {
    WSABUF wsabuf = { ULONG(len), LPSTR(buf) };
    Overlapped op{0};
    op.coroutine = current().get();
    OVERLAPPED* evt = &op.overlapped;

    if (sd_ == -1) {
        throw SocketCloseException();
    }

    if(WSASend(sd_, &wsabuf, 1, NULL, flags, evt, NULL)) {
        if (isSocketCloseError(GetLastError())) {
            throw SocketCloseException();
        } else if (ERROR_IO_PENDING != GetLastError()) {
            throw SystemError();
        }
    }
    current()->block();
    if (ERROR_SUCCESS != op.error) {
        if (isSocketCloseError(op.error)) {
            throw SocketCloseException();
        } else {
            throw SystemError(op.error);
        }
    }
    assert(op.bytes >= 0);
    return op.bytes;
}

}