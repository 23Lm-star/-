#include "MysqlApiHandler.h"
#include "../auth/AuthHandler.h"
#include "../auth/SessionManager.h"
#include "../Logger.h"
#include <mysql/mysql.h>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <cstring>
#include <chrono>

namespace WebServer {

// 用户登录API实现
HttpResponse MysqlApiHandler::login(const HttpRequest& req) {
    HttpResponse res;
    res.contentType = "application/json";
    
    try {
        std::string username = req.params.at("username");
        std::string password = req.params.at("password");
        
        if (Auth::AuthHandler::validateUser(username, password)) {
            // 验证成功，创建会话
            std::string sessionId = Auth::SessionManager::instance().createSession(username);
            
            res.statusCode = 200;
            res.body = "{\"success\": true, \"message\": \"登录成功\"}";
            // 设置Cookie
            res.headers["Set-Cookie"] = "session_id=" + sessionId + "; HttpOnly; Path=/";
        } else {
            // 验证失败
            res.statusCode = 401;
            res.body = "{\"success\": false, \"message\": \"用户名或密码错误\"}";
        }
    } catch (const std::exception& e) {
        res.statusCode = 400;
        res.body = "{\"success\": false, \"message\": \"参数错误\"}";
    }
    
    return res;
}

// 用户登出API实现
HttpResponse MysqlApiHandler::logout(const HttpRequest& req) {
    HttpResponse res;
    res.contentType = "application/json";
    
    // 从Cookie中获取session_id并销毁
    auto it = req.headers.find("Cookie");
    if (it != req.headers.end()) {
        std::string cookies = it->second;
        std::string target = "session_id=";
        size_t pos = cookies.find(target);
        if (pos != std::string::npos) {
            size_t start = pos + target.length();
            size_t end = cookies.find(';', start);
            std::string sessionId;
            if (end == std::string::npos) {
                sessionId = cookies.substr(start);
            } else {
                sessionId = cookies.substr(start, end - start);
            }
            Auth::SessionManager::instance().destroySession(sessionId);
        }
    }
    
    res.statusCode = 200;
    res.body = "{\"success\": true, \"message\": \"已登出\"}";
    // 清除Cookie
    res.headers["Set-Cookie"] = "session_id=; HttpOnly; Path=/; Max-Age=0";
    
    return res;
}

// 用户注册API
HttpResponse MysqlApiHandler::registerUser(const HttpRequest& req) {
    HttpResponse res;
    res.contentType = "application/json";
    
    try {
        std::string username = req.params.at("username");
        std::string password = req.params.at("password");
        
        // 验证用户名格式
        if (username.empty() || username.length() < 3) {
            res.statusCode = 400;
            res.body = "{\"success\": false, \"message\": \"用户名至少需要3个字符\"}";
            return res;
        }
        
        // 验证密码强度
        if (password.empty() || password.length() < 6) {
            res.statusCode = 400;
            res.body = "{\"success\": false, \"message\": \"密码至少需要6个字符\"}";
            return res;
        }
        
        // 添加用户（状态为pending）
        if (Auth::AuthHandler::addUser(username, password)) {
            res.statusCode = 200;
            res.body = "{\"success\": true, \"message\": \"注册成功，请等待管理员审批\"}";
        } else {
            res.statusCode = 400;
            res.body = "{\"success\": false, \"message\": \"用户名已存在\"}";
        }
    } catch (const std::exception& e) {
        res.statusCode = 400;
        res.body = "{\"success\": false, \"message\": \"参数错误\"}";
    }
    
    return res;
}

// 获取当前用户（包含角色信息）
HttpResponse MysqlApiHandler::currentUser(const HttpRequest& req) {
    HttpResponse res;
    res.contentType = "application/json";
    
    // 从Cookie中获取session_id
    auto it = req.headers.find("Cookie");
    if (it != req.headers.end()) {
        std::string cookies = it->second;
        std::string target = "session_id=";
        size_t pos = cookies.find(target);
        if (pos != std::string::npos) {
            size_t start = pos + target.length();
            size_t end = cookies.find(';', start);
            std::string sessionId;
            if (end == std::string::npos) {
                sessionId = cookies.substr(start);
            } else {
                sessionId = cookies.substr(start, end - start);
            }
            
            // 验证会话并获取用户名
            std::string username = Auth::SessionManager::instance().getSessionUser(sessionId);
            if (!username.empty()) {
                std::string role = Auth::AuthHandler::getUserRole(username);
                res.statusCode = 200;
                res.body = "{\"success\": true, \"user\": \"" + username + "\", \"role\": \"" + role + "\"}";
                return res;
            }
        }
    }
    
    // 未登录或会话无效
    res.statusCode = 401;
    res.body = "{\"success\": false, \"message\": \"未登录\"}";
    return res;
}

// 获取所有用户列表（超级管理员专用）
HttpResponse MysqlApiHandler::getUsers(const HttpRequest& req) {
    HttpResponse res;
    res.contentType = "application/json";
    
    // 验证会话并获取用户名
    std::string username = getUsernameFromSession(req);
    if (username.empty()) {
        res.statusCode = 401;
        res.body = "{\"success\": false, \"message\": \"未登录\"}";
        return res;
    }
    
    // 验证是否为超级管理员
    if (!Auth::AuthHandler::isSuperAdmin(username)) {
        res.statusCode = 403;
        res.body = "{\"success\": false, \"message\": \"权限不足\"}";
        return res;
    }
    
    // 获取用户列表
    std::vector<Auth::UserInfo> users = Auth::AuthHandler::getUsers();
    
    // 构建JSON响应
    std::string json = "{\"success\": true, \"data\": [";
    for (size_t i = 0; i < users.size(); ++i) {
        if (i > 0) json += ",";
        json += "{\"id\": " + std::to_string(users[i].id) + ",";
        json += "\"username\": \"" + users[i].username + "\",";
        json += "\"role\": \"" + users[i].role + "\",";
        json += "\"status\": \"" + users[i].status + "\",";
        json += "\"createdAt\": \"" + users[i].createdAt + "\"}";
    }
    json += "]}";
    
    res.statusCode = 200;
    res.body = json;
    return res;
}

// 获取待审批用户列表（超级管理员专用）
HttpResponse MysqlApiHandler::getPendingUsers(const HttpRequest& req) {
    HttpResponse res;
    res.contentType = "application/json";
    
    // 验证会话并获取用户名
    std::string username = getUsernameFromSession(req);
    if (username.empty()) {
        res.statusCode = 401;
        res.body = "{\"success\": false, \"message\": \"未登录\"}";
        return res;
    }
    
    // 验证是否为超级管理员
    if (!Auth::AuthHandler::isSuperAdmin(username)) {
        res.statusCode = 403;
        res.body = "{\"success\": false, \"message\": \"权限不足\"}";
        return res;
    }
    
    // 获取待审批用户列表
    std::vector<Auth::UserInfo> users = Auth::AuthHandler::getPendingUsers();
    
    // 构建JSON响应
    std::string json = "{\"success\": true, \"data\": [";
    for (size_t i = 0; i < users.size(); ++i) {
        if (i > 0) json += ",";
        json += "{\"id\": " + std::to_string(users[i].id) + ",";
        json += "\"username\": \"" + users[i].username + "\",";
        json += "\"role\": \"" + users[i].role + "\",";
        json += "\"status\": \"" + users[i].status + "\",";
        json += "\"createdAt\": \"" + users[i].createdAt + "\"}";
    }
    json += "]}";
    
    res.statusCode = 200;
    res.body = json;
    return res;
}

// 审批用户（超级管理员专用）
HttpResponse MysqlApiHandler::approveUser(const HttpRequest& req) {
    HttpResponse res;
    res.contentType = "application/json";
    
    // 验证会话并获取用户名
    std::string username = getUsernameFromSession(req);
    if (username.empty()) {
        res.statusCode = 401;
        res.body = "{\"success\": false, \"message\": \"未登录\"}";
        return res;
    }
    
    // 验证是否为超级管理员
    if (!Auth::AuthHandler::isSuperAdmin(username)) {
        res.statusCode = 403;
        res.body = "{\"success\": false, \"message\": \"权限不足\"}";
        return res;
    }
    
    try {
        int userId = std::stoi(req.params.at("userId"));
        
        if (Auth::AuthHandler::updateUserStatus(userId, "approved")) {
            res.statusCode = 200;
            res.body = "{\"success\": true, \"message\": \"审批通过\"}";
        } else {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"审批失败\"}";
        }
    } catch (const std::exception& e) {
        res.statusCode = 400;
        res.body = "{\"success\": false, \"message\": \"参数错误\"}";
    }
    
    return res;
}

// 拒绝用户注册（超级管理员专用）
HttpResponse MysqlApiHandler::rejectUser(const HttpRequest& req) {
    HttpResponse res;
    res.contentType = "application/json";
    
    // 验证会话并获取用户名
    std::string username = getUsernameFromSession(req);
    if (username.empty()) {
        res.statusCode = 401;
        res.body = "{\"success\": false, \"message\": \"未登录\"}";
        return res;
    }
    
    // 验证是否为超级管理员
    if (!Auth::AuthHandler::isSuperAdmin(username)) {
        res.statusCode = 403;
        res.body = "{\"success\": false, \"message\": \"权限不足\"}";
        return res;
    }
    
    try {
        int userId = std::stoi(req.params.at("userId"));
        
        if (Auth::AuthHandler::updateUserStatus(userId, "rejected")) {
            res.statusCode = 200;
            res.body = "{\"success\": true, \"message\": \"已拒绝\"}";
        } else {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"操作失败\"}";
        }
    } catch (const std::exception& e) {
        res.statusCode = 400;
        res.body = "{\"success\": false, \"message\": \"参数错误\"}";
    }
    
    return res;
}

// 删除用户（超级管理员专用）
HttpResponse MysqlApiHandler::deleteUser(const HttpRequest& req) {
    HttpResponse res;
    res.contentType = "application/json";
    
    // 验证会话并获取用户名
    std::string username = getUsernameFromSession(req);
    if (username.empty()) {
        res.statusCode = 401;
        res.body = "{\"success\": false, \"message\": \"未登录\"}";
        return res;
    }
    
    // 验证是否为超级管理员
    if (!Auth::AuthHandler::isSuperAdmin(username)) {
        res.statusCode = 403;
        res.body = "{\"success\": false, \"message\": \"权限不足\"}";
        return res;
    }
    
    try {
        int userId = std::stoi(req.params.at("userId"));
        
        if (Auth::AuthHandler::deleteUser(userId)) {
            res.statusCode = 200;
            res.body = "{\"success\": true, \"message\": \"删除成功\"}";
        } else {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"删除失败\"}";
        }
    } catch (const std::exception& e) {
        res.statusCode = 400;
        res.body = "{\"success\": false, \"message\": \"参数错误\"}";
    }
    
    return res;
}

// 修改密码
HttpResponse MysqlApiHandler::changePassword(const HttpRequest& req) {
    HttpResponse res;
    res.contentType = "application/json";
    
    // 验证会话并获取用户名
    std::string currentUser = getUsernameFromSession(req);
    if (currentUser.empty()) {
        res.statusCode = 401;
        res.body = "{\"success\": false, \"message\": \"未登录\"}";
        return res;
    }
    
    try {
        std::string targetUsername = req.params.at("username");
        std::string newPassword = req.params.at("password");
        
        // 验证密码强度
        if (newPassword.empty() || newPassword.length() < 6) {
            res.statusCode = 400;
            res.body = "{\"success\": false, \"message\": \"密码至少需要6个字符\"}";
            return res;
        }
        
        // 如果修改的是自己的密码，直接修改
        if (targetUsername == currentUser) {
            if (Auth::AuthHandler::changePassword(targetUsername, newPassword)) {
                res.statusCode = 200;
                res.body = "{\"success\": true, \"message\": \"密码修改成功\"}";
            } else {
                res.statusCode = 500;
                res.body = "{\"success\": false, \"message\": \"密码修改失败\"}";
            }
            return res;
        }
        
        // 如果修改的是其他用户的密码，需要超级管理员权限
        if (!Auth::AuthHandler::isSuperAdmin(currentUser)) {
            res.statusCode = 403;
            res.body = "{\"success\": false, \"message\": \"权限不足\"}";
            return res;
        }
        
        // 超级管理员修改其他用户密码
        if (Auth::AuthHandler::changePassword(targetUsername, newPassword)) {
            res.statusCode = 200;
            res.body = "{\"success\": true, \"message\": \"密码修改成功\"}";
        } else {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"密码修改失败\"}";
        }
    } catch (const std::exception& e) {
        res.statusCode = 400;
        res.body = "{\"success\": false, \"message\": \"参数错误\"}";
    }
    
    return res;
}

// 从请求中获取用户名（辅助函数）
std::string MysqlApiHandler::getUsernameFromSession(const HttpRequest& req) {
    auto it = req.headers.find("Cookie");
    if (it != req.headers.end()) {
        std::string cookies = it->second;
        std::string target = "session_id=";
        size_t pos = cookies.find(target);
        if (pos != std::string::npos) {
            size_t start = pos + target.length();
            size_t end = cookies.find(';', start);
            std::string sessionId;
            if (end == std::string::npos) {
                sessionId = cookies.substr(start);
            } else {
                sessionId = cookies.substr(start, end - start);
            }
            return Auth::SessionManager::instance().getSessionUser(sessionId);
        }
    }
    return "";
}

HttpResponse MysqlApiHandler::getTables(const HttpRequest& req) {
    HttpResponse res;
    auto startTime = std::chrono::steady_clock::now();
    bool success = false;
    
    try {
        auto conn = DBConnection::getMysqlConnection();
        if (!conn.isValid()) {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"无法获取MySQL连接\"}";
            return res;
        }
        
        MYSQL_RES* result = conn->executeQuery("SHOW TABLES");
        if (!result) {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"查询失败\"}";
            return res;
        }
        
        std::vector<std::string> tables;
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result))) {
            std::string tableName = row[0] ? row[0] : "";
            // 过滤掉sys_users表，该表只能在用户管理模块中由超级管理员查看
            if (tableName != "sys_users") {
                tables.push_back(tableName);
            }
        }
        mysql_free_result(result);
        
        std::ostringstream oss;
        oss << "{\"success\": true, \"tables\": [";
        for (size_t i = 0; i < tables.size(); ++i) {
            if (i > 0) oss << ",";
            oss << "\"" << tables[i] << "\"";
        }
        oss << "]}";
        
        res.contentType = "application/json";
        res.body = oss.str();
        success = true;
    } catch (const std::exception& e) {
        res.statusCode = 500;
        res.body = "{\"success\": false, \"message\": \"服务器内部错误: " + std::string(e.what()) + "\"}";
    } catch (...) {
        res.statusCode = 500;
        res.body = "{\"success\": false, \"message\": \"服务器内部错误\"}";
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    Logger::getInstance().log(Logger::LogType::MYSQL, req.clientIP, "GET_TABLES", "SHOW TABLES", success, duration);
    
    return res;
}

HttpResponse MysqlApiHandler::getTableData(const HttpRequest& req) {
    HttpResponse res;
    auto startTime = std::chrono::steady_clock::now();
    bool success = false;
    std::string sql;
    
    auto it = req.params.find("table");
    if (it == req.params.end()) {
        res.statusCode = 400;
        res.body = "{\"success\": false, \"message\": \"缺少table参数\"}";
        return res;
    }
    
    std::string table = it->second;
    
    try {
        auto conn = DBConnection::getMysqlConnection();
        if (!conn.isValid()) {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"无法获取MySQL连接\"}";
            return res;
        }
        
        std::string primaryKey = "";
        std::string descSql = "DESCRIBE " + table;
        MYSQL_RES* descResult = conn->executeQuery(descSql);
        if (descResult) {
            MYSQL_ROW descRow;
            while ((descRow = mysql_fetch_row(descResult))) {
                if (descRow[3] && strcmp(descRow[3], "PRI") == 0) {
                    primaryKey = descRow[0];
                    break;
                }
            }
            mysql_free_result(descResult);
        }
        
        sql = "SELECT * FROM " + table;
        MYSQL_RES* result = conn->executeQuery(sql);
        if (!result) {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"查询失败\"}";
            return res;
        }
        
        MYSQL_FIELD* fields = mysql_fetch_fields(result);
        unsigned int numFields = mysql_num_fields(result);
        
        std::vector<std::string> headers;
        std::vector<int> fieldOrder;
        int primaryIndex = -1;
        
        for (unsigned int i = 0; i < numFields; ++i) {
            headers.push_back(fields[i].name);
            fieldOrder.push_back(i);
            if (primaryKey == fields[i].name) {
                primaryIndex = i;
            }
        }
        
        if (primaryIndex != -1 && primaryIndex != 0) {
            std::swap(fieldOrder[0], fieldOrder[primaryIndex]);
        }
        
        std::vector<std::vector<std::string>> data;
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result))) {
            std::vector<std::string> rowData;
            for (unsigned int i = 0; i < numFields; ++i) {
                int originalIndex = fieldOrder[i];
                rowData.push_back(row[originalIndex] ? row[originalIndex] : "NULL");
            }
            data.push_back(rowData);
        }
        mysql_free_result(result);
        
        if (primaryIndex != -1 && primaryIndex != 0) {
            std::swap(headers[0], headers[primaryIndex]);
        }
        
        res.contentType = "application/json";
        res.body = "{\"success\": true, \"headers\": [";
        for (size_t i = 0; i < headers.size(); ++i) {
            if (i > 0) res.body += ",";
            res.body += "\"" + headers[i] + "\"";
        }
        res.body += "], \"data\": [";
        
        for (size_t i = 0; i < data.size(); ++i) {
            if (i > 0) res.body += ",";
            res.body += "[";
            for (size_t j = 0; j < data[i].size(); ++j) {
                if (j > 0) res.body += ",";
                res.body += "\"" + data[i][j] + "\"";
            }
            res.body += "]";
        }
        res.body += "], \"primaryKey\": \"" + primaryKey + "\"}";
        success = true;
    } catch (...) {
        res.statusCode = 500;
        res.body = "{\"success\": false, \"message\": \"服务器内部错误\"}";
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    Logger::getInstance().log(Logger::LogType::MYSQL, req.clientIP, "SELECT", sql, success, duration);
    
    return res;
}

HttpResponse MysqlApiHandler::getTableStructure(const HttpRequest& req) {
    HttpResponse res;
    auto startTime = std::chrono::steady_clock::now();
    bool success = false;
    std::string sql;
    
    auto it = req.params.find("table");
    if (it == req.params.end()) {
        res.statusCode = 400;
        res.body = "{\"success\": false, \"message\": \"缺少table参数\"}";
        return res;
    }
    
    std::string table = it->second;
    
    try {
        auto conn = DBConnection::getMysqlConnection();
        if (!conn.isValid()) {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"无法获取MySQL连接\"}";
            return res;
        }
        
        sql = "DESCRIBE " + table;
        MYSQL_RES* result = conn->executeQuery(sql);
        if (!result) {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"查询失败\"}";
            return res;
        }
        
        std::vector<std::string> headers = {"字段", "类型", "是否为空", "键", "默认值", "额外"};
        std::vector<std::vector<std::string>> data;
        
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result))) {
            std::vector<std::string> rowData;
            for (unsigned int i = 0; i < 6; ++i) {
                rowData.push_back(row[i] ? row[i] : "");
            }
            data.push_back(rowData);
        }
        mysql_free_result(result);
        
        res.contentType = "application/json";
        res.body = toJson(data, headers);
        success = true;
    } catch (...) {
        res.statusCode = 500;
        res.body = "{\"success\": false, \"message\": \"服务器内部错误\"}";
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    Logger::getInstance().log(Logger::LogType::MYSQL, req.clientIP, "DESCRIBE", sql, success, duration);
    
    return res;
}

HttpResponse MysqlApiHandler::insertRow(const HttpRequest& req) {
    HttpResponse res;
    auto startTime = std::chrono::steady_clock::now();
    bool success = false;
    std::string sql;
    
    auto tableIt = req.params.find("table");
    auto dataIt = req.params.find("data");
    
    if (tableIt == req.params.end() || dataIt == req.params.end()) {
        res.statusCode = 400;
        res.body = "{\"success\": false, \"message\": \"缺少参数\"}";
        return res;
    }
    
    try {
        auto conn = DBConnection::getMysqlConnection();
        if (!conn.isValid()) {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"无法获取MySQL连接\"}";
            return res;
        }
        
        sql = "INSERT INTO " + tableIt->second + " SET " + dataIt->second;
        bool ret = conn->executeUpdate(sql);
        
        if (ret) {
            res.contentType = "application/json";
            res.body = "{\"success\": true, \"message\": \"插入成功\", \"affectedRows\": " 
                       + std::to_string(conn->getAffectedRows()) + "}";
            success = true;
        } else {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"插入失败\"}";
        }
    } catch (...) {
        res.statusCode = 500;
        res.body = "{\"success\": false, \"message\": \"服务器内部错误\"}";
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    Logger::getInstance().log(Logger::LogType::MYSQL, req.clientIP, "INSERT", sql, success, duration);
    
    return res;
}

HttpResponse MysqlApiHandler::updateRow(const HttpRequest& req) {
    HttpResponse res;
    auto startTime = std::chrono::steady_clock::now();
    bool success = false;
    std::string sql;
    
    auto tableIt = req.params.find("table");
    auto dataIt = req.params.find("data");
    auto whereIt = req.params.find("where");
    
    if (tableIt == req.params.end() || dataIt == req.params.end() || whereIt == req.params.end()) {
        res.statusCode = 400;
        res.body = "{\"success\": false, \"message\": \"缺少参数\"}";
        return res;
    }
    
    try {
        auto conn = DBConnection::getMysqlConnection();
        if (!conn.isValid()) {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"无法获取MySQL连接\"}";
            return res;
        }
        
        sql = "UPDATE " + tableIt->second + " SET " + dataIt->second + " WHERE " + whereIt->second;
        bool ret = conn->executeUpdate(sql);
        
        if (ret) {
            res.contentType = "application/json";
            res.body = "{\"success\": true, \"message\": \"更新成功\", \"affectedRows\": " 
                       + std::to_string(conn->getAffectedRows()) + "}";
            success = true;
        } else {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"更新失败\"}";
        }
    } catch (...) {
        res.statusCode = 500;
        res.body = "{\"success\": false, \"message\": \"服务器内部错误\"}";
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    Logger::getInstance().log(Logger::LogType::MYSQL, req.clientIP, "UPDATE", sql, success, duration);
    
    return res;
}

HttpResponse MysqlApiHandler::deleteRow(const HttpRequest& req) {
    HttpResponse res;
    auto startTime = std::chrono::steady_clock::now();
    bool success = false;
    std::string sql;
    
    auto tableIt = req.params.find("table");
    auto whereIt = req.params.find("where");
    
    if (tableIt == req.params.end() || whereIt == req.params.end()) {
        res.statusCode = 400;
        res.body = "{\"success\": false, \"message\": \"缺少参数\"}";
        return res;
    }
    
    try {
        auto conn = DBConnection::getMysqlConnection();
        if (!conn.isValid()) {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"无法获取MySQL连接\"}";
            return res;
        }
        
        sql = "DELETE FROM " + tableIt->second + " WHERE " + whereIt->second;
        bool ret = conn->executeUpdate(sql);
        
        if (ret) {
            res.contentType = "application/json";
            res.body = "{\"success\": true, \"message\": \"删除成功\", \"affectedRows\": " 
                       + std::to_string(conn->getAffectedRows()) + "}";
            success = true;
        } else {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"删除失败\"}";
        }
    } catch (...) {
        res.statusCode = 500;
        res.body = "{\"success\": false, \"message\": \"服务器内部错误\"}";
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    Logger::getInstance().log(Logger::LogType::MYSQL, req.clientIP, "DELETE", sql, success, duration);
    
    return res;
}

HttpResponse MysqlApiHandler::createTable(const HttpRequest& req) {
    HttpResponse res;
    auto startTime = std::chrono::steady_clock::now();
    bool success = false;
    std::string sql;
    
    try {
        auto conn = DBConnection::getMysqlConnection();
        if (!conn.isValid()) {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"无法获取MySQL连接\"}";
            return res;
        }
        
        std::string tableName;
        std::vector<std::string> fields;
        
        std::string body = req.body;
        size_t tablePos = body.find("\"tableName\":\"");
        if (tablePos != std::string::npos) {
            size_t start = tablePos + 13;
            size_t end = body.find("\"", start);
            if (end != std::string::npos) {
                tableName = body.substr(start, end - start);
            }
        }
        
        size_t fieldsPos = body.find("\"fields\":[");
        if (fieldsPos != std::string::npos) {
            size_t fieldsEnd = body.find("]", fieldsPos);
            if (fieldsEnd != std::string::npos) {
                std::string fieldsStr = body.substr(fieldsPos + 9, fieldsEnd - fieldsPos - 9);
                size_t fieldStart = 0;
                while (fieldStart != std::string::npos) {
                    size_t objStart = fieldsStr.find("{", fieldStart);
                    if (objStart == std::string::npos) break;
                    
                    size_t namePos = fieldsStr.find("\"name\":\"", objStart);
                    size_t typePos = fieldsStr.find("\"type\":\"", objStart);
                    size_t nullablePos = fieldsStr.find("\"nullable\":", objStart);
                    size_t primaryPos = fieldsStr.find("\"primary\":", objStart);
                    size_t autoincPos = fieldsStr.find("\"autoinc\":", objStart);
                    size_t uniquePos = fieldsStr.find("\"unique\":", objStart);
                    size_t defaultPos = fieldsStr.find("\"default\":", objStart);
                    
                    std::string name, type, defaultValue;
                    bool nullable = true, primary = false, autoinc = false, unique = false;
                    
                    if (namePos != std::string::npos) {
                        size_t start = namePos + 8;
                        size_t end = fieldsStr.find("\"", start);
                        if (end != std::string::npos) name = fieldsStr.substr(start, end - start);
                    }
                    
                    if (typePos != std::string::npos) {
                        size_t start = typePos + 8;
                        size_t end = fieldsStr.find("\"", start);
                        if (end != std::string::npos) type = fieldsStr.substr(start, end - start);
                    }
                    
                    if (nullablePos != std::string::npos) {
                        size_t start = nullablePos + 11;
                        nullable = (fieldsStr.substr(start, 5) == "true");
                    }
                    
                    if (primaryPos != std::string::npos) {
                        size_t start = primaryPos + 11;
                        primary = (fieldsStr.substr(start, 5) == "true");
                    }
                    
                    if (autoincPos != std::string::npos) {
                        size_t start = autoincPos + 12;
                        autoinc = (fieldsStr.substr(start, 5) == "true");
                    }
                    
                    if (uniquePos != std::string::npos) {
                        size_t start = uniquePos + 11;
                        unique = (fieldsStr.substr(start, 5) == "true");
                    }
                    
                    if (defaultPos != std::string::npos) {
                        size_t start = defaultPos + 11;
                        if (fieldsStr.substr(start, 4) != "null") {
                            size_t end = fieldsStr.find("\"", start + 1);
                            if (end != std::string::npos) {
                                defaultValue = fieldsStr.substr(start + 1, end - start - 1);
                            }
                        }
                    }
                    
                    if (!name.empty() && !type.empty()) {
                        std::string fieldDef = name + " " + type;
                        if (primary) {
                            fieldDef += " PRIMARY KEY";
                            if (autoinc) {
                                fieldDef += " AUTO_INCREMENT";
                            }
                        } else {
                            if (!nullable) {
                                fieldDef += " NOT NULL";
                            }
                            if (autoinc) {
                                fieldDef += " AUTO_INCREMENT";
                            }
                            if (unique) {
                                fieldDef += " UNIQUE";
                            }
                            if (!defaultValue.empty()) {
                                fieldDef += " DEFAULT '" + defaultValue + "'";
                            }
                        }
                        fields.push_back(fieldDef);
                    }
                    
                    fieldStart = fieldsStr.find("}", objStart) + 1;
                }
            }
        }
        
        if (tableName.empty() || fields.empty()) {
            res.statusCode = 400;
            res.body = "{\"success\": false, \"message\": \"缺少表名或字段信息\"}";
            return res;
        }
        
        sql = "CREATE TABLE " + tableName + " (";
        for (size_t i = 0; i < fields.size(); ++i) {
            if (i > 0) sql += ", ";
            sql += fields[i];
        }
        sql += ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
        
        bool ret = conn->executeUpdate(sql);
        
        if (ret) {
            res.contentType = "application/json";
            res.body = "{\"success\": true, \"message\": \"表创建成功\"}";
            success = true;
        } else {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"表创建失败\"}";
        }
    } catch (...) {
        res.statusCode = 500;
        res.body = "{\"success\": false, \"message\": \"服务器内部错误\"}";
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    Logger::getInstance().log(Logger::LogType::MYSQL, req.clientIP, "CREATE_TABLE", sql, success, duration);
    
    return res;
}

HttpResponse MysqlApiHandler::dropTable(const HttpRequest& req) {
    HttpResponse res;
    auto startTime = std::chrono::steady_clock::now();
    bool success = false;
    std::string sql;
    
    auto it = req.params.find("table");
    if (it == req.params.end()) {
        res.statusCode = 400;
        res.body = "{\"success\": false, \"message\": \"缺少table参数\"}";
        return res;
    }
    
    try {
        auto conn = DBConnection::getMysqlConnection();
        if (!conn.isValid()) {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"无法获取MySQL连接\"}";
            return res;
        }
        
        sql = "DROP TABLE " + it->second;
        bool ret = conn->executeUpdate(sql);
        
        if (ret) {
            res.contentType = "application/json";
            res.body = "{\"success\": true, \"message\": \"表删除成功\"}";
            success = true;
        } else {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"表删除失败\"}";
        }
    } catch (...) {
        res.statusCode = 500;
        res.body = "{\"success\": false, \"message\": \"服务器内部错误\"}";
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    Logger::getInstance().log(Logger::LogType::MYSQL, req.clientIP, "DROP_TABLE", sql, success, duration);
    
    return res;
}

HttpResponse MysqlApiHandler::dropColumn(const HttpRequest& req) {
    HttpResponse res;
    auto startTime = std::chrono::steady_clock::now();
    bool success = false;
    std::string sql;
    
    auto tableIt = req.params.find("table");
    auto nameIt = req.params.find("name");
    
    if (tableIt == req.params.end() || nameIt == req.params.end()) {
        res.statusCode = 400;
        res.body = "{\"success\": false, \"message\": \"缺少参数\"}";
        return res;
    }
    
    try {
        auto conn = DBConnection::getMysqlConnection();
        if (!conn.isValid()) {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"无法获取MySQL连接\"}";
            return res;
        }
        
        std::string descSql = "DESCRIBE " + tableIt->second;
        MYSQL_RES* descResult = conn->executeQuery(descSql);
        if (descResult) {
            MYSQL_ROW descRow;
            while ((descRow = mysql_fetch_row(descResult))) {
                if (descRow[0] && strcmp(descRow[0], nameIt->second.c_str()) == 0) {
                    if (descRow[3] && strcmp(descRow[3], "PRI") == 0) {
                        mysql_free_result(descResult);
                        res.statusCode = 400;
                        res.body = "{\"success\": false, \"message\": \"主键字段不能删除\"}";
                        return res;
                    }
                    break;
                }
            }
            mysql_free_result(descResult);
        }
        
        sql = "ALTER TABLE " + tableIt->second + " DROP COLUMN " + nameIt->second;
        bool ret = conn->executeUpdate(sql);
        
        if (ret) {
            res.contentType = "application/json";
            res.body = "{\"success\": true, \"message\": \"字段删除成功\"}";
            success = true;
        } else {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"字段删除失败\"}";
        }
    } catch (...) {
        res.statusCode = 500;
        res.body = "{\"success\": false, \"message\": \"服务器内部错误\"}";
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    Logger::getInstance().log(Logger::LogType::MYSQL, req.clientIP, "DROP_COLUMN", sql, success, duration);
    
    return res;
}

HttpResponse MysqlApiHandler::addColumn(const HttpRequest& req) {
    HttpResponse res;
    auto startTime = std::chrono::steady_clock::now();
    bool success = false;
    std::string sql;
    
    auto tableIt = req.params.find("table");
    auto nameIt = req.params.find("name");
    auto typeIt = req.params.find("type");
    auto constraintsIt = req.params.find("constraints");
    auto nullableIt = req.params.find("nullable");
    
    if (tableIt == req.params.end() || nameIt == req.params.end() || typeIt == req.params.end()) {
        res.statusCode = 400;
        res.body = "{\"success\": false, \"message\": \"缺少参数\"}";
        return res;
    }
    
    try {
        auto conn = DBConnection::getMysqlConnection();
        if (!conn.isValid()) {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"无法获取MySQL连接\"}";
            return res;
        }
        
        // 检查NOT NULL约束但没有默认值的情况
        if (constraintsIt != req.params.end() && !constraintsIt->second.empty()) {
            std::string constraints = constraintsIt->second;
            bool hasNotNull = constraints.find("NOT NULL") != std::string::npos;
            bool hasDefault = constraints.find("DEFAULT") != std::string::npos;
            
            if (hasNotNull && !hasDefault) {
                res.statusCode = 400;
                res.body = "{\"success\": false, \"message\": \"NOT NULL字段必须设置默认值\"}";
                return res;
            }
        }
        
        sql = "ALTER TABLE " + tableIt->second + " ADD COLUMN " + nameIt->second + " " + typeIt->second;
        
        if (constraintsIt != req.params.end() && !constraintsIt->second.empty()) {
            sql += constraintsIt->second;
        }
        
        bool ret = conn->executeUpdate(sql);
        
        if (ret) {
            res.contentType = "application/json";
            res.body = "{\"success\": true, \"message\": \"字段添加成功\"}";
            success = true;
        } else {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"字段添加失败\"}";
        }
    } catch (...) {
        res.statusCode = 500;
        res.body = "{\"success\": false, \"message\": \"服务器内部错误\"}";
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    Logger::getInstance().log(Logger::LogType::MYSQL, req.clientIP, "ADD_COLUMN", sql, success, duration);
    
    return res;
}

HttpResponse MysqlApiHandler::setPrimaryKey(const HttpRequest& req) {
    HttpResponse res;
    auto startTime = std::chrono::steady_clock::now();
    bool success = false;
    std::string sql;
    
    auto tableIt = req.params.find("table");
    auto columnIt = req.params.find("column");
    
    if (tableIt == req.params.end() || columnIt == req.params.end()) {
        res.statusCode = 400;
        res.body = "{\"success\": false, \"message\": \"缺少参数\"}";
        return res;
    }
    
    try {
        auto conn = DBConnection::getMysqlConnection();
        if (!conn.isValid()) {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"无法获取MySQL连接\"}";
            return res;
        }
        
        // 检查表是否已有主键
        std::string checkSql = "DESCRIBE " + tableIt->second;
        MYSQL_RES* checkResult = conn->executeQuery(checkSql);
        if (!checkResult) {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"无法获取表结构\"}";
            return res;
        }
        
        bool hasExistingPrimary = false;
        bool fieldExists = false;
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(checkResult))) {
            if (row[3] && strcmp(row[3], "PRI") == 0) {
                hasExistingPrimary = true;
                break;
            }
            if (row[0] && strcmp(row[0], columnIt->second.c_str()) == 0) {
                fieldExists = true;
            }
        }
        mysql_free_result(checkResult);
        
        if (hasExistingPrimary) {
            res.statusCode = 400;
            res.body = "{\"success\": false, \"message\": \"该表已有主键，无法再设置主键\"}";
            return res;
        }
        
        if (!fieldExists) {
            res.statusCode = 400;
            res.body = "{\"success\": false, \"message\": \"指定的字段不存在\"}";
            return res;
        }
        
        // 先检查字段是否允许NULL
        checkResult = conn->executeQuery(checkSql);
        bool isNullable = true;
        while ((row = mysql_fetch_row(checkResult))) {
            if (row[0] && strcmp(row[0], columnIt->second.c_str()) == 0) {
                if (row[2] && strcmp(row[2], "NO") == 0) {
                    isNullable = false;
                }
                break;
            }
        }
        mysql_free_result(checkResult);
        
        // 如果字段允许NULL，需要先修改为NOT NULL
        if (isNullable) {
            std::string fieldType = req.params.at("type");
            std::string modifySql = "ALTER TABLE " + tableIt->second + " MODIFY COLUMN " + columnIt->second + " " + 
                                   fieldType + " NOT NULL";
            
            auto defaultIt = req.params.find("default");
            if (defaultIt != req.params.end() && !defaultIt->second.empty()) {
                modifySql += " DEFAULT '" + defaultIt->second + "'";
            }
            
            if (!conn->executeUpdate(modifySql)) {
                res.statusCode = 500;
                res.body = "{\"success\": false, \"message\": \"无法将字段设置为NOT NULL\"}";
                return res;
            }
        }
        
        // 检查字段是否有重复值
        std::string countSql = "SELECT " + columnIt->second + ", COUNT(*) as cnt FROM " + tableIt->second + 
                               " GROUP BY " + columnIt->second + " HAVING cnt > 1";
        MYSQL_RES* countResult = conn->executeQuery(countSql);
        if (countResult) {
            MYSQL_ROW countRow = mysql_fetch_row(countResult);
            if (countRow) {
                mysql_free_result(countResult);
                res.statusCode = 400;
                res.body = "{\"success\": false, \"message\": \"该字段存在重复值，无法设置为主键\"}";
                return res;
            }
            mysql_free_result(countResult);
        }
        
        // 设置主键
        sql = "ALTER TABLE " + tableIt->second + " ADD PRIMARY KEY (" + columnIt->second + ")";
        bool ret = conn->executeUpdate(sql);
        
        if (ret) {
            res.contentType = "application/json";
            res.body = "{\"success\": true, \"message\": \"主键设置成功\"}";
            success = true;
        } else {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"主键设置失败\"}";
        }
    } catch (...) {
        res.statusCode = 500;
        res.body = "{\"success\": false, \"message\": \"服务器内部错误\"}";
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    Logger::getInstance().log(Logger::LogType::MYSQL, req.clientIP, "SET_PRIMARY_KEY", sql, success, duration);
    
    return res;
}

HttpResponse MysqlApiHandler::modifyColumn(const HttpRequest& req) {
    HttpResponse res;
    auto startTime = std::chrono::steady_clock::now();
    bool success = false;
    std::string sql;
    
    auto tableIt = req.params.find("table");
    auto nameIt = req.params.find("name");
    auto typeIt = req.params.find("type");
    auto constraintsIt = req.params.find("constraints");
    
    if (tableIt == req.params.end() || nameIt == req.params.end() || typeIt == req.params.end()) {
        res.statusCode = 400;
        res.body = "{\"success\": false, \"message\": \"缺少参数\"}";
        return res;
    }
    
    try {
        auto conn = DBConnection::getMysqlConnection();
        if (!conn.isValid()) {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"无法获取MySQL连接\"}";
            return res;
        }
        
        std::string tableName = tableIt->second;
        std::string columnName = nameIt->second;
        std::string columnType = typeIt->second;
        std::string constraints = constraintsIt != req.params.end() ? constraintsIt->second : "";
        
        std::string newName;
        auto newNameIt = req.params.find("newName");
        if (newNameIt != req.params.end() && !newNameIt->second.empty()) {
            newName = newNameIt->second;
        }
        
        // 检查是否要设置UNIQUE约束，如果是则检查现有数据是否有重复
        if (constraints.find("UNIQUE") != std::string::npos) {
            std::string checkUniqueSql = "SELECT " + columnName + ", COUNT(*) as cnt FROM " + tableName +
                                         " GROUP BY " + columnName + " HAVING cnt > 1";
            MYSQL_RES* uniqueResult = conn->executeQuery(checkUniqueSql);
            if (uniqueResult) {
                MYSQL_ROW row = mysql_fetch_row(uniqueResult);
                if (row) {
                    mysql_free_result(uniqueResult);
                    res.statusCode = 400;
                    res.body = "{\"success\": false, \"message\": \"该字段存在重复值，无法设置唯一约束\"}";
                    return res;
                }
                mysql_free_result(uniqueResult);
            }
        }
        
        // 检查是否要设置NOT NULL约束，如果是则检查现有数据是否有空值
        if (constraints.find("NOT NULL") != std::string::npos && constraints.find("DEFAULT") == std::string::npos) {
            std::string checkNullSql = "SELECT COUNT(*) FROM " + tableName + " WHERE " + columnName + " IS NULL";
            MYSQL_RES* nullResult = conn->executeQuery(checkNullSql);
            if (nullResult) {
                MYSQL_ROW row = mysql_fetch_row(nullResult);
                if (row && atoi(row[0]) > 0) {
                    mysql_free_result(nullResult);
                    res.statusCode = 400;
                    res.body = "{\"success\": false, \"message\": \"该字段存在空值，无法设置NOT NULL约束\"}";
                    return res;
                }
                mysql_free_result(nullResult);
            }
        }
        
        // 修改字段类型和约束
        sql = "ALTER TABLE " + tableName + " MODIFY COLUMN " + columnName + " " + columnType + constraints;
        bool ret = conn->executeUpdate(sql);
        
        if (!ret) {
            res.statusCode = 500;
            res.body = "{\"success\": false, \"message\": \"修改字段属性失败\"}";
            return res;
        }
        
        // 如果需要重命名字段
        if (!newName.empty() && newName != columnName) {
            std::string renameSql = "ALTER TABLE " + tableName + " CHANGE COLUMN " + columnName + " " + newName + " " + columnType + constraints;
            if (!conn->executeUpdate(renameSql)) {
                res.statusCode = 500;
                res.body = "{\"success\": false, \"message\": \"重命名字段失败\"}";
                return res;
            }
            sql += "; " + renameSql;
        }
        
        res.contentType = "application/json";
        res.body = "{\"success\": true, \"message\": \"字段修改成功\"}";
        success = true;
    } catch (...) {
        res.statusCode = 500;
        res.body = "{\"success\": false, \"message\": \"服务器内部错误\"}";
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    Logger::getInstance().log(Logger::LogType::MYSQL, req.clientIP, "MODIFY_COLUMN", sql, success, duration);
    
    return res;
}

std::string MysqlApiHandler::toJson(const std::vector<std::vector<std::string>>& data, 
                                    const std::vector<std::string>& headers) {
    std::ostringstream oss;
    oss << "{\"success\": true, \"headers\": [";
    for (size_t i = 0; i < headers.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "\"" << headers[i] << "\"";
    }
    oss << "], \"data\": [";
    
    for (size_t i = 0; i < data.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "[";
        for (size_t j = 0; j < data[i].size(); ++j) {
            if (j > 0) oss << ",";
            oss << "\"" << data[i][j] << "\"";
        }
        oss << "]";
    }
    oss << "]}";
    
    return oss.str();
}

std::string MysqlApiHandler::toJson(const std::map<std::string, std::string>& data) {
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& pair : data) {
        if (!first) oss << ",";
        oss << "\"" << pair.first << "\": \"" << pair.second << "\"";
        first = false;
    }
    oss << "}";
    return oss.str();
}

HttpResponse MysqlApiHandler::poolStatus(const HttpRequest& req) {
    HttpResponse res;
    res.contentType = "application/json";
    
    size_t idle = DBConnection::getMysqlPoolIdleCount();
    size_t active = DBConnection::getMysqlPoolActiveCount();
    size_t total = idle + active;
    
    res.body = "{\"success\": true, \"idle\": " + std::to_string(idle) + 
               ", \"active\": " + std::to_string(active) + 
               ", \"total\": " + std::to_string(total) + 
               ", \"maxActive\": " + std::to_string(DBConnection::getMysqlPoolMaxActive()) +
               ", \"minIdlePercent\": " + std::to_string(DBConnection::getMysqlPoolMinIdlePercent()) +
               ", \"currentMinIdle\": " + std::to_string(DBConnection::getMysqlPoolCurrentMinIdle()) +
               ", \"maxWaitMs\": " + std::to_string(DBConnection::getMysqlPoolMaxWaitMs()) +
               ", \"idleTimeoutMs\": " + std::to_string(DBConnection::getMysqlPoolIdleTimeoutMs()) + "}";
    
    return res;
}

HttpResponse MysqlApiHandler::updatePoolConfig(const HttpRequest& req) {
    HttpResponse res;
    res.contentType = "application/json";
    
    try {
        auto maxActiveIt = req.params.find("maxActive");
        auto minIdleIt = req.params.find("minIdle");
        auto maxWaitMsIt = req.params.find("maxWaitMs");
        auto idleTimeoutMsIt = req.params.find("idleTimeoutMs");
        
        if (maxActiveIt != req.params.end()) {
            int maxActive = std::stoi(maxActiveIt->second);
            if (!DBConnection::setMysqlPoolMaxActive(maxActive)) {
                res.statusCode = 400;
                res.body = "{\"success\": false, \"message\": \"最大连接数不能小于当前活跃连接数\"}";
                return res;
            }
        }
        
        if (minIdleIt != req.params.end()) {
            int minIdlePercent = std::stoi(minIdleIt->second);
            DBConnection::setMysqlPoolMinIdlePercent(minIdlePercent);
        }
        
        if (maxWaitMsIt != req.params.end()) {
            int maxWaitMs = std::stoi(maxWaitMsIt->second);
            DBConnection::setMysqlPoolMaxWaitMs(maxWaitMs);
        }
        
        if (idleTimeoutMsIt != req.params.end()) {
            int idleTimeoutMs = std::stoi(idleTimeoutMsIt->second);
            DBConnection::setMysqlPoolIdleTimeoutMs(idleTimeoutMs);
        }
        
        res.body = "{\"success\": true, \"message\": \"MySQL连接池配置更新成功\"}";
    } catch (...) {
        res.statusCode = 500;
        res.body = "{\"success\": false, \"message\": \"配置更新失败\"}";
    }
    
    return res;
}

}