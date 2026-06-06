// ==============================================================================
// AuthHandler.cpp
// 用户认证处理器实现文件
// 
// 功能说明：
// - 用户认证和管理的统一接口
// - 封装用户验证、注册、管理等功能
// - 基于MySQL数据库进行持久化存储
// ==============================================================================

#include "AuthHandler.h"
#include "UserDBInit.h"

namespace Auth {

// 初始化认证模块
bool AuthHandler::init() {
    return UserDBInit::init();
}

// 验证用户登录
bool AuthHandler::validateUser(const std::string& username, const std::string& password) {
    return UserDBInit::validateUser(username, password);
}

// 添加新用户（注册）
bool AuthHandler::addUser(const std::string& username, const std::string& password) {
    return UserDBInit::addUser(username, password);
}

// 获取用户角色
std::string AuthHandler::getUserRole(const std::string& username) {
    return UserDBInit::getUserRole(username);
}

// 判断是否为超级管理员
bool AuthHandler::isSuperAdmin(const std::string& username) {
    std::string role = UserDBInit::getUserRole(username);
    return role == "super_admin";
}

// 更新用户状态
bool AuthHandler::updateUserStatus(int userId, const std::string& status) {
    return UserDBInit::updateUserStatus(userId, status);
}

// 删除用户
bool AuthHandler::deleteUser(int userId) {
    return UserDBInit::deleteUser(userId);
}

// 修改密码
bool AuthHandler::changePassword(const std::string& username, const std::string& newPassword) {
    return UserDBInit::changePassword(username, newPassword);
}

// 获取所有用户
std::vector<UserInfo> AuthHandler::getUsers() {
    std::vector<UserInfo> users;
    auto result = UserDBInit::getAllUsers();
    
    for (const auto& row : result) {
        UserInfo info;
        info.id = std::stoi(row[0]);
        info.username = row[1];
        info.role = row[2];
        info.status = row[3];
        info.createdAt = row[4];
        users.push_back(info);
    }
    
    return users;
}

// 获取待审批用户
std::vector<UserInfo> AuthHandler::getPendingUsers() {
    std::vector<UserInfo> users;
    auto result = UserDBInit::getPendingUsers();
    
    for (const auto& row : result) {
        UserInfo info;
        info.id = std::stoi(row[0]);
        info.username = row[1];
        info.role = row[2];
        info.status = row[3];
        info.createdAt = row[4];
        users.push_back(info);
    }
    
    return users;
}

} // namespace Auth