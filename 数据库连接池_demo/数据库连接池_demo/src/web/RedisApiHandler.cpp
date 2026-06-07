
#include "RedisApiHandler.h"
#include "../Logger.h"
#include <sstream>
#include <chrono>

namespace WebServer {

HttpResponse RedisApiHandler::get(const HttpRequest& req) {
    HttpResponse res;
    auto startTime = std::chrono::steady_clock::now();
    bool success = false;
    
    auto it = req.params.find("key");
    if (it == req.params.end()) {
        res.statusCode = 400;
        res.body = "{\"success\": false, \"message\": \"缺少key参数\"}";
        return res;
    }
    
    try {
        auto conn = DBConnection::getRedisConnection();
        if (!conn.isValid()) {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"无法获取Redis连接\"}";
            return res;
        }
        
        std::string value = conn->get(it->second);
        
        res.contentType = "application/json";
        res.body = "{\"success\": true, \"key\": \"" + it->second + "\", \"value\": \"" + value + "\"}";
        success = true;
    } catch (...) {
        res.statusCode = 500;
        res.body = "{\"success\": false, \"message\": \"服务器内部错误\"}";
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    Logger::getInstance().log(Logger::LogType::REDIS, req.clientIP, "GET", "GET " + it->second, success, duration);
    
    return res;
}

HttpResponse RedisApiHandler::set(const HttpRequest& req) {
    HttpResponse res;
    auto startTime = std::chrono::steady_clock::now();
    bool success = false;
    
    auto keyIt = req.params.find("key");
    auto valueIt = req.params.find("value");
    
    if (keyIt == req.params.end() || valueIt == req.params.end()) {
        res.statusCode = 400;
        res.body = "{\"success\": false, \"message\": \"缺少参数\"}";
        return res;
    }
    
    try {
        auto conn = DBConnection::getRedisConnection();
        if (!conn.isValid()) {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"无法获取Redis连接\"}";
            return res;
        }
        
        bool ret = conn->set(keyIt->second, valueIt->second);
        
        res.contentType = "application/json";
        if (ret) {
            res.body = "{\"success\": true, \"message\": \"设置成功\"}";
            success = true;
        } else {
            res.body = "{\"success\": false, \"message\": \"设置失败\"}";
        }
    } catch (...) {
        res.statusCode = 500;
        res.body = "{\"success\": false, \"message\": \"服务器内部错误\"}";
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    Logger::getInstance().log(Logger::LogType::REDIS, req.clientIP, "SET", "SET " + keyIt->second + " " + valueIt->second, success, duration);
    
    return res;
}

HttpResponse RedisApiHandler::hget(const HttpRequest& req) {
    HttpResponse res;
    auto startTime = std::chrono::steady_clock::now();
    bool success = false;
    
    auto keyIt = req.params.find("key");
    auto fieldIt = req.params.find("field");
    
    if (keyIt == req.params.end() || fieldIt == req.params.end()) {
        res.statusCode = 400;
        res.body = "{\"success\": false, \"message\": \"缺少参数\"}";
        return res;
    }
    
    try {
        auto conn = DBConnection::getRedisConnection();
        if (!conn.isValid()) {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"无法获取Redis连接\"}";
            return res;
        }
        
        std::string value = conn->hget(keyIt->second, fieldIt->second);
        
        res.contentType = "application/json";
        res.body = "{\"success\": true, \"key\": \"" + keyIt->second + "\", \"field\": \"" 
                   + fieldIt->second + "\", \"value\": \"" + value + "\"}";
        success = true;
    } catch (...) {
        res.statusCode = 500;
        res.body = "{\"success\": false, \"message\": \"服务器内部错误\"}";
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    Logger::getInstance().log(Logger::LogType::REDIS, req.clientIP, "HGET", "HGET " + keyIt->second + " " + fieldIt->second, success, duration);
    
    return res;
}

HttpResponse RedisApiHandler::hset(const HttpRequest& req) {
    HttpResponse res;
    auto startTime = std::chrono::steady_clock::now();
    bool success = false;
    
    auto keyIt = req.params.find("key");
    auto fieldIt = req.params.find("field");
    auto valueIt = req.params.find("value");
    
    if (keyIt == req.params.end() || fieldIt == req.params.end() || valueIt == req.params.end()) {
        res.statusCode = 400;
        res.body = "{\"success\": false, \"message\": \"缺少参数\"}";
        return res;
    }
    
    try {
        auto conn = DBConnection::getRedisConnection();
        if (!conn.isValid()) {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"无法获取Redis连接\"}";
            return res;
        }
        
        bool ret = conn->hset(keyIt->second, fieldIt->second, valueIt->second);
        
        res.contentType = "application/json";
        if (ret) {
            res.body = "{\"success\": true, \"message\": \"设置成功\"}";
            success = true;
        } else {
            res.body = "{\"success\": false, \"message\": \"设置失败\"}";
        }
    } catch (...) {
        res.statusCode = 500;
        res.body = "{\"success\": false, \"message\": \"服务器内部错误\"}";
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    Logger::getInstance().log(Logger::LogType::REDIS, req.clientIP, "HSET", "HSET " + keyIt->second + " " + fieldIt->second + " " + valueIt->second, success, duration);
    
    return res;
}

HttpResponse RedisApiHandler::hgetall(const HttpRequest& req) {
    HttpResponse res;
    auto startTime = std::chrono::steady_clock::now();
    bool success = false;
    
    auto it = req.params.find("key");
    if (it == req.params.end()) {
        res.statusCode = 400;
        res.body = "{\"success\": false, \"message\": \"缺少key参数\"}";
        return res;
    }
    
    try {
        auto conn = DBConnection::getRedisConnection();
        if (!conn.isValid()) {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"无法获取Redis连接\"}";
            return res;
        }
        
        auto data = conn->hgetall(it->second);
        
        std::ostringstream oss;
        oss << "{\"success\": true, \"key\": \"" << it->second << "\", \"data\": {";
        bool first = true;
        for (const auto& pair : data) {
            if (!first) oss << ",";
            oss << "\"" << pair.first << "\": \"" << pair.second << "\"";
            first = false;
        }
        oss << "}}";
        
        res.contentType = "application/json";
        res.body = oss.str();
        success = true;
    } catch (...) {
        res.statusCode = 500;
        res.body = "{\"success\": false, \"message\": \"服务器内部错误\"}";
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    Logger::getInstance().log(Logger::LogType::REDIS, req.clientIP, "HGETALL", "HGETALL " + it->second, success, duration);
    
    return res;
}

HttpResponse RedisApiHandler::del(const HttpRequest& req) {
    HttpResponse res;
    auto startTime = std::chrono::steady_clock::now();
    bool success = false;
    
    auto it = req.params.find("key");
    if (it == req.params.end()) {
        res.statusCode = 400;
        res.body = "{\"success\": false, \"message\": \"缺少key参数\"}";
        return res;
    }
    
    try {
        auto conn = DBConnection::getRedisConnection();
        if (!conn.isValid()) {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"无法获取Redis连接\"}";
            return res;
        }
        
        bool ret = conn->del(it->second);
        
        res.contentType = "application/json";
        if (ret) {
            res.body = "{\"success\": true, \"message\": \"删除成功\"}";
            success = true;
        } else {
            res.body = "{\"success\": false, \"message\": \"删除失败\"}";
        }
    } catch (...) {
        res.statusCode = 500;
        res.body = "{\"success\": false, \"message\": \"服务器内部错误\"}";
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    Logger::getInstance().log(Logger::LogType::REDIS, req.clientIP, "DEL", "DEL " + it->second, success, duration);
    
    return res;
}

HttpResponse RedisApiHandler::hdel(const HttpRequest& req) {
    HttpResponse res;
    auto startTime = std::chrono::steady_clock::now();
    bool success = false;
    
    auto keyIt = req.params.find("key");
    auto fieldIt = req.params.find("field");
    
    if (keyIt == req.params.end() || fieldIt == req.params.end()) {
        res.statusCode = 400;
        res.body = "{\"success\": false, \"message\": \"缺少参数\"}";
        return res;
    }
    
    try {
        auto conn = DBConnection::getRedisConnection();
        if (!conn.isValid()) {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"无法获取Redis连接\"}";
            return res;
        }
        
        bool ret = conn->hdel(keyIt->second, fieldIt->second);
        
        res.contentType = "application/json";
        if (ret) {
            res.body = "{\"success\": true, \"message\": \"删除字段成功\"}";
            success = true;
        } else {
            res.body = "{\"success\": false, \"message\": \"删除字段失败\"}";
        }
    } catch (...) {
        res.statusCode = 500;
        res.body = "{\"success\": false, \"message\": \"服务器内部错误\"}";
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    Logger::getInstance().log(Logger::LogType::REDIS, req.clientIP, "HDEL", "HDEL " + keyIt->second + " " + fieldIt->second, success, duration);
    
    return res;
}

HttpResponse RedisApiHandler::keys(const HttpRequest& req) {
    HttpResponse res;
    auto startTime = std::chrono::steady_clock::now();
    bool success = false;
    
    auto it = req.params.find("pattern");
    std::string pattern = it != req.params.end() ? it->second : "*";
    
    try {
        auto conn = DBConnection::getRedisConnection();
        if (!conn.isValid()) {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"无法获取Redis连接\"}";
            return res;
        }
        
        res.contentType = "application/json";
        res.body = "{\"success\": true, \"keys\": []}";
        success = true;
    } catch (...) {
        res.statusCode = 500;
        res.body = "{\"success\": false, \"message\": \"服务器内部错误\"}";
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    Logger::getInstance().log(Logger::LogType::REDIS, req.clientIP, "KEYS", "KEYS " + pattern, success, duration);
    
    return res;
}

HttpResponse RedisApiHandler::poolStatus(const HttpRequest& req) {
    HttpResponse res;
    res.contentType = "application/json";
    
    size_t idle = DBConnection::getRedisPoolIdleCount();
    size_t active = DBConnection::getRedisPoolActiveCount();
    size_t total = idle + active;
    
    res.body = "{\"success\": true, \"idle\": " + std::to_string(idle) + 
               ", \"active\": " + std::to_string(active) + 
               ", \"total\": " + std::to_string(total) + 
               ", \"maxActive\": " + std::to_string(DBConnection::getRedisPoolMaxActive()) +
               ", \"minIdlePercent\": " + std::to_string(DBConnection::getRedisPoolMinIdlePercent()) +
               ", \"currentMinIdle\": " + std::to_string(DBConnection::getRedisPoolCurrentMinIdle()) +
               ", \"maxWaitMs\": " + std::to_string(DBConnection::getRedisPoolMaxWaitMs()) +
               ", \"idleTimeoutMs\": " + std::to_string(DBConnection::getRedisPoolIdleTimeoutMs()) + "}";
    
    return res;
}

HttpResponse RedisApiHandler::updatePoolConfig(const HttpRequest& req) {
    HttpResponse res;
    res.contentType = "application/json";
    
    try {
        auto maxActiveIt = req.params.find("maxActive");
        auto minIdleIt = req.params.find("minIdle");
        auto maxWaitMsIt = req.params.find("maxWaitMs");
        auto idleTimeoutMsIt = req.params.find("idleTimeoutMs");
        
        if (maxActiveIt != req.params.end()) {
            int maxActive = std::stoi(maxActiveIt->second);
            if (!DBConnection::setRedisPoolMaxActive(maxActive)) {
                res.statusCode = 400;
                res.body = "{\"success\": false, \"message\": \"最大连接数不能小于当前活跃连接数\"}";
                return res;
            }
        }
        
        if (minIdleIt != req.params.end()) {
            int minIdlePercent = std::stoi(minIdleIt->second);
            DBConnection::setRedisPoolMinIdlePercent(minIdlePercent);
        }
        
        if (maxWaitMsIt != req.params.end()) {
            int maxWaitMs = std::stoi(maxWaitMsIt->second);
            DBConnection::setRedisPoolMaxWaitMs(maxWaitMs);
        }
        
        if (idleTimeoutMsIt != req.params.end()) {
            int idleTimeoutMs = std::stoi(idleTimeoutMsIt->second);
            DBConnection::setRedisPoolIdleTimeoutMs(idleTimeoutMs);
        }
        
        res.body = "{\"success\": true, \"message\": \"Redis连接池配置更新成功\"}";
    } catch (...) {
        res.statusCode = 500;
        res.body = "{\"success\": false, \"message\": \"配置更新失败\"}";
    }
    
    return res;
}

}
