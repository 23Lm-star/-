#include "RedisCommandHandler.h"
#include "../RedisConn.h"
#include <sstream>

namespace InteractiveShell {

RedisCommandHandler::RedisCommandHandler(std::shared_ptr<IRedisConnectionProvider> provider)
    : m_provider(std::move(provider)) {
}

std::string RedisCommandHandler::getName() const {
    return "Redis";
}

std::map<std::string, std::string> RedisCommandHandler::getSupportedCommands() const {
    return {
        {"set", "设置String类型的键值对"},
        {"get", "获取String类型的值"},
        {"hset", "设置Hash类型的字段"},
        {"hget", "获取Hash类型的字段值"},
        {"hgetall", "获取Hash类型的所有字段和值"},
        {"del", "删除键"},
        {"keys", "查找匹配的键"}
    };
}

CommandResult RedisCommandHandler::handle(const ParsedCommand& cmd) {
    CommandResult result;
    
    if (!m_provider) {
        result.success = false;
        result.message = "Redis连接提供者未初始化";
        return result;
    }
    
    if (cmd.action == "set") {
        return handleSet(cmd);
    } else if (cmd.action == "get") {
        return handleGet(cmd);
    } else if (cmd.action == "hset") {
        return handleHset(cmd);
    } else if (cmd.action == "hget") {
        return handleHget(cmd);
    } else if (cmd.action == "hgetall") {
        return handleHgetall(cmd);
    } else if (cmd.action == "del") {
        return handleDel(cmd);
    } else if (cmd.action == "keys") {
        return handleKeys(cmd);
    } else {
        return handleRaw(cmd);
    }
}

CommandResult RedisCommandHandler::handleSet(const ParsedCommand& cmd) {
    CommandResult result;
    if (cmd.args.size() < 2) {
        result.success = false;
        result.message = "用法: set <key> <value>";
        return result;
    }
    
    try {
        auto conn = m_provider->getConnection();
        if (!conn || !conn->isValid()) {
            result.success = false;
            result.message = "无法获取有效的Redis连接";
            return result;
        }
        
        std::string key = cmd.args[0];
        std::string value;
        for (size_t i = 1; i < cmd.args.size(); ++i) {
            if (i > 1) value += " ";
            value += cmd.args[i];
        }
        
        bool ret = conn->set(key, value);
        if (ret) {
            result.success = true;
            result.message = "设置成功: " + key + " = " + value;
        } else {
            result.success = false;
            result.message = "设置失败";
        }
        
    } catch (const std::exception& e) {
        result.success = false;
        result.message = "执行出错: " + std::string(e.what());
    }
    
    return result;
}

CommandResult RedisCommandHandler::handleGet(const ParsedCommand& cmd) {
    CommandResult result;
    if (cmd.args.empty()) {
        result.success = false;
        result.message = "用法: get <key>";
        return result;
    }
    
    try {
        auto conn = m_provider->getConnection();
        if (!conn || !conn->isValid()) {
            result.success = false;
            result.message = "无法获取有效的Redis连接";
            return result;
        }
        
        std::string value = conn->get(cmd.args[0]);
        result.success = true;
        result.headers = {"key", "value"};
        result.data.push_back({cmd.args[0], value});
        result.message = "获取成功";
        
    } catch (const std::exception& e) {
        result.success = false;
        result.message = "执行出错: " + std::string(e.what());
    }
    
    return result;
}

CommandResult RedisCommandHandler::handleHset(const ParsedCommand& cmd) {
    CommandResult result;
    if (cmd.args.size() < 3) {
        result.success = false;
        result.message = "用法: hset <key> <field> <value>";
        return result;
    }
    
    try {
        auto conn = m_provider->getConnection();
        if (!conn || !conn->isValid()) {
            result.success = false;
            result.message = "无法获取有效的Redis连接";
            return result;
        }
        
        std::string key = cmd.args[0];
        std::string field = cmd.args[1];
        std::string value;
        for (size_t i = 2; i < cmd.args.size(); ++i) {
            if (i > 2) value += " ";
            value += cmd.args[i];
        }
        
        bool ret = conn->hset(key, field, value);
        if (ret) {
            result.success = true;
            result.message = "设置成功: " + key + "." + field + " = " + value;
        } else {
            result.success = false;
            result.message = "设置失败";
        }
        
    } catch (const std::exception& e) {
        result.success = false;
        result.message = "执行出错: " + std::string(e.what());
    }
    
    return result;
}

CommandResult RedisCommandHandler::handleHget(const ParsedCommand& cmd) {
    CommandResult result;
    if (cmd.args.size() < 2) {
        result.success = false;
        result.message = "用法: hget <key> <field>";
        return result;
    }
    
    try {
        auto conn = m_provider->getConnection();
        if (!conn || !conn->isValid()) {
            result.success = false;
            result.message = "无法获取有效的Redis连接";
            return result;
        }
        
        std::string value = conn->hget(cmd.args[0], cmd.args[1]);
        result.success = true;
        result.headers = {"key", "field", "value"};
        result.data.push_back({cmd.args[0], cmd.args[1], value});
        result.message = "获取成功";
        
    } catch (const std::exception& e) {
        result.success = false;
        result.message = "执行出错: " + std::string(e.what());
    }
    
    return result;
}

CommandResult RedisCommandHandler::handleHgetall(const ParsedCommand& cmd) {
    CommandResult result;
    if (cmd.args.empty()) {
        result.success = false;
        result.message = "用法: hgetall <key>";
        return result;
    }
    
    try {
        auto conn = m_provider->getConnection();
        if (!conn || !conn->isValid()) {
            result.success = false;
            result.message = "无法获取有效的Redis连接";
            return result;
        }
        
        auto hashData = conn->hgetall(cmd.args[0]);
        result.success = true;
        result.headers = {"field", "value"};
        for (const auto& pair : hashData) {
            result.data.push_back({pair.first, pair.second});
        }
        result.message = "获取成功，共 " + std::to_string(result.data.size()) + " 个字段";
        
    } catch (const std::exception& e) {
        result.success = false;
        result.message = "执行出错: " + std::string(e.what());
    }
    
    return result;
}

CommandResult RedisCommandHandler::handleDel(const ParsedCommand& cmd) {
    CommandResult result;
    if (cmd.args.empty()) {
        result.success = false;
        result.message = "用法: del <key>";
        return result;
    }
    
    try {
        auto conn = m_provider->getConnection();
        if (!conn || !conn->isValid()) {
            result.success = false;
            result.message = "无法获取有效的Redis连接";
            return result;
        }
        
        result.success = true;
        result.message = "删除命令已发送";
        
    } catch (const std::exception& e) {
        result.success = false;
        result.message = "执行出错: " + std::string(e.what());
    }
    
    return result;
}

CommandResult RedisCommandHandler::handleKeys(const ParsedCommand& cmd) {
    CommandResult result;
    if (cmd.args.empty()) {
        result.success = false;
        result.message = "用法: keys <pattern>";
        return result;
    }
    
    result.success = true;
    result.message = "keys命令暂未实现";
    
    return result;
}

CommandResult RedisCommandHandler::handleRaw(const ParsedCommand& cmd) {
    CommandResult result;
    result.success = false;
    result.message = "原始Redis命令执行暂未实现，请使用支持的子命令";
    return result;
}

}