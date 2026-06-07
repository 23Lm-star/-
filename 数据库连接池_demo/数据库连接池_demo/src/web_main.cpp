#include <iostream>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <cstring>
#include "DBConnection.h"
#include "web/HttpServer.h"
#include "web/MysqlApiHandler.h"
#include "web/RedisApiHandler.h"
#include "auth/AuthHandler.h"

int main(int argc, char* argv[]) {
    std::cout << "========================================\n";
    std::cout << "    数据库连接池 - Web服务模式\n";
    std::cout << "========================================\n\n";
    
    std::cout << "正在初始化MySQL连接池...\n";
    bool mysqlInit = DBConnection::initMysqlPool();
    std::cout << "MySQL连接池初始化: " << (mysqlInit ? "成功" : "失败") << "\n";
    
    std::cout << "正在初始化Redis连接池...\n";
    bool redisInit = DBConnection::initRedisPool();
    std::cout << "Redis连接池初始化: " << (redisInit ? "成功" : "失败") << "\n";
    
    std::cout << "正在初始化用户认证模块...\n";
    bool authInit = Auth::AuthHandler::init();
    std::cout << "用户认证模块初始化: " << (authInit ? "成功" : "失败") << "\n";
    
    if (!mysqlInit) {
        std::cout << "警告: MySQL连接池初始化失败\n";
    }
    if (!redisInit) {
        std::cout << "警告: Redis连接池初始化失败\n";
    }
    
    int port = 8080;
    WebServer::HttpServer server(port);
    
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        std::string basePath = std::string(cwd) + "/web/";
        server.serveStatic("/", basePath + "index.html");
        server.serveStatic("/monitor", basePath + "monitor.html");
        server.serveStatic("/login.html", basePath + "login.html");
        std::cout << "✅ 监控页面: http://172.22.136.134:" << port << "/monitor\n";
    } else {
        server.serveStatic("/", "./web/index.html");
        server.serveStatic("/monitor", "./web/monitor.html");
        server.serveStatic("/login.html", "./web/login.html");
    }
    
    // 用户认证API（公开路径，无需拦截）
    server.post("/api/login", WebServer::MysqlApiHandler::login);
    server.post("/api/logout", WebServer::MysqlApiHandler::logout);
    server.get("/api/current-user", WebServer::MysqlApiHandler::currentUser);
    
    // 用户注册API（公开路径）
    server.post("/api/register", WebServer::MysqlApiHandler::registerUser);
    
    // 用户管理API（需要超级管理员权限）
    server.get("/api/users", WebServer::MysqlApiHandler::getUsers);
    server.get("/api/users/pending", WebServer::MysqlApiHandler::getPendingUsers);
    server.post("/api/users/approve", WebServer::MysqlApiHandler::approveUser);
    server.post("/api/users/reject", WebServer::MysqlApiHandler::rejectUser);
    server.del("/api/users", WebServer::MysqlApiHandler::deleteUser);
    server.put("/api/users/password", WebServer::MysqlApiHandler::changePassword);
    
    server.get("/api/mysql/tables", WebServer::MysqlApiHandler::getTables);
    server.get("/api/mysql/table", WebServer::MysqlApiHandler::getTableData);
    server.get("/api/mysql/structure", WebServer::MysqlApiHandler::getTableStructure);
    server.post("/api/mysql/insert", WebServer::MysqlApiHandler::insertRow);
    server.put("/api/mysql/update", WebServer::MysqlApiHandler::updateRow);
    server.del("/api/mysql/delete", WebServer::MysqlApiHandler::deleteRow);
    server.post("/api/mysql/createTable", WebServer::MysqlApiHandler::createTable);
    server.del("/api/mysql/dropTable", WebServer::MysqlApiHandler::dropTable);
    server.post("/api/mysql/addColumn", WebServer::MysqlApiHandler::addColumn);
    server.del("/api/mysql/dropColumn", WebServer::MysqlApiHandler::dropColumn);
    server.post("/api/mysql/setPrimaryKey", WebServer::MysqlApiHandler::setPrimaryKey);
    server.put("/api/mysql/modifyColumn", WebServer::MysqlApiHandler::modifyColumn);
    server.get("/api/mysql/poolStatus", WebServer::MysqlApiHandler::poolStatus);
    server.put("/api/mysql/updatePoolConfig", WebServer::MysqlApiHandler::updatePoolConfig);
    
    server.get("/api/redis/get", WebServer::RedisApiHandler::get);
    server.post("/api/redis/set", WebServer::RedisApiHandler::set);
    server.get("/api/redis/hget", WebServer::RedisApiHandler::hget);
    server.post("/api/redis/hset", WebServer::RedisApiHandler::hset);
    server.get("/api/redis/hgetall", WebServer::RedisApiHandler::hgetall);
    server.del("/api/redis/del", WebServer::RedisApiHandler::del);
    server.del("/api/redis/hdel", WebServer::RedisApiHandler::hdel);
    server.get("/api/redis/poolStatus", WebServer::RedisApiHandler::poolStatus);
    server.put("/api/redis/updatePoolConfig", WebServer::RedisApiHandler::updatePoolConfig);
    
    std::cout << "正在启动Web服务器...\n";
    if (server.start()) {
        std::cout << "✅ Web服务已启动，监听端口: " << port << "\n";
        std::cout << "🌐 访问地址: http://172.22.136.134:" << port << "\n";
        std::cout << "🔐 登录页面: http://172.22.136.134:" << port << "/login.html\n";
        std::cout << "\n按 Ctrl+C 停止服务\n";
        
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } else {
        std::cerr << "❌ Web服务启动失败\n";
        return 1;
    }
    
    return 0;
}