/**
 * @file SessionManager.cpp
 * @brief 会话管理器实现文件
 *
 * 实现说明：
 * - 使用Redis存储会话，支持集群环境下的会话共享
 * - 会话ID格式为32字节十六进制字符串
 * - 会话过期时间由Redis自动管理
 * - 包含用户名和角色信息
 */

#include "SessionManager.h"
#include <chrono>
#include <iostream>

namespace Auth {

// 构造函数 - 初始化Redis连接为nullptr
SessionManager::SessionManager() : m_redis(nullptr), m_timeoutSeconds(86400) {}

// 获取单例实例 - 使用局部静态变量实现懒汉单例
SessionManager& SessionManager::instance() {
    static SessionManager instance;
    return instance;
}

// 初始化Redis连接
bool SessionManager::initRedis(const std::string& host, int port) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 如果已有连接，先关闭
    if (m_redis) {
        redisFree(m_redis);
        m_redis = nullptr;
    }

    // 连接到Redis服务器
    m_redis = redisConnect(host.c_str(), port);
    if (m_redis == nullptr || m_redis->err) {
        if (m_redis) {
            redisFree(m_redis);
            m_redis = nullptr;
        }
        return false;
    }

    return true;
}

// 生成随机会话ID
std::string SessionManager::generateSessionId() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    std::stringstream ss;
    for (int i = 0; i < 32; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << dis(gen);
    }
    return ss.str();
}

// 创建新会话
std::string SessionManager::createSession(const std::string& username, const std::string& role) {
    if (!m_redis) {
        std::cerr << "[DEBUG] Redis not connected!" << std::endl;
        return "";
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // 生成32字节随机会话ID
    std::string sessionId = generateSessionId();

    std::cout << "[DEBUG] Creating session for user: " << username << ", session ID: " << sessionId << std::endl;

    // 获取当前时间戳
    long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // 构造会话数据（JSON格式）
    std::string sessionData = "{\"username\":\"" + username +
                           "\",\"role\":\"" + role +
                           "\",\"createdAt\":" + std::to_string(timestamp) + "}";

    // 存储到Redis，key格式：session:<sessionId>
    std::string key = "session:" + sessionId;
    redisReply* reply = (redisReply*)redisCommand(m_redis, "SETEX %s %d %s",
                                               key.c_str(), m_timeoutSeconds, sessionData.c_str());

    if (reply == nullptr || m_redis->err) {
        if (reply) freeReplyObject(reply);
        std::cerr << "[DEBUG] Failed to store session in Redis!" << std::endl;
        return "";
    }

    std::cout << "[DEBUG] Session stored in Redis successfully" << std::endl;

    freeReplyObject(reply);
    return sessionId;
}

// 验证会话是否有效
bool SessionManager::validateSession(const std::string& sessionId) {
    if (!m_redis) {
        std::cerr << "[DEBUG] validateSession: Redis not connected!" << std::endl;
        return false;
    }

    if (sessionId.empty()) {
        std::cerr << "[DEBUG] validateSession: Session ID is empty!" << std::endl;
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    std::string key = "session:" + sessionId;
    std::cout << "[DEBUG] validateSession: Checking key: " << key << std::endl;
    
    redisReply* reply = (redisReply*)redisCommand(m_redis, "EXISTS %s", key.c_str());

    if (reply == nullptr || m_redis->err) {
        if (reply) freeReplyObject(reply);
        std::cerr << "[DEBUG] validateSession: Redis command failed!" << std::endl;
        return false;
    }

    bool exists = (reply->integer == 1);
    std::cout << "[DEBUG] validateSession: Session " << (exists ? "exists" : "does NOT exist") << std::endl;
    freeReplyObject(reply);

    // 如果会话存在，刷新过期时间（续期）
    if (exists) {
        redisCommand(m_redis, "EXPIRE %s %d", key.c_str(), m_timeoutSeconds);
        std::cout << "[DEBUG] validateSession: Session expiration renewed" << std::endl;
    }

    return exists;
}

// 获取会话对应的用户名
std::string SessionManager::getSessionUser(const std::string& sessionId) {
    if (!m_redis) {
        std::cerr << "[DEBUG] getSessionUser: Redis not connected!" << std::endl;
        return "";
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    std::string key = "session:" + sessionId;
    std::cout << "[DEBUG] getSessionUser: Getting key: " << key << std::endl;
    
    redisReply* reply = (redisReply*)redisCommand(m_redis, "GET %s", key.c_str());

    if (reply == nullptr) {
        std::cerr << "[DEBUG] getSessionUser: Reply is nullptr!" << std::endl;
        return "";
    }
    
    if (m_redis->err) {
        std::cerr << "[DEBUG] getSessionUser: Redis error: " << m_redis->errstr << std::endl;
        freeReplyObject(reply);
        return "";
    }
    
    if (reply->type != REDIS_REPLY_STRING) {
        std::cerr << "[DEBUG] getSessionUser: Reply type is not string, type: " << reply->type << std::endl;
        freeReplyObject(reply);
        return "";
    }

    // 解析JSON获取用户名（简单解析）
    std::string data = std::string(reply->str, reply->len);
    std::cout << "[DEBUG] getSessionUser: Raw data from Redis: '" << data << "'" << std::endl;
    std::cout << "[DEBUG] getSessionUser: Data length: " << data.length() << std::endl;
    for (size_t i = 0; i < std::min((size_t)20, data.length()); ++i) {
        std::cout << "[DEBUG] getSessionUser: Char at " << i << ": '" << data[i] << "' (ASCII: " << (int)data[i] << ")" << std::endl;
    }
    freeReplyObject(reply);

    // 简单提取username
    size_t pos = data.find("\"username\":\"");
    std::cout << "[DEBUG] getSessionUser: Found username at position: " << pos << std::endl;
    if (pos != std::string::npos) {
        size_t nextPos = pos + 12; // "username":" 的长度是12
        std::cout << "[DEBUG] getSessionUser: Next position after username marker: " << nextPos << std::endl;
        if (nextPos < data.length()) {
            std::cout << "[DEBUG] getSessionUser: Character at nextPos: '" << data[nextPos] << "' (ASCII: " << (int)data[nextPos] << ")" << std::endl;
        }
        size_t end = data.find("\"", nextPos);
        std::cout << "[DEBUG] getSessionUser: End position: " << end << std::endl;
        if (end != std::string::npos && end > nextPos) {
            std::string username = data.substr(nextPos, end - nextPos);
            std::cout << "[DEBUG] getSessionUser: Extracted username: '" << username << "'" << std::endl;
            return username;
        } else {
            std::cerr << "[DEBUG] getSessionUser: End position is invalid!" << std::endl;
        }
    }

    return "";
}

// 获取会话对应的用户角色
std::string SessionManager::getSessionRole(const std::string& sessionId) {
    if (!m_redis) {
        std::cerr << "[DEBUG] getSessionRole: Redis not connected!" << std::endl;
        return "";
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    std::string key = "session:" + sessionId;
    redisReply* reply = (redisReply*)redisCommand(m_redis, "GET %s", key.c_str());

    if (reply == nullptr || m_redis->err) {
        if (reply) freeReplyObject(reply);
        std::cerr << "[DEBUG] getSessionRole: Redis command failed!" << std::endl;
        return "";
    }
    
    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        std::cerr << "[DEBUG] getSessionRole: Reply type is not string, type: " << reply->type << std::endl;
        return "";
    }

    std::string data = std::string(reply->str, reply->len);
    std::cout << "[DEBUG] getSessionRole: Raw data from Redis: '" << data << "'" << std::endl;
    freeReplyObject(reply);

    // 简单提取role - "role":" 的长度是8
    size_t pos = data.find("\"role\":\"");
    std::cout << "[DEBUG] getSessionRole: Found role at position: " << pos << std::endl;
    if (pos != std::string::npos) {
        pos += 8; // "role":" 的长度是8
        size_t end = data.find("\"", pos);
        std::cout << "[DEBUG] getSessionRole: End position: " << end << std::endl;
        if (end != std::string::npos) {
            std::string role = data.substr(pos, end - pos);
            std::cout << "[DEBUG] getSessionRole: Extracted role: '" << role << "'" << std::endl;
            return role;
        }
    }

    std::cerr << "[DEBUG] getSessionRole: Role not found in session data!" << std::endl;
    return "";
}

// 销毁指定会话
void SessionManager::destroySession(const std::string& sessionId) {
    if (!m_redis) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    std::string key = "session:" + sessionId;
    redisCommand(m_redis, "DEL %s", key.c_str());
}

// 设置会话超时时间
void SessionManager::setSessionTimeout(int seconds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_timeoutSeconds = seconds;
}

// 检查Redis连接状态
bool SessionManager::isRedisConnected() {
    if (!m_redis) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    redisReply* reply = (redisReply*)redisCommand(m_redis, "PING");
    if (reply == nullptr || m_redis->err) {
        if (reply) freeReplyObject(reply);
        return false;
    }

    bool connected = (reply->type == REDIS_REPLY_STATUS &&
                     std::string(reply->str, reply->len) == "PONG");
    freeReplyObject(reply);
    return connected;
}

} // namespace Auth
