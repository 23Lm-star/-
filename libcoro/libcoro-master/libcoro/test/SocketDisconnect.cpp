
#include <coro/Common.hpp>
#include <coro/coro.hpp>

auto msg = std::string("hello world\n");

coro::Ptr<coro::Socket> newServer() {
    auto ls = std::make_shared<coro::Socket>();
    ls->setsockopt(SOL_SOCKET, SO_REUSEADDR, 1);
    ls->bind(coro::SocketAddr("127.0.0.1", 9090));
    ls->listen(10);

    auto sd = ls->accept();
    return sd;
}

coro::Ptr<coro::Socket> newClient() {
    auto sd = std::make_shared<coro::Socket>();
    sd->connect(coro::SocketAddr("127.0.0.1", 9090));
    return sd;
}

void testReadDisconnect() {
    std::cout << "  Test 1: Client disconnects while server is reading" << std::endl;

    auto server = coro::start([]{
        char buf[1024] = { 0 };
        std::cout << "    [Server] Waiting for client data..." << std::endl;
        auto sd = newServer();
        sd->readAll(buf, msg.length());
        std::cout << "    [Server] Received first chunk, trying to read second..." << std::endl;
        try {
            sd->readAll(buf, msg.length());
            assert(!"Failed: exception should have been thrown");
        } catch (coro::SocketCloseException const&) {
            std::cout << "    [Server] Correctly caught connection close exception" << std::endl;
        }
    });

    auto client = coro::start([]{
        auto sd = newClient();
        std::cout << "    [Client] Sending data then disconnecting immediately..." << std::endl;
        sd->writeAll(msg.c_str(), msg.length());
    });
    coro::run();
}

void testWriteDisconnect() {
    std::cout << "  Test 2: Server disconnects while client is writing" << std::endl;

    auto server = coro::start([]{
        char buf[1024] = { 0 };
        auto sd = newServer();
        std::cout << "    [Server] Receiving data then closing connection immediately..." << std::endl;
        sd->readAll(buf, msg.length());
    });

    auto client = coro::start([]{
        auto sd = newClient();
        sd->writeAll(msg.c_str(), msg.length());
        std::cout << "    [Client] Sent data, waiting for server response..." << std::endl;
        try {
            for (;;) {
                sd->writeAll(msg.c_str(), msg.length());
            }
        } catch (coro::SocketCloseException const&) {
            std::cout << "    [Client] Server closed connection, correctly caught exception" << std::endl;
        }
    });
    coro::run();
}

int main() {
    std::cout << "========== Socket Disconnect Test ==========" << std::endl;
    std::cout << "Testing SocketCloseException handling" << std::endl;
    std::cout << "---" << std::endl;

    testReadDisconnect();

    std::cout << "---" << std::endl;

    testWriteDisconnect();

    std::cout << "---" << std::endl;
    std::cout << "Socket disconnect test completed" << std::endl;
    return 0;
}