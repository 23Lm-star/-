/**
 * @file SessionManager.h
 * @brief 会话管理器头文件 - 负责管理用户会话的创建、验证和销毁
 * 
 * 功能说明：
 * - 采用单例模式确保全局只有一个会话管理器实例
 * - 使用Redis存储会话，支持集群环境下会话共享
 * - 会话ID采用随机生成方式确保安全性
 * - 会话过期时间由Redis自动管理
 * 
 * 使用场景：
 * - 用户登录后创建会话
 * - 请求拦截时验证会话有效性
 * - 用户登出时销毁会话
 * - 集群环境下的会话共享
 */

#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <string>
#include <mutex>
#include <random>
#include <sstream>
#include <iomanip>
#include <hiredis/hiredis.h>

namespace Auth {

/**
 * @brief 会话数据结构
 * 存储单个会话的相关信息
 */
struct Session {
    std::string username;                              // 会话对应的用户名
    std::string role;                                  // 用户角色
    long long createdAt;                               // 会话创建时间戳
};

/**
 * @brief 会话管理器类
 * 统一管理所有用户会话的生命周期，使用Redis实现会话共享
 */
class SessionManager {
public:
    /**
     * @brief 获取单例实例
     * @return SessionManager& 返回单例引用
     */
    static SessionManager& instance();

    /**
     * @brief 初始化Redis连接
     * @param host Redis服务器地址
     * @param port Redis服务器端口
     * @return bool 初始化是否成功
     */
    bool initRedis(const std::string& host, int port);

    /**
     * @brief 创建新会话
     * @param username 用户名
     * @param role 用户角色
     * @return std::string 返回生成的会话ID，如果创建失败则返回空字符串
     */
    std::string createSession(const std::string& username, const std::string& role = "user");

    /**
     * @brief 验证会话是否有效
     * @param sessionId 会话ID
     * @return bool 有效返回true，无效返回false
     */
    bool validateSession(const std::string& sessionId);

    /**
     * @brief 获取会话对应的用户名
     * @param sessionId 会话ID
     * @return std::string 用户名，如果会话无效则返回空字符串
     */
    std::string getSessionUser(const std::string& sessionId);

    /**
     * @brief 获取会话对应的用户角色
     * @param sessionId 会话ID
     * @return std::string 角色，如果会话无效则返回空字符串
     */
    std::string getSessionRole(const std::string& sessionId);

    /**
     * @brief 销毁指定会话
     * @param sessionId 会话ID
     */
    void destroySession(const std::string& sessionId);

    /**
     * @brief 设置会话超时时间
     * @param seconds 超时秒数
     */
    void setSessionTimeout(int seconds);

    /**
     * @brief 检查Redis连接状态
     * @return bool 连接正常返回true
     */
    bool isRedisConnected();

private:
    // 私有构造函数，确保单例模式
    SessionManager();
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

    // 生成随机会话ID
    std::string generateSessionId();

    redisContext* m_redis;          // Redis连接上下文
    std::mutex m_mutex;             // 线程安全互斥锁
    int m_timeoutSeconds;           // 会话超时时间（秒）
};

} // namespace Auth

#endif // SESSION_MANAGER_H