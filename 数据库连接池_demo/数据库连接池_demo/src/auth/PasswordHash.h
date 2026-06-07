// ==============================================================================
// PasswordHash.h
// 密码哈希工具类头文件
// 
// 功能说明：
// - 实现SHA-256密码哈希算法
// - 生成随机盐值
// - 验证密码是否匹配哈希值
// 
// 安全特性：
// - 使用SHA-256加密算法
// - 每个用户使用独立的随机盐值
// - 防止彩虹表攻击
// ==============================================================================

#ifndef PASSWORDHASH_H
#define PASSWORDHASH_H

#include <string>

namespace Auth {

class PasswordHash {
public:
    /**
     * @brief 生成随机盐值
     * @param length 盐值长度，默认为32位
     * @return 随机盐值字符串
     */
    static std::string generateSalt(int length = 32);
    
    /**
     * @brief 对密码进行SHA-256哈希处理
     * @param password 原始密码
     * @param salt 盐值
     * @return 哈希后的密码字符串（64位十六进制）
     */
    static std::string hashPassword(const std::string& password, const std::string& salt);
    
    /**
     * @brief 验证密码是否匹配
     * @param password 用户输入的密码
     * @param hash 存储的哈希值
     * @param salt 存储的盐值
     * @return 密码匹配返回true，否则返回false
     */
    static bool verifyPassword(const std::string& password, const std::string& hash, const std::string& salt);
};

} // namespace Auth

#endif // PASSWORDHASH_H