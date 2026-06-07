// ==============================================================================
// UserDBInit.cpp
// 用户数据库初始化与操作类实现文件
// 
// 功能说明：
// - 创建sys_users用户表
// - 初始化默认超级管理员
// - 提供用户数据的增删改查操作
// - 实现密码的安全存储（SHA-256 + 盐值）
// ==============================================================================

#include "UserDBInit.h"
#include "PasswordHash.h"
#include "DBConnection.h"
#include "MysqlConn.h"
#include <sstream>

namespace Auth {

// 初始化用户数据库（创建表并初始化默认管理员）
bool UserDBInit::init() {
    if (!createTable()) {
        return false;
    }
    return createDefaultAdmin();
}

// 创建sys_users表
bool UserDBInit::createTable() {
    auto conn = DBConnection::getMysqlConnection();
    if (!conn.isValid()) {
        return false;
    }
    
    std::string sql = R"(
        CREATE TABLE IF NOT EXISTS sys_users (
            id INT PRIMARY KEY AUTO_INCREMENT,
            username VARCHAR(50) NOT NULL UNIQUE,
            password_hash VARCHAR(64) NOT NULL,
            salt VARCHAR(64) NOT NULL,
            role ENUM('super_admin', 'user') NOT NULL DEFAULT 'user',
            status ENUM('pending', 'approved', 'rejected') NOT NULL DEFAULT 'pending',
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
    )";
    
    return conn->executeCreateTable(sql);
}

// 创建默认超级管理员用户
bool UserDBInit::createDefaultAdmin() {
    auto conn = DBConnection::getMysqlConnection();
    if (!conn.isValid()) {
        return false;
    }
    
    // 检查admin用户是否已存在
    std::string checkSql = "SELECT id FROM sys_users WHERE username = 'admin'";
    MYSQL_RES* result = conn->executeQuery(checkSql);
    if (result) {
        MYSQL_ROW row = mysql_fetch_row(result);
        mysql_free_result(result);
        if (row) {
            // 用户已存在，无需创建
            return true;
        }
    }
    
    // 创建默认管理员：admin / admin123
    std::string salt = PasswordHash::generateSalt(32);
    std::string passwordHash = PasswordHash::hashPassword("admin123", salt);
    
    std::string sql = "INSERT INTO sys_users (username, password_hash, salt, role, status) "
                      "VALUES ('admin', '" + passwordHash + "', '" + salt + "', 'super_admin', 'approved')";
    
    return conn->executeInsert(sql);
}

// 添加新用户（注册）
bool UserDBInit::addUser(const std::string& username, const std::string& password) {
    auto conn = DBConnection::getMysqlConnection();
    if (!conn.isValid()) {
        return false;
    }
    
    // 检查用户名是否已存在
    std::string checkSql = "SELECT id FROM sys_users WHERE username = '" + username + "'";
    MYSQL_RES* result = conn->executeQuery(checkSql);
    if (result) {
        MYSQL_ROW row = mysql_fetch_row(result);
        mysql_free_result(result);
        if (row) {
            // 用户名已存在
            return false;
        }
    }
    
    // 生成盐值并哈希密码
    std::string salt = PasswordHash::generateSalt(32);
    std::string passwordHash = PasswordHash::hashPassword(password, salt);
    
    // 插入新用户，状态为pending
    std::string sql = "INSERT INTO sys_users (username, password_hash, salt, role, status) "
                      "VALUES ('" + username + "', '" + passwordHash + "', '" + salt + "', 'user', 'pending')";
    
    return conn->executeInsert(sql);
}

// 验证用户登录（检查密码和状态）
bool UserDBInit::validateUser(const std::string& username, const std::string& password) {
    auto conn = DBConnection::getMysqlConnection();
    if (!conn.isValid()) {
        return false;
    }
    
    std::string sql = "SELECT password_hash, salt, status FROM sys_users WHERE username = '" + username + "'";
    MYSQL_RES* result = conn->executeQuery(sql);
    if (!result) {
        return false;
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row) {
        mysql_free_result(result);
        return false;
    }
    
    std::string passwordHash = row[0] ? row[0] : "";
    std::string salt = row[1] ? row[1] : "";
    std::string status = row[2] ? row[2] : "";
    
    mysql_free_result(result);
    
    // 检查用户状态是否为已批准
    if (status != "approved") {
        return false;
    }
    
    // 验证密码
    return PasswordHash::verifyPassword(password, passwordHash, salt);
}

// 获取用户角色
std::string UserDBInit::getUserRole(const std::string& username) {
    auto conn = DBConnection::getMysqlConnection();
    if (!conn.isValid()) {
        return "";
    }
    
    std::string sql = "SELECT role FROM sys_users WHERE username = '" + username + "'";
    MYSQL_RES* result = conn->executeQuery(sql);
    if (!result) {
        return "";
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    std::string role = row && row[0] ? row[0] : "";
    
    mysql_free_result(result);
    return role;
}

// 获取用户状态
std::string UserDBInit::getUserStatus(const std::string& username) {
    auto conn = DBConnection::getMysqlConnection();
    if (!conn.isValid()) {
        return "";
    }
    
    std::string sql = "SELECT status FROM sys_users WHERE username = '" + username + "'";
    MYSQL_RES* result = conn->executeQuery(sql);
    if (!result) {
        return "";
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    std::string status = row && row[0] ? row[0] : "";
    
    mysql_free_result(result);
    return status;
}

// 更新用户状态
bool UserDBInit::updateUserStatus(int userId, const std::string& status) {
    auto conn = DBConnection::getMysqlConnection();
    if (!conn.isValid()) {
        return false;
    }
    
    std::string sql = "UPDATE sys_users SET status = '" + status + "' WHERE id = " + std::to_string(userId);
    
    return conn->executeUpdate(sql);
}

// 删除用户
bool UserDBInit::deleteUser(int userId) {
    auto conn = DBConnection::getMysqlConnection();
    if (!conn.isValid()) {
        return false;
    }
    
    std::string sql = "DELETE FROM sys_users WHERE id = " + std::to_string(userId);
    
    return conn->executeDelete(sql);
}

// 修改密码
bool UserDBInit::changePassword(const std::string& username, const std::string& newPassword) {
    auto conn = DBConnection::getMysqlConnection();
    if (!conn.isValid()) {
        return false;
    }
    
    // 生成新的盐值并哈希新密码
    std::string salt = PasswordHash::generateSalt(32);
    std::string passwordHash = PasswordHash::hashPassword(newPassword, salt);
    
    std::string sql = "UPDATE sys_users SET password_hash = '" + passwordHash + "', salt = '" + salt + "' WHERE username = '" + username + "'";
    
    return conn->executeUpdate(sql);
}

// 获取所有用户列表
std::vector<std::vector<std::string>> UserDBInit::getAllUsers() {
    std::vector<std::vector<std::string>> users;
    auto conn = DBConnection::getMysqlConnection();
    if (!conn.isValid()) {
        return users;
    }
    
    std::string sql = "SELECT id, username, role, status, created_at FROM sys_users ORDER BY created_at DESC";
    MYSQL_RES* result = conn->executeQuery(sql);
    if (!result) {
        return users;
    }
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) != nullptr) {
        std::vector<std::string> user;
        for (int i = 0; i < mysql_num_fields(result); ++i) {
            user.push_back(row[i] ? row[i] : "");
        }
        users.push_back(user);
    }
    
    mysql_free_result(result);
    return users;
}

// 获取待审批用户列表
std::vector<std::vector<std::string>> UserDBInit::getPendingUsers() {
    std::vector<std::vector<std::string>> users;
    auto conn = DBConnection::getMysqlConnection();
    if (!conn.isValid()) {
        return users;
    }
    
    std::string sql = "SELECT id, username, role, status, created_at FROM sys_users WHERE status = 'pending' ORDER BY created_at DESC";
    MYSQL_RES* result = conn->executeQuery(sql);
    if (!result) {
        return users;
    }
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) != nullptr) {
        std::vector<std::string> user;
        for (int i = 0; i < mysql_num_fields(result); ++i) {
            user.push_back(row[i] ? row[i] : "");
        }
        users.push_back(user);
    }
    
    mysql_free_result(result);
    return users;
}

} // namespace Auth