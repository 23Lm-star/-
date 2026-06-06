#include <iostream>
#include <memory>
#include "DBConnection.h"
#include "interactive/Command.h"
#include "interactive/InteractiveShell.h"
#include "interactive/MysqlCommandHandler.h"
#include "interactive/RedisCommandHandler.h"

class SimpleMysqlProvider : public InteractiveShell::IMysqlConnectionProvider {
public:
    std::shared_ptr<MysqlConn> getConnection() override {
        auto conn = new MysqlConn("192.168.232.155", 3306, "root", "123456", "test");
        if (conn->connect()) {
            return std::shared_ptr<MysqlConn>(conn);
        }
        delete conn;
        return nullptr;
    }
};

class SimpleRedisProvider : public InteractiveShell::IRedisConnectionProvider {
public:
    std::shared_ptr<RedisConn> getConnection() override {
        auto conn = new RedisConn("192.168.232.155", 6379, 3000);
        if (conn->connect()) {
            return std::shared_ptr<RedisConn>(conn);
        }
        delete conn;
        return nullptr;
    }
};

int main(int argc, char* argv[]) {
    std::cout << "========================================\n";
    std::cout << "    数据库连接池 - 交互式模式\n";
    std::cout << "========================================\n\n";
    
    bool mysqlInit = DBConnection::initMysqlPool();
    bool redisInit = DBConnection::initRedisPool();
    
    if (!mysqlInit) {
        std::cout << "警告: MySQL连接池初始化失败\n";
    }
    if (!redisInit) {
        std::cout << "警告: Redis连接池初始化失败\n";
    }
    
    InteractiveShell::InteractiveShell shell;
    
    if (mysqlInit) {
        auto mysqlProvider = std::make_shared<SimpleMysqlProvider>();
        auto mysqlHandler = std::make_shared<InteractiveShell::MysqlCommandHandler>(mysqlProvider);
        shell.registerMysqlHandler(mysqlHandler);
    }
    
    if (redisInit) {
        auto redisProvider = std::make_shared<SimpleRedisProvider>();
        auto redisHandler = std::make_shared<InteractiveShell::RedisCommandHandler>(redisProvider);
        shell.registerRedisHandler(redisHandler);
    }
    
    shell.run();
    
    return 0;
}