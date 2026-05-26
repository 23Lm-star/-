#include <coro/Common.hpp>
#include <coro/coro.hpp>
#include <iostream>

void server() {
    try {
        char buf[1024] = { 0 };
        std::cout << "[Server] Creating listening socket..." << std::endl;
        auto ls = std::make_shared<coro::Socket>();
        ls->setsockopt(SOL_SOCKET, SO_REUSEADDR, 1);
        ls->bind(coro::SocketAddr("127.0.0.1", 9090));
        ls->listen(10);
        std::cout << "[Server] Listening on port 9090, waiting for client connection..." << std::endl;

        auto sd = ls->accept();
        std::cout << "[Server] Client connected!" << std::endl;

        ssize_t len = 0;
        int readCount = 0;
        try
        {
            std::cout << "[Server] Starting to receive data..." << std::endl;
            while ((len = sd->read(buf, sizeof(buf))) > 0) {
                readCount++;
                printf("[Server] Received data (#%d, %d bytes): %.*s", readCount, (int)len, (int)len, buf);
                fflush(stdout);
            }
            std::cout << "[Server] Client disconnected, received " << readCount << " data chunks" << std::endl;
        }
        catch (...)
        {
            std::cout << "[Server] Error occurred" << std::endl;
        }
        std::cout << "[Server] Exiting" << std::endl;
    } catch (coro::SystemError const& ex) {
        std::cout << "[Server] System error: " << ex.what() << std::endl;
        exit(1);
    }
}

void client() {
    auto sd = std::make_shared<coro::Socket>();
    auto msg = "hello world\n";
    try {
        std::cout << "[Client] Connecting to server 127.0.0.1:9090..." << std::endl;
        sd->connect(coro::SocketAddr("127.0.0.1", 9090));
        std::cout << "[Client] Connection successful!" << std::endl;

        std::cout << "[Client] Starting to send data (1000 messages)..." << std::endl;
        for (auto i = 0; i < 1000; ++i) {
            sd->writeAll(msg, strlen(msg));
        }
        std::cout << "[Client] Sending completed, sent 1000 messages" << std::endl;
        std::cout << "[Client] Disconnecting" << std::endl;
    } catch (coro::SystemError const& ex) {
        std::cout << "[Client] System error: " << ex.what() << std::endl;
    }
}

int main() {
    std::cout << "========== Socket Communication Test ==========" << std::endl;
    std::cout << "Testing TCP socket: client sends data, server receives" << std::endl;
    std::cout << "---" << std::endl;

    std::cout << "Starting server coroutine..." << std::endl;
    auto cserver = coro::start(server);

    std::cout << "Starting client coroutine..." << std::endl;
    auto cclient = coro::start(client);

    std::cout << "---" << std::endl;
    std::cout << "Starting coroutine scheduler..." << std::endl;
    coro::run();

    std::cout << "---" << std::endl;
    std::cout << "Socket communication test completed" << std::endl;
    return 0;
}