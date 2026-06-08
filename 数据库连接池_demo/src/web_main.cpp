#include <iostream>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <cstring>
#include <getopt.h>
#include <csignal>
#include <execinfo.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "DBConnection.h"
#include "web/HttpServer.h"
#include "web/MysqlApiHandler.h"
#include "web/RedisApiHandler.h"
#include "web/ClusterMonitor.h"
#include "auth/AuthHandler.h"
#include "auth/SessionManager.h"

// 信号处理函数
void signalHandler(int sig) {
    std::cerr << "[FATAL] Caught signal " << sig << ", printing stack trace..." << std::endl;
    void* buffer[100];
    int n = backtrace(buffer, 100);
    char** strings = backtrace_symbols(buffer, n);
    for (int i = 0; i < n; i++) {
        std::cerr << "  " << strings[i] << std::endl;
    }
    free(strings);
    _exit(1);
}

// 打印使用方法
void printUsage(const char* programName) {
    std::cout << "使用方法: " << programName << " [选项]\n";
    std::cout << "\n选项:\n";
    std::cout << "  --port <端口>    指定服务器监听端口 (默认: 8080)\n";
    std::cout << "  --help          显示帮助信息\n";
    std::cout << "\n示例:\n";
    std::cout << "  " << programName << "                  # 使用默认端口8080\n";
    std::cout << "  " << programName << " --port 8081     # 使用端口8081\n";
}

int main(int argc, char* argv[]) {
    // 注册信号处理器
    signal(SIGSEGV, signalHandler);
    signal(SIGABRT, signalHandler);
    
    int port = 8080;

    // 解析命令行参数
    static struct option longOptions[] = {
        {"port", required_argument, 0, 'p'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int optionIndex = 0;
    int c;

    while ((c = getopt_long(argc, argv, "p:h", longOptions, &optionIndex)) != -1) {
        switch (c) {
            case 'p':
                port = std::stoi(optarg);
                if (port <= 0 || port > 65535) {
                    std::cerr << "错误: 无效的端口号\n";
                    return 1;
                }
                break;
            case 'h':
                printUsage(argv[0]);
                return 0;
            default:
                printUsage(argv[0]);
                return 1;
        }
    }

    std::cout << "========================================\n";
    std::cout << "    数据库连接池 - Web服务模式\n";
    std::cout << "    集群节点 - 端口: " << port << "\n";
    std::cout << "========================================\n\n";

    std::cout << "正在初始化MySQL连接池...\n";
    bool mysqlInit = DBConnection::initMysqlPool(10, 20, 100);  // 初始10个，最大100个
    std::cout << "MySQL连接池初始化: " << (mysqlInit ? "成功" : "失败") << "\n";

    std::cout << "正在初始化Redis连接池...\n";
    bool redisInit = DBConnection::initRedisPool(10, 20, 100);  // 初始10个，最大100个
    std::cout << "Redis连接池初始化: " << (redisInit ? "成功" : "失败") << "\n";

    // 初始化Redis会话存储（用于集群环境）
    std::cout << "正在初始化Redis会话存储...\n";
    bool sessionInit = Auth::SessionManager::instance().initRedis("192.168.232.160", 6379);
    std::cout << "Redis会话存储初始化: " << (sessionInit ? "成功" : "失败") << "\n";

    std::cout << "正在初始化用户认证模块...\n";
    bool authInit = Auth::AuthHandler::init();
    std::cout << "用户认证模块初始化: " << (authInit ? "成功" : "失败") << "\n";

    std::cout << "正在初始化集群监控模块...\n";
    std::cout << "[DEBUG] 调用 ClusterMonitor::init()\n";
    WebServer::ClusterMonitor::instance().init(port, "192.168.232.160", 6379);
    std::cout << "[DEBUG] ClusterMonitor::init() 完成\n";
    
    std::cout << "[DEBUG] 调用 ClusterMonitor::loadInitialConfig()\n";
    WebServer::ClusterMonitor::instance().loadInitialConfig();
    std::cout << "[DEBUG] ClusterMonitor::loadInitialConfig() 完成\n";
    
    std::cout << "[DEBUG] 调用 ClusterMonitor::startStatusReporter()\n";
    WebServer::ClusterMonitor::instance().startStatusReporter();
    std::cout << "[DEBUG] ClusterMonitor::startStatusReporter() 完成\n";
    
    std::cout << "集群监控模块初始化: 成功\n";

    if (!mysqlInit) {
        std::cout << "警告: MySQL连接池初始化失败\n";
    }
    if (!redisInit) {
        std::cout << "警告: Redis连接池初始化失败\n";
    }
    if (!sessionInit) {
        std::cout << "警告: Redis会话存储初始化失败\n";
    }

    std::cout << "[DEBUG] 创建HttpServer对象...\n";
    WebServer::HttpServer server(port);
    std::cout << "[DEBUG] HttpServer对象创建成功\n";

    char cwd[1024];
    std::cout << "[DEBUG] 调用getcwd...\n";
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        std::string basePath = std::string(cwd) + "/web/";
        server.serveStatic("/", basePath + "index.html");
        server.serveStatic("/monitor", basePath + "monitor.html");
        server.serveStatic("/test", basePath + "test_pool.html");
        server.serveStatic("/login.html", basePath + "login.html");
        std::cout << "✅ 监控页面: http://172.22.136.134:" << port << "/monitor\n";
        std::cout << "✅ 测试页面: http://172.22.136.134:" << port << "/test\n";
    } else {
        server.serveStatic("/", "./web/index.html");
        server.serveStatic("/monitor", "./web/monitor.html");
        server.serveStatic("/test", "./web/test_pool.html");
        server.serveStatic("/login.html", "./web/login.html");
    }

    // 健康检查接口（用于负载均衡器）
    int healthPort = port;  // 保存端口值供lambda使用
    server.get("/health", [healthPort](const WebServer::HttpRequest& req) {
        WebServer::HttpResponse res;
        res.statusCode = 200;
        res.contentType = "application/json";

        bool mysqlOk = DBConnection::initMysqlPool();
        bool redisOk = DBConnection::initRedisPool();
        bool sessionOk = Auth::SessionManager::instance().isRedisConnected();

        std::string status = (mysqlOk && redisOk && sessionOk) ? "healthy" : "unhealthy";

        res.body = "{"
            "\"status\": \"" + status + "\","
            "\"port\": " + std::to_string(healthPort) + ","
            "\"mysql\": " + std::string(mysqlOk ? "true" : "false") + ","
            "\"redis\": " + std::string(redisOk ? "true" : "false") + ","
            "\"session\": " + std::string(sessionOk ? "true" : "false") +
        "}";

        return res;
    });

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
    server.get("/api/mysql/stressTest", WebServer::MysqlApiHandler::stressTest);
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
    server.get("/api/redis/stressTest", WebServer::RedisApiHandler::stressTest);

    server.get("/api/cluster/poolStatus", [](const WebServer::HttpRequest& req) {
        WebServer::HttpResponse res;
        res.statusCode = 200;
        res.contentType = "application/json";
        
        auto clusterStatus = WebServer::ClusterMonitor::instance().getClusterStatus();
        
        res.body = "{"
            "\"success\": true,"
            "\"total_nodes\": " + std::to_string(clusterStatus.total_nodes) + ","
            "\"online_nodes\": " + std::to_string(clusterStatus.online_nodes) + ","
            "\"offline_nodes\": " + std::to_string(clusterStatus.offline_nodes) + ","
            "\"total_mysql_active\": " + std::to_string(clusterStatus.total_mysql_active) + ","
            "\"total_mysql_idle\": " + std::to_string(clusterStatus.total_mysql_idle) + ","
            "\"total_mysql_max\": " + std::to_string(clusterStatus.total_mysql_max) + ","
            "\"total_redis_active\": " + std::to_string(clusterStatus.total_redis_active) + ","
            "\"total_redis_idle\": " + std::to_string(clusterStatus.total_redis_idle) + ","
            "\"total_redis_max\": " + std::to_string(clusterStatus.total_redis_max) + ","
            "\"nodes\": {";
        
        for (const auto& pair : clusterStatus.nodes) {
            const auto& node = pair.second;
            res.body += "\"" + std::to_string(node.port) + "\": {"
                "\"port\": " + std::to_string(node.port) + ","
                "\"online\": " + (node.online ? "true" : "false") + ","
                "\"mysql_active\": " + std::to_string(node.mysql_active) + ","
                "\"mysql_idle\": " + std::to_string(node.mysql_idle) + ","
                "\"mysql_total\": " + std::to_string(node.mysql_total) + ","
                "\"mysql_maxActive\": " + std::to_string(node.mysql_maxActive) + ","
                "\"redis_active\": " + std::to_string(node.redis_active) + ","
                "\"redis_idle\": " + std::to_string(node.redis_idle) + ","
                "\"redis_total\": " + std::to_string(node.redis_total) + ","
                "\"redis_maxActive\": " + std::to_string(node.redis_maxActive) + ","
                "\"timestamp\": \"" + node.timestamp + "\""
                "},";
        }
        
        if (!clusterStatus.nodes.empty()) {
            res.body.pop_back();
        }
        
        res.body += "}}";
        
        return res;
    });

    server.put("/api/cluster/updatePoolConfig", [](const WebServer::HttpRequest& req) {
        WebServer::HttpResponse res;
        res.contentType = "application/json";
        
        int maxActive = 20, minIdlePercent = 10, maxWaitMs = 3000, idleTimeoutMs = 30000;
        std::string poolType = "mysql";
        
        std::cout << "[DEBUG] 收到配置更新请求，参数个数: " << req.params.size() << std::endl;
        for (const auto& param : req.params) {
            std::cout << "[DEBUG] 参数: " << param.first << " = " << param.second << std::endl;
            if (param.first == "maxActive") maxActive = std::stoi(param.second);
            if (param.first == "minIdlePercent") minIdlePercent = std::stoi(param.second);
            if (param.first == "maxWaitMs") maxWaitMs = std::stoi(param.second);
            if (param.first == "idleTimeoutMs") idleTimeoutMs = std::stoi(param.second);
            if (param.first == "poolType") poolType = param.second;
        }
        
        std::cout << "[INFO] 准备更新配置 - poolType: " << poolType 
                  << ", maxActive: " << maxActive 
                  << ", minIdlePercent: " << minIdlePercent << std::endl;
        
        bool success = true;
        if (poolType == "mysql") {
            std::cout << "[INFO] 应用MySQL配置到当前节点" << std::endl;
            success = DBConnection::setMysqlPoolMaxActive(maxActive);
            DBConnection::setMysqlPoolMinIdlePercent(minIdlePercent);
            DBConnection::setMysqlPoolMaxWaitMs(maxWaitMs);
            DBConnection::setMysqlPoolIdleTimeoutMs(idleTimeoutMs);
            std::cout << "[INFO] MySQL配置应用完成，success: " << (success ? "true" : "false") << std::endl;
        } else {
            std::cout << "[INFO] 应用Redis配置到当前节点" << std::endl;
            success = DBConnection::setRedisPoolMaxActive(maxActive);
            DBConnection::setRedisPoolMinIdlePercent(minIdlePercent);
            DBConnection::setRedisPoolMaxWaitMs(maxWaitMs);
            DBConnection::setRedisPoolIdleTimeoutMs(idleTimeoutMs);
            std::cout << "[INFO] Redis配置应用完成，success: " << (success ? "true" : "false") << std::endl;
        }
        
        // 广播配置更新到集群中的其他节点
        std::cout << "[INFO] 开始广播配置到集群" << std::endl;
        WebServer::ClusterMonitor::instance().broadcastConfigUpdate(poolType, maxActive, minIdlePercent, maxWaitMs, idleTimeoutMs);
        
        res.statusCode = success ? 200 : 500;
        res.body = "{\"success\": " + std::string(success ? "true" : "false") + ", \"message\": \"配置已更新并广播到集群所有节点\"}";
        return res;
    });

    std::cout << "正在启动Web服务器...\n";
    if (server.start()) {
        std::cout << "✅ Web服务已启动，监听端口: " << port << "\n";
        std::cout << "🌐 访问地址: http://172.22.136.134:" << port << "\n";
        std::cout << "🔐 登录页面: http://172.22.136.134:" << port << "/login.html\n";
        std::cout << "💚 健康检查: http://172.22.136.134:" << port << "/health\n";
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
