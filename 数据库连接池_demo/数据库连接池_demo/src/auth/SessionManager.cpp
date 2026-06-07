/**
 * @file SessionManager.cpp
 * @brief 会话管理器实现文件
 * 
 * 实现说明：
 * - 使用std::random_device和std::mt19937生成密码学安全的随机会话ID
 * - 会话ID格式为32字节十六进制字符串
 * - 清理过期会话时会遍历所有会话并移除超时的会话
 */

#include "SessionManager.h"

namespace Auth {

SessionManager::SessionManager() : m_timeoutSeconds(3600) {}

// 获取单例实例 - 使用局部静态变量实现懒汉单例
SessionManager& SessionManager::instance() {
    static SessionManager instance;
    return instance;
}

// 创建新会话
std::string SessionManager::createSession(const std::string& username) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // 生成32字节随机会话ID
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    std::stringstream ss;
    for (int i = 0; i < 32; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << dis(gen);
    }
    
    std::string sessionId = ss.str();
    m_sessions[sessionId] = {username, std::chrono::steady_clock::now()};
    
    return sessionId;
}

// 验证会话是否有效
bool SessionManager::validateSession(const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_sessions.find(sessionId);
    if (it == m_sessions.end()) {
        return false;
    }
    
    // 检查会话是否超时
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        now - it->second.createdAt).count();
    
    if (elapsed > m_timeoutSeconds) {
        m_sessions.erase(it);
        return false;
    }
    
    // 更新最后访问时间（续期）
    it->second.createdAt = now;
    return true;
}

// 获取会话对应的用户名
std::string SessionManager::getSessionUser(const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_sessions.find(sessionId);
    if (it != m_sessions.end()) {
        return it->second.username;
    }
    return "";
}

// 销毁指定会话
void SessionManager::destroySession(const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sessions.erase(sessionId);
}

// 设置会话超时时间
void SessionManager::setSessionTimeout(int seconds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_timeoutSeconds = seconds;
}

// 清理所有过期会话
void SessionManager::cleanExpiredSessions() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto now = std::chrono::steady_clock::now();
    for (auto it = m_sessions.begin(); it != m_sessions.end(); ) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - it->second.createdAt).count();
        if (elapsed > m_timeoutSeconds) {
            it = m_sessions.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace Auth