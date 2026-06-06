// ==============================================================================
// UserDBInit.h
// 用户数据库初始化与操作类头文件
// 
// 功能说明：
// - 创建sys_users用户表
// - 初始化默认超级管理员
// - 提供用户数据的增删改查操作
// - 实现密码的安全存储（SHA-256 + 盐值）
// ==============================================================================

#ifndef USERDBINIT_H
#define USERDBINIT_H

#include <string>
#include <vector>

namespace Auth {

class UserDBInit {
public:
    /**
     * @brief 初始化用户数据库（创建表并初始化默认管理员）
     * @return 成功返回true，失败返回false
     */
    static bool init();
    
    /**
     * @brief 创建sys_users表
     * @return 成功返回true，失败返回false
     */
    static bool createTable();
    
    /**
     * @brief 创建默认超级管理员用户（admin/admin123）
     * @return 成功返回true，失败返回false
     */
    static bool createDefaultAdmin();
    
    /**
     * @brief 添加新用户（注册）
     * @param username 用户名
     * @param password 密码
     * @return 成功返回true（用户不存在），失败返回false（用户名已存在）
     */
    static bool addUser(const std::string& username, const std::string& password);
    
    /**
     * @brief 验证用户登录（检查密码和状态）
     * @param username 用户名
     * @param password 密码
     * @return 验证成功且用户已审批返回true，否则返回false
     */
    static bool validateUser(const std::string& username, const std::string& password);
    
    /**
     * @brief 获取用户角色
     * @param username 用户名
     * @return 返回角色字符串（super_admin或user），失败返回空字符串
     */
    static std::string getUserRole(const std::string& username);
    
    /**
     * @brief 获取用户状态
     * @param username 用户名
     * @return 返回状态字符串（pending/approved/rejected），失败返回空字符串
     */
    static std::string getUserStatus(const std::string& username);
    
    /**
     * @brief 更新用户状态
     * @param userId 用户ID
     * @param status 新状态（pending/approved/rejected）
     * @return 成功返回true，失败返回false
     */
    static bool updateUserStatus(int userId, const std::string& status);
    
    /**
     * @brief 删除用户
     * @param userId 用户ID
     * @return 成功返回true，失败返回false
     */
    static bool deleteUser(int userId);
    
    /**
     * @brief 修改密码
     * @param username 用户名
     * @param newPassword 新密码
     * @return 成功返回true，失败返回false
     */
    static bool changePassword(const std::string& username, const std::string& newPassword);
    
    /**
     * @brief 获取所有用户列表
     * @return 用户列表，每个用户包含[id, username, role, status, created_at]
     */
    static std::vector<std::vector<std::string>> getAllUsers();
    
    /**
     * @brief 获取待审批用户列表
     * @return 待审批用户列表，每个用户包含[id, username, role, status, created_at]
     */
    static std::vector<std::vector<std::string>> getPendingUsers();
};

} // namespace Auth

#endif // USERDBINIT_H