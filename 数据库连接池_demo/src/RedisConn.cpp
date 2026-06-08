
#include "RedisConn.h"

RedisConn::RedisConn(const std::string& host, int port, int timeout)
    : m_host(host), m_port(port), m_conn(nullptr), m_isValid(false) {
    m_timeout.tv_sec = timeout / 1000;
    m_timeout.tv_usec = (timeout % 1000) * 1000;
}

RedisConn::~RedisConn() {
    disconnect();
}

bool RedisConn::connect() {
    disconnect();
    
    m_conn = redisConnectWithTimeout(m_host.c_str(), m_port, m_timeout);
    if (!m_conn || m_conn->err) {
        if (m_conn) {
            redisFree(m_conn);
            m_conn = nullptr;
        }
        m_isValid.store(false);
        return false;
    }
    
    m_isValid.store(true);
    updateLastUsedTime();
    return true;
}

void RedisConn::disconnect() {
    if (m_conn) {
        redisFree(m_conn);
        m_conn = nullptr;
    }
    m_isValid.store(false);
}

bool RedisConn::isValid() const {
    return m_isValid.load() && m_conn != nullptr;
}

bool RedisConn::reconnect() {
    return connect();
}

void RedisConn::markInvalid() {
    m_isValid.store(false);
}

bool RedisConn::set(const std::string& key, const std::string& value) {
    if (!isValid()) {
        return false;
    }
    
    updateLastUsedTime();
    
    void* reply = redisCommand(m_conn, "SET %s %s", key.c_str(), value.c_str());
    if (!reply) {
        m_isValid.store(false);
        return false;
    }
    
    freeReplyObject(reply);
    return true;
}

std::string RedisConn::get(const std::string& key) {
    if (!isValid()) {
        return "";
    }
    
    updateLastUsedTime();
    
    redisReply* reply = reinterpret_cast<redisReply*>(redisCommand(m_conn, "GET %s", key.c_str()));
    if (!reply) {
        m_isValid.store(false);
        return "";
    }
    
    if (reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        m_isValid.store(false);
        return "";
    }
    
    std::string result;
    if (reply->type == REDIS_REPLY_STRING) {
        result = std::string(reply->str, reply->len);
    }
    
    freeReplyObject(reply);
    return result;
}

bool RedisConn::hset(const std::string& key, const std::string& field, const std::string& value) {
    if (!isValid()) {
        return false;
    }
    
    updateLastUsedTime();
    
    void* reply = redisCommand(m_conn, "HSET %s %s %s", key.c_str(), field.c_str(), value.c_str());
    if (!reply) {
        m_isValid.store(false);
        return false;
    }
    
    freeReplyObject(reply);
    return true;
}

std::string RedisConn::hget(const std::string& key, const std::string& field) {
    if (!isValid()) {
        return "";
    }
    
    updateLastUsedTime();
    
    redisReply* reply = reinterpret_cast<redisReply*>(redisCommand(m_conn, "HGET %s %s", key.c_str(), field.c_str()));
    if (!reply) {
        m_isValid.store(false);
        return "";
    }
    
    if (reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        m_isValid.store(false);
        return "";
    }
    
    std::string result;
    if (reply->type == REDIS_REPLY_STRING) {
        result = std::string(reply->str, reply->len);
    }
    
    freeReplyObject(reply);
    return result;
}

std::unordered_map<std::string, std::string> RedisConn::hgetall(const std::string& key) {
    std::unordered_map<std::string, std::string> result;
    
    if (!isValid()) {
        return result;
    }
    
    updateLastUsedTime();
    
    redisReply* reply = reinterpret_cast<redisReply*>(redisCommand(m_conn, "HGETALL %s", key.c_str()));
    if (!reply) {
        m_isValid.store(false);
        return result;
    }
    
    if (reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        m_isValid.store(false);
        return result;
    }
    
    if (reply->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply->elements; i += 2) {
            std::string field(reply->element[i]->str, reply->element[i]->len);
            std::string value(reply->element[i+1]->str, reply->element[i+1]->len);
            result[field] = value;
        }
    }
    
    freeReplyObject(reply);
    return result;
}

bool RedisConn::del(const std::string& key) {
    if (!isValid()) {
        return false;
    }
    
    updateLastUsedTime();
    
    void* reply = redisCommand(m_conn, "DEL %s", key.c_str());
    if (!reply) {
        m_isValid.store(false);
        return false;
    }
    
    freeReplyObject(reply);
    return true;
}

bool RedisConn::hdel(const std::string& key, const std::string& field) {
    if (!isValid()) {
        return false;
    }
    
    updateLastUsedTime();
    
    void* reply = redisCommand(m_conn, "HDEL %s %s", key.c_str(), field.c_str());
    if (!reply) {
        m_isValid.store(false);
        return false;
    }
    
    freeReplyObject(reply);
    return true;
}

bool RedisConn::ping() {
    if (!m_conn) {
        m_isValid.store(false);
        return false;
    }
    
    redisReply* reply = reinterpret_cast<redisReply*>(redisCommand(m_conn, "PING"));
    if (!reply) {
        m_isValid.store(false);
        return false;
    }
    
    // Redis PING 返回的是 REDIS_REPLY_STATUS 类型，内容为 "PONG"
    // 而不是 REDIS_REPLY_STRING，原来的判断永远为 false 导致所有连接验证失败
    bool success = false;
    if (reply->type == REDIS_REPLY_STATUS) {
        success = (std::string(reply->str, reply->len) == "PONG");
    } else if (reply->type == REDIS_REPLY_STRING) {
        // 兼容某些 Redis 版本或代理返回 STRING 类型
        success = (std::string(reply->str, reply->len) == "PONG");
    }
    
    freeReplyObject(reply);
    
    m_isValid.store(success);
    return success;
}

bool RedisConn::zadd(const std::string& key, const std::string& member, double score) {
    if (!isValid()) {
        return false;
    }
    
    updateLastUsedTime();
    
    void* reply = redisCommand(m_conn, "ZADD %s %f %s", key.c_str(), score, member.c_str());
    if (!reply) {
        m_isValid.store(false);
        return false;
    }
    
    freeReplyObject(reply);
    return true;
}

std::vector<std::pair<std::string, double>> RedisConn::zrange(const std::string& key, int start, int stop) {
    std::vector<std::pair<std::string, double>> result;
    
    if (!isValid()) {
        return result;
    }
    
    updateLastUsedTime();
    
    redisReply* reply = reinterpret_cast<redisReply*>(redisCommand(m_conn, "ZRANGE %s %d %d WITHSCORES", key.c_str(), start, stop));
    if (!reply) {
        m_isValid.store(false);
        return result;
    }
    
    if (reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        m_isValid.store(false);
        return result;
    }
    
    if (reply->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply->elements; i += 2) {
            std::string member(reply->element[i]->str, reply->element[i]->len);
            double score = 0;
            if (i + 1 < reply->elements) {
                score = std::atof(reply->element[i + 1]->str);
            }
            result.push_back({member, score});
        }
    }
    
    freeReplyObject(reply);
    return result;
}

bool RedisConn::zrem(const std::string& key, const std::string& member) {
    if (!isValid()) {
        return false;
    }
    
    updateLastUsedTime();
    
    void* reply = redisCommand(m_conn, "ZREM %s %s", key.c_str(), member.c_str());
    if (!reply) {
        m_isValid.store(false);
        return false;
    }
    
    freeReplyObject(reply);
    return true;
}

void RedisConn::updateLastUsedTime() {
    m_lastUsedTime = std::chrono::steady_clock::now();
}

std::chrono::steady_clock::time_point RedisConn::getLastUsedTime() const {
    return m_lastUsedTime;
}
