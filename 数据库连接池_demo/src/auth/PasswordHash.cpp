// ==============================================================================
// PasswordHash.cpp
// 密码哈希工具类实现文件
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

#include "PasswordHash.h"
#include <openssl/sha.h>
#include <random>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <iostream>

namespace Auth {

// 生成随机盐值
std::string PasswordHash::generateSalt(int length) {
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, chars.size() - 1);
    
    std::string salt;
    salt.reserve(length);
    for (int i = 0; i < length; ++i) {
        salt += chars[dis(gen)];
    }
    return salt;
}

// 对密码进行SHA-256哈希处理
std::string PasswordHash::hashPassword(const std::string& password, const std::string& salt) {
    std::string input = password + salt;
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), hash);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    return ss.str();
}

// 验证密码是否匹配
bool PasswordHash::verifyPassword(const std::string& password, const std::string& hash, const std::string& salt) {
    std::cout << "[DEBUG] PasswordHash::verifyPassword: Starting verification" << std::endl;
    std::cout << "[DEBUG] PasswordHash::verifyPassword: Password length: " << password.length() 
              << ", hash length: " << hash.length() 
              << ", salt length: " << salt.length() << std::endl;
    
    std::string computedHash = hashPassword(password, salt);
    std::cout << "[DEBUG] PasswordHash::verifyPassword: Computed hash: " << computedHash << std::endl;
    std::cout << "[DEBUG] PasswordHash::verifyPassword: Stored hash:   " << hash << std::endl;
    std::cout << "[DEBUG] PasswordHash::verifyPassword: Salt: " << salt << std::endl;
    
    bool match = computedHash == hash;
    std::cout << "[DEBUG] PasswordHash::verifyPassword: Result: " << (match ? "MATCH" : "NO MATCH") << std::endl;
    
    return match;
}

} // namespace Auth