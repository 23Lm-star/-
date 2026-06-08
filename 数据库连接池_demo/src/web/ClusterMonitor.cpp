#define _GNU_SOURCE
#include "ClusterMonitor.h"
#include "../DBConnection.h"
#include "../RedisConn.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <unordered_map>

namespace WebServer {

ClusterMonitor::ClusterMonitor() 
    : m_port(8080), m_redisHost("localhost"), m_redisPort(6379), m_reporterThread(nullptr), m_running(false) {}

ClusterMonitor::~ClusterMonitor() {
    stopStatusReporter();
}

ClusterMonitor& ClusterMonitor::instance() {
    static ClusterMonitor instance;
    return instance;
}

bool ClusterMonitor::init(int port, const std::string& redisHost, int redisPort) {
    m_port = port;
    m_redisHost = redisHost;
    m_redisPort = redisPort;
    
    // 注册当前节点到集群
    registerNode();
    
    return true;
}

void ClusterMonitor::startStatusReporter() {
    if (m_running.load()) return;
    
    std::cout << "[DEBUG] startStatusReporter: setting m_running to true" << std::endl;
    m_running.store(true);
    std::cout << "[DEBUG] startStatusReporter: creating thread" << std::endl;
    m_reporterThread = new std::thread(&ClusterMonitor::reportStatusLoop, this);
    std::cout << "[DEBUG] startStatusReporter: thread created" << std::endl;
}

void ClusterMonitor::stopStatusReporter() {
    m_running.store(false);
    if (m_reporterThread != nullptr) {
        if (m_reporterThread->joinable()) {
            m_reporterThread->join();
        }
        delete m_reporterThread;
        m_reporterThread = nullptr;
    }
    
    // 注销当前节点
    unregisterNode();
}

void ClusterMonitor::registerNode() {
    RedisConn conn(m_redisHost, m_redisPort);
    if (!conn.connect()) {
        std::cerr << "[DEBUG] ClusterMonitor: Failed to connect to Redis for node registration" << std::endl;
        return;
    }
    
    time_t now = time(nullptr);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    // 将节点注册到有序集合，score为注册时间戳
    conn.zadd("cluster:nodes:register", std::to_string(m_port), static_cast<double>(now));
    
    // 设置节点信息
    std::string key = "cluster:node:" + std::to_string(m_port) + ":info";
    conn.hset(key, "port", std::to_string(m_port));
    conn.hset(key, "registered", timestamp);
    conn.hset(key, "status", "registered");
    
    std::cout << "[INFO] 节点已注册到集群: 端口 " << m_port << std::endl;
}

void ClusterMonitor::unregisterNode() {
    RedisConn conn(m_redisHost, m_redisPort);
    if (!conn.connect()) {
        return;
    }
    
    // 从有序集合中移除
    conn.zrem("cluster:nodes:register", std::to_string(m_port));
    
    // 删除节点信息
    std::string key = "cluster:node:" + std::to_string(m_port) + ":info";
    conn.del(key);
    
    std::cout << "[INFO] 节点已从集群注销: 端口 " << m_port << std::endl;
}

std::vector<int> ClusterMonitor::getRegisteredPorts() {
    std::vector<int> ports;
    
    RedisConn conn(m_redisHost, m_redisPort);
    if (!conn.connect()) {
        return ports;
    }
    
    // 从有序集合中获取所有注册的节点
    std::vector<std::pair<std::string, double>> members = conn.zrange("cluster:nodes:register", 0, -1);
    
    for (const auto& member : members) {
        try {
            int port = std::stoi(member.first);
            ports.push_back(port);
        } catch (...) {
            // 忽略无效的端口
        }
    }
    
    return ports;
}

void ClusterMonitor::reportStatusLoop() {
    std::cout << "[DEBUG] reportStatusLoop: started" << std::endl;
    while (m_running.load()) {
        std::cout << "[DEBUG] reportStatusLoop: calling writeLocalStatus()" << std::endl;
        writeLocalStatus();
        std::cout << "[DEBUG] reportStatusLoop: calling checkAndApplyRemoteConfig()" << std::endl;
        // 检查远程配置更新
        checkAndApplyRemoteConfig();
        std::cout << "[DEBUG] reportStatusLoop: sleeping 5 seconds" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    std::cout << "[DEBUG] reportStatusLoop: exiting" << std::endl;
}

// 清理超过60秒没有心跳的过期节点
void ClusterMonitor::cleanupExpiredNodes(RedisConn& conn) {
    // 获取所有注册的节点
    std::vector<std::pair<std::string, double>> members = conn.zrange("cluster:nodes:register", 0, -1);
    
    time_t now = time(nullptr);
    const int EXPIRED_SECONDS = 60; // 超过60秒没有心跳视为过期
    
    int cleanedCount = 0;
    for (const auto& member : members) {
        try {
            int port = std::stoi(member.first);
            
            // 跳过当前节点
            if (port == m_port) continue;
            
            // 检查心跳时间戳
            std::string key = getNodeKey(port);
            std::string heartbeat = conn.get(key + ":heartbeat");
            
            bool shouldClean = false;
            
            if (heartbeat.empty()) {
                // 没有心跳记录，视为过期
                shouldClean = true;
                std::cout << "[INFO] 清理过期节点: 端口 " << port << " (无心跳)" << std::endl;
            } else {
                // 解析心跳时间戳
                struct tm hb_tm = {};
                if (strptime(heartbeat.c_str(), "%Y-%m-%d %H:%M:%S", &hb_tm) != nullptr) {
                    time_t hb_time = mktime(&hb_tm);
                    double diff = difftime(now, hb_time);
                    
                    if (diff > EXPIRED_SECONDS) {
                        shouldClean = true;
                        std::cout << "[INFO] 清理过期节点: 端口 " << port << " (心跳过期 " << static_cast<int>(diff) << "秒)" << std::endl;
                    }
                }
            }
            
            if (shouldClean) {
                conn.zrem("cluster:nodes:register", std::to_string(port));
                conn.del(key);
                conn.del(key + ":heartbeat");
                conn.del("cluster:node:" + std::to_string(port) + ":info");
                cleanedCount++;
            }
        } catch (...) {
            // 忽略无效的端口
        }
    }
    
    if (cleanedCount > 0) {
        std::cout << "[INFO] 已清理 " << cleanedCount << " 个过期节点" << std::endl;
    }
}

void ClusterMonitor::writeLocalStatus() {
    auto poolStatus = DBConnection::getMysqlPoolStatus();
    auto redisStatus = DBConnection::getRedisPoolStatus();
    
    RedisConn conn(m_redisHost, m_redisPort);
    if (!conn.connect()) {
        std::cerr << "[DEBUG] ClusterMonitor: Failed to connect to Redis for status reporting" << std::endl;
        return;
    }
    
    std::string key = getNodeKey(m_port);
    
    time_t now = time(nullptr);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    // 更新节点状态
    conn.hset(key, "port", std::to_string(m_port));
    conn.hset(key, "online", "1");
    conn.hset(key, "mysql_active", std::to_string(poolStatus.active));
    conn.hset(key, "mysql_idle", std::to_string(poolStatus.idle));
    conn.hset(key, "mysql_total", std::to_string(poolStatus.total));
    conn.hset(key, "mysql_maxActive", std::to_string(poolStatus.maxActive));
    conn.hset(key, "redis_active", std::to_string(redisStatus.active));
    conn.hset(key, "redis_idle", std::to_string(redisStatus.idle));
    conn.hset(key, "redis_total", std::to_string(redisStatus.total));
    conn.hset(key, "redis_maxActive", std::to_string(redisStatus.maxActive));
    conn.hset(key, "timestamp", timestamp);
    
    // 标记心跳，用于检测节点是否存活
    conn.set(key + ":heartbeat", timestamp);
    
    // 节点信息更新
    std::string infoKey = "cluster:node:" + std::to_string(m_port) + ":info";
    conn.hset(infoKey, "lastHeartbeat", timestamp);
}

std::string ClusterMonitor::getNodeKey(int port) {
    return "cluster:node:" + std::to_string(port) + ":status";
}

NodeStatus ClusterMonitor::getLocalNodeStatus() {
    NodeStatus status;
    status.port = m_port;
    
    auto poolStatus = DBConnection::getMysqlPoolStatus();
    auto redisStatus = DBConnection::getRedisPoolStatus();
    
    status.online = true;
    status.mysql_active = poolStatus.active;
    status.mysql_idle = poolStatus.idle;
    status.mysql_total = poolStatus.total;
    status.mysql_maxActive = poolStatus.maxActive;
    status.redis_active = redisStatus.active;
    status.redis_idle = redisStatus.idle;
    status.redis_total = redisStatus.total;
    status.redis_maxActive = redisStatus.maxActive;
    
    time_t now = time(nullptr);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    status.timestamp = timestamp;
    
    return status;
}

ClusterStatus ClusterMonitor::getClusterStatus() {
    ClusterStatus cluster;
    cluster.total_nodes = 0;
    cluster.online_nodes = 0;
    cluster.offline_nodes = 0;
    cluster.total_mysql_active = 0;
    cluster.total_mysql_idle = 0;
    cluster.total_mysql_max = 0;
    cluster.total_redis_active = 0;
    cluster.total_redis_idle = 0;
    cluster.total_redis_max = 0;
    
    RedisConn conn(m_redisHost, m_redisPort);
    if (!conn.connect()) {
        // Redis连接失败，返回空结果
        return cluster;
    }
    
    // 首先清理过期节点（心跳超过60秒未更新的）
    cleanupExpiredNodes(conn);
    
    // 获取所有注册的节点
    std::vector<int> registeredPorts = getRegisteredPorts();
    
    // 如果没有注册的节点，返回空结果
    if (registeredPorts.empty()) {
        return cluster;
    }
    
    cluster.total_nodes = static_cast<int>(registeredPorts.size());
    
    for (int port : registeredPorts) {
        NodeStatus node;
        node.port = port;
        
        std::string key = getNodeKey(port);
        
        // 检查心跳是否存在，如果不存在超过15秒则认为节点离线
        std::string heartbeat = conn.get(key + ":heartbeat");
        bool hasHeartbeat = !heartbeat.empty();
        
        std::unordered_map<std::string, std::string> data = conn.hgetall(key);
        
        if (data.empty() || !hasHeartbeat) {
            node.online = false;
            cluster.offline_nodes++;
        } else {
            node.online = true;
            node.mysql_active = data.count("mysql_active") ? std::stoi(data["mysql_active"]) : 0;
            node.mysql_idle = data.count("mysql_idle") ? std::stoi(data["mysql_idle"]) : 0;
            node.mysql_total = data.count("mysql_total") ? std::stoi(data["mysql_total"]) : 0;
            node.mysql_maxActive = data.count("mysql_maxActive") ? std::stoi(data["mysql_maxActive"]) : 0;
            node.redis_active = data.count("redis_active") ? std::stoi(data["redis_active"]) : 0;
            node.redis_idle = data.count("redis_idle") ? std::stoi(data["redis_idle"]) : 0;
            node.redis_total = data.count("redis_total") ? std::stoi(data["redis_total"]) : 0;
            node.redis_maxActive = data.count("redis_maxActive") ? std::stoi(data["redis_maxActive"]) : 0;
            node.timestamp = data.count("timestamp") ? data["timestamp"] : "";
            
            cluster.online_nodes++;
            cluster.total_mysql_active += node.mysql_active;
            cluster.total_mysql_idle += node.mysql_idle;
            cluster.total_mysql_max += node.mysql_maxActive;
            cluster.total_redis_active += node.redis_active;
            cluster.total_redis_idle += node.redis_idle;
            cluster.total_redis_max += node.redis_maxActive;
        }
        
        cluster.nodes[port] = node;
    }
    
    return cluster;
}

// 广播配置更新到Redis，其他节点会检测到并应用新配置
void ClusterMonitor::broadcastConfigUpdate(const std::string& poolType, int maxActive, int minIdlePercent, int maxWaitMs, int idleTimeoutMs) {
    std::cout << "[DEBUG] 开始广播配置更新 - poolType: " << poolType 
              << ", redis: " << m_redisHost << ":" << m_redisPort << std::endl;
    
    RedisConn conn(m_redisHost, m_redisPort);
    if (!conn.connect()) {
        std::cerr << "[ERROR] ClusterMonitor: Failed to connect to Redis for config broadcast" << std::endl;
        return;
    }
    
    std::cout << "[DEBUG] Redis连接成功，准备写入配置" << std::endl;
    
    time_t now = time(nullptr);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    // 存储配置更新信息到Redis
    std::string configKey = "cluster:config:" + poolType;
    std::cout << "[DEBUG] 写入配置到key: " << configKey << std::endl;
    
    bool hset1 = conn.hset(configKey, "maxActive", std::to_string(maxActive));
    bool hset2 = conn.hset(configKey, "minIdlePercent", std::to_string(minIdlePercent));
    bool hset3 = conn.hset(configKey, "maxWaitMs", std::to_string(maxWaitMs));
    bool hset4 = conn.hset(configKey, "idleTimeoutMs", std::to_string(idleTimeoutMs));
    bool hset5 = conn.hset(configKey, "updatedAt", timestamp);
    bool hset6 = conn.hset(configKey, "updatedBy", std::to_string(m_port));
    
    std::cout << "[DEBUG] hset结果: " << hset1 << "," << hset2 << "," << hset3 << "," << hset4 << "," << hset5 << "," << hset6 << std::endl;
    
    // 获取当前版本号并递增
    std::string versionKey = configKey + ":version";
    std::string currentVersionStr = conn.get(versionKey);
    std::cout << "[DEBUG] 当前版本号: " << currentVersionStr << std::endl;
    
    int version = currentVersionStr.empty() ? 1 : (std::stoi(currentVersionStr) + 1);
    bool setResult = conn.set(versionKey, std::to_string(version));
    std::cout << "[DEBUG] 设置版本号 " << version << " 结果: " << setResult << std::endl;
    
    // 验证写入是否成功
    auto verifyConfig = conn.hgetall(configKey);
    std::cout << "[DEBUG] 验证配置读取，字段数: " << verifyConfig.size() << std::endl;
    for (const auto& kv : verifyConfig) {
        std::cout << "[DEBUG]   " << kv.first << " = " << kv.second << std::endl;
    }
    
    // 更新当前节点的版本号，避免重复应用配置
    if (poolType == "mysql") {
        m_lastMysqlVersion = std::to_string(version);
        std::cout << "[DEBUG] 更新当前节点MySQL版本号为: " << m_lastMysqlVersion << std::endl;
    } else {
        m_lastRedisVersion = std::to_string(version);
        std::cout << "[DEBUG] 更新当前节点Redis版本号为: " << m_lastRedisVersion << std::endl;
    }
    
    std::cout << "[INFO] 集群配置已广播: " << poolType 
              << ", maxActive=" << maxActive 
              << ", version=" << version
              << ", updatedBy=" << m_port << std::endl;
}

// 加载初始配置（节点启动时调用）
void ClusterMonitor::loadInitialConfig() {
    std::cout << "[DEBUG] 开始加载初始配置，连接Redis: " << m_redisHost << ":" << m_redisPort << std::endl;
    
    RedisConn conn(m_redisHost, m_redisPort);
    if (!conn.connect()) {
        std::cerr << "[ERROR] ClusterMonitor: Failed to connect to Redis for initial config load" << std::endl;
        return;
    }
    
    std::cout << "[DEBUG] Redis连接成功，开始读取配置" << std::endl;
    
    // 加载MySQL初始配置
    std::string mysqlConfigKey = "cluster:config:mysql";
    std::string mysqlVersionStr = conn.get(mysqlConfigKey + ":version");
    std::cout << "[DEBUG] MySQL版本号: " << mysqlVersionStr 
              << ", 本地版本: " << m_lastMysqlVersion << std::endl;
    
    if (!mysqlVersionStr.empty()) {
        std::unordered_map<std::string, std::string> config = conn.hgetall(mysqlConfigKey);
        std::cout << "[DEBUG] MySQL配置字段数: " << config.size() << std::endl;
        
        if (!config.empty()) {
            int maxActive = std::stoi(config.count("maxActive") ? config["maxActive"] : "20");
            int minIdlePercent = std::stoi(config.count("minIdlePercent") ? config["minIdlePercent"] : "20");
            int maxWaitMs = std::stoi(config.count("maxWaitMs") ? config["maxWaitMs"] : "3000");
            int idleTimeoutMs = std::stoi(config.count("idleTimeoutMs") ? config["idleTimeoutMs"] : "30000");
            
            std::cout << "[DEBUG] 准备应用MySQL配置: maxActive=" << maxActive 
                      << ", minIdlePercent=" << minIdlePercent << std::endl;
            
            DBConnection::setMysqlPoolMaxActive(maxActive);
            DBConnection::setMysqlPoolMinIdlePercent(minIdlePercent);
            DBConnection::setMysqlPoolMaxWaitMs(maxWaitMs);
            DBConnection::setMysqlPoolIdleTimeoutMs(idleTimeoutMs);
            
            m_lastMysqlVersion = mysqlVersionStr;
            std::cout << "[INFO] 节点 " << m_port << " 已加载MySQL初始配置, version=" << mysqlVersionStr << std::endl;
        }
    } else {
        std::cout << "[DEBUG] 没有找到MySQL配置，使用默认配置" << std::endl;
    }
    
    // 加载Redis初始配置
    std::string redisConfigKey = "cluster:config:redis";
    std::string redisVersionStr = conn.get(redisConfigKey + ":version");
    std::cout << "[DEBUG] Redis版本号: " << redisVersionStr 
              << ", 本地版本: " << m_lastRedisVersion << std::endl;
    
    if (!redisVersionStr.empty()) {
        std::unordered_map<std::string, std::string> config = conn.hgetall(redisConfigKey);
        std::cout << "[DEBUG] Redis配置字段数: " << config.size() << std::endl;
        
        if (!config.empty()) {
            int maxActive = std::stoi(config.count("maxActive") ? config["maxActive"] : "20");
            int minIdlePercent = std::stoi(config.count("minIdlePercent") ? config["minIdlePercent"] : "20");
            int maxWaitMs = std::stoi(config.count("maxWaitMs") ? config["maxWaitMs"] : "3000");
            int idleTimeoutMs = std::stoi(config.count("idleTimeoutMs") ? config["idleTimeoutMs"] : "30000");
            
            std::cout << "[DEBUG] 准备应用Redis配置: maxActive=" << maxActive 
                      << ", minIdlePercent=" << minIdlePercent << std::endl;
            
            DBConnection::setRedisPoolMaxActive(maxActive);
            DBConnection::setRedisPoolMinIdlePercent(minIdlePercent);
            DBConnection::setRedisPoolMaxWaitMs(maxWaitMs);
            DBConnection::setRedisPoolIdleTimeoutMs(idleTimeoutMs);
            
            m_lastRedisVersion = redisVersionStr;
            std::cout << "[INFO] 节点 " << m_port << " 已加载Redis初始配置, version=" << redisVersionStr << std::endl;
        }
    } else {
        std::cout << "[DEBUG] 没有找到Redis配置，使用默认配置" << std::endl;
    }
}

// 检查并应用远程配置更新
bool ClusterMonitor::checkAndApplyRemoteConfig() {
    RedisConn conn(m_redisHost, m_redisPort);
    if (!conn.connect()) {
        return false;
    }
    
    // 检查MySQL配置更新
    std::string mysqlConfigKey = "cluster:config:mysql";
    std::string mysqlVersionStr = conn.get(mysqlConfigKey + ":version");
    
    if (!mysqlVersionStr.empty() && mysqlVersionStr != m_lastMysqlVersion) {
        std::cout << "[DEBUG] 检测到MySQL配置更新: 本地版本=" << m_lastMysqlVersion 
                  << ", 远程版本=" << mysqlVersionStr << std::endl;
        
        std::unordered_map<std::string, std::string> config = conn.hgetall(mysqlConfigKey);
        if (!config.empty()) {
            int maxActive = std::stoi(config.count("maxActive") ? config["maxActive"] : "20");
            int minIdlePercent = std::stoi(config.count("minIdlePercent") ? config["minIdlePercent"] : "20");
            int maxWaitMs = std::stoi(config.count("maxWaitMs") ? config["maxWaitMs"] : "3000");
            int idleTimeoutMs = std::stoi(config.count("idleTimeoutMs") ? config["idleTimeoutMs"] : "30000");
            
            DBConnection::setMysqlPoolMaxActive(maxActive);
            DBConnection::setMysqlPoolMinIdlePercent(minIdlePercent);
            DBConnection::setMysqlPoolMaxWaitMs(maxWaitMs);
            DBConnection::setMysqlPoolIdleTimeoutMs(idleTimeoutMs);
            
            m_lastMysqlVersion = mysqlVersionStr;
            std::cout << "[INFO] 节点 " << m_port << " 已应用MySQL远程配置更新, version=" << mysqlVersionStr << std::endl;
        }
    }
    
    // 检查Redis配置更新
    std::string redisConfigKey = "cluster:config:redis";
    std::string redisVersionStr = conn.get(redisConfigKey + ":version");
    
    if (!redisVersionStr.empty() && redisVersionStr != m_lastRedisVersion) {
        std::cout << "[DEBUG] 检测到Redis配置更新: 本地版本=" << m_lastRedisVersion 
                  << ", 远程版本=" << redisVersionStr << std::endl;
        
        std::unordered_map<std::string, std::string> config = conn.hgetall(redisConfigKey);
        if (!config.empty()) {
            int maxActive = std::stoi(config.count("maxActive") ? config["maxActive"] : "20");
            int minIdlePercent = std::stoi(config.count("minIdlePercent") ? config["minIdlePercent"] : "20");
            int maxWaitMs = std::stoi(config.count("maxWaitMs") ? config["maxWaitMs"] : "3000");
            int idleTimeoutMs = std::stoi(config.count("idleTimeoutMs") ? config["idleTimeoutMs"] : "30000");
            
            DBConnection::setRedisPoolMaxActive(maxActive);
            DBConnection::setRedisPoolMinIdlePercent(minIdlePercent);
            DBConnection::setRedisPoolMaxWaitMs(maxWaitMs);
            DBConnection::setRedisPoolIdleTimeoutMs(idleTimeoutMs);
            
            m_lastRedisVersion = redisVersionStr;
            std::cout << "[INFO] 节点 " << m_port << " 已应用Redis远程配置更新, version=" << redisVersionStr << std::endl;
        }
    }
    
    return true;
}

}