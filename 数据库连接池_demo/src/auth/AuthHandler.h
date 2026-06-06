/**
 * @file AuthHandler.h
 * @brief 用户认证处理器头文件 - 负责验证用户凭据和管理用户列表
 * 
 * 功能说明：
 * - 提供用户凭据验证功能（基于MySQL持久化存储）
 * - 管理用户列表（支持添加、查询、删除用户）
 * - 支持角色和状态管理
 * - 密码使用SHA-256+salt加密存储
 * 
 * 使用场景：
 * - 登录时验证用户名密码
 * - 添加新用户（注册）
 * - 获取所有用户列表（用于管理）
 * - 验证用户角色权限
 */

#ifndef AUTH_HANDLER_H
#define AUTH_HANDLER_H

#include <string>
#include <unordered_map>
#include <vector>

namespace Auth {

/**
 * @brief 用户信息结构体
 */
struct UserInfo {
    int id;
    std::string username;
    std::string role;
    std::string status;
    std::string createdAt;
};

/**
 * @brief 用户认证处理器类
 * 统一处理用户认证相关操作，基于MySQL持久化存储
 */
class AuthHandler {
public:
    /**
     * @brief 初始化认证系统
     * @return bool 初始化成功返回true
     */
    static bool init();

    /**
     * @brief 验证用户凭据（用户名和密码）
     * @param username 用户名
     * @param password 密码
     * @return bool 验证通过返回true，否则返回false
     */
    static bool validateUser(const std::string& username, const std::string& password);

    /**
     * @brief 添加新用户（注册）
     * @param username 用户名
     * @param password 密码
     * @return bool 添加成功返回true
     */
    static bool addUser(const std::string& username, const std::string& password);

    /**
     * @brief 获取所有用户列表
     * @return std::vector<UserInfo> 用户列表
     */
    static std::vector<UserInfo> getUsers();

    /**
     * @brief 获取待审批用户列表
     * @return std::vector<UserInfo> 待审批用户列表
     */
    static std::vector<UserInfo> getPendingUsers();

    /**
     * @brief 获取用户角色
     * @param username 用户名
     * @return std::string 角色名（super_admin/user），如果用户不存在返回空字符串
     */
    static std::string getUserRole(const std::string& username);

    /**
     * @brief 获取用户状态
     * @param username 用户名
     * @return std::string 状态（pending/approved/rejected），如果用户不存在返回空字符串
     */
    static std::string getUserStatus(const std::string& username);

    /**
     * @brief 检查用户是否是超级管理员
     * @param username 用户名
     * @return bool 是超级管理员返回true
     */
    static bool isSuperAdmin(const std::string& username);

    /**
     * @brief 更新用户状态（审批/拒绝）
     * @param userId 用户ID
     * @param status 新状态
     * @return bool 更新成功返回true
     */
    static bool updateUserStatus(int userId, const std::string& status);

    /**
     * @brief 删除用户
     * @param userId 用户ID
     * @return bool 删除成功返回true
     */
    static bool deleteUser(int userId);

    /**
     * @brief 修改用户密码
     * @param username 用户名
     * @param newPassword 新密码
     * @return bool 修改成功返回true
     */
    static bool changePassword(const std::string& username, const std::string& newPassword);
};

} // namespace Auth

#endif // AUTH_HANDLER_H