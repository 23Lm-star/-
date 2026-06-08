
#ifndef REDISCONN_H
#define REDISCONN_H

#include <hiredis/hiredis.h>
#include <string>
#include <atomic>
#include <chrono>
#include <unordered_map>
#include <vector>

/**
 * @brief Redis连接封装类
 * 
 * 封装Redis原生连接，提供基础的String和Hash操作接口，
 * 内置连接失效检测和重新连接功能。
 */
class RedisConn {
public:
    /**
     * @brief 构造函数
     * @param host Redis主机地址
     * @param port Redis端口
     * @param timeout 连接超时时间（毫秒）
     */
    RedisConn(const std::string& host, int port, int timeout = 3000);
    
    /**
     * @brief 析构函数
     * 
     * 关闭Redis连接。
     */
    ~RedisConn();
    
    /**
     * @brief 建立Redis连接
     * @return 连接成功返回true，失败返回false
     */
    bool connect();
    
    /**
     * @brief 断开Redis连接
     */
    void disconnect();
    
    /**
     * @brief 判断连接是否有效
     * @return 连接有效返回true，否则返回false
     */
    bool isValid() const;
    
    /**
     * @brief 尝试重新连接
     * @return 重连成功返回true，失败返回false
     */
    bool reconnect();
    
    /**
     * @brief 标记连接失效
     */
    void markInvalid();
    
    /**
     * @brief 设置String值
     * @param key 键名
     * @param value 键值
     * @return 设置成功返回true，失败返回false
     */
    bool set(const std::string& key, const std::string& value);
    
    /**
     * @brief 获取String值
     * @param key 键名
     * @return 返回键值，失败返回空字符串
     */
    std::string get(const std::string& key);
    
    /**
     * @brief 设置Hash字段
     * @param key Hash键名
     * @param field 字段名
     * @param value 字段值
     * @return 设置成功返回true，失败返回false
     */
    bool hset(const std::string& key, const std::string& field, const std::string& value);
    
    /**
     * @brief 获取Hash字段
     * @param key Hash键名
     * @param field 字段名
     * @return 返回字段值，失败返回空字符串
     */
    std::string hget(const std::string& key, const std::string& field);
    
    /**
     * @brief 获取Hash所有字段和值
     * @param key Hash键名
     * @return 返回字段名和值的映射表
     */
    std::unordered_map<std::string, std::string> hgetall(const std::string& key);
    
    /**
     * @brief 删除键
     * @param key 键名
     * @return 删除成功返回true，失败返回false
     */
    bool del(const std::string& key);
    
    /**
     * @brief 删除Hash字段
     * @param key Hash键名
     * @param field 字段名
     * @return 删除成功返回true，失败返回false
     */
    bool hdel(const std::string& key, const std::string& field);
    
    /**
     * @brief 心跳检测
     * 
     * 执行PING命令检查连接是否存活。
     * @return 检测成功返回true，失败返回false
     */
    bool ping();
    
    /**
     * @brief 添加元素到有序集合
     * @param key 有序集合键名
     * @param member 成员
     * @param score 分数
     * @return 添加成功返回true，失败返回false
     */
    bool zadd(const std::string& key, const std::string& member, double score);
    
    /**
     * @brief 获取有序集合指定范围的成员
     * @param key 有序集合键名
     * @param start 开始索引
     * @param stop 结束索引（-1表示最后一个）
     * @return 返回成员和分数的向量
     */
    std::vector<std::pair<std::string, double>> zrange(const std::string& key, int start, int stop);
    
    /**
     * @brief 从有序集合中移除成员
     * @param key 有序集合键名
     * @param member 成员
     * @return 移除成功返回true，失败返回false
     */
    bool zrem(const std::string& key, const std::string& member);
    
    /**
     * @brief 更新最后使用时间
     */
    void updateLastUsedTime();
    
    /**
     * @brief 获取最后使用时间
     * @return 最后使用时间点
     */
    std::chrono::steady_clock::time_point getLastUsedTime() const;

private:
    redisContext* m_conn;                    ///< Redis原生连接指针
    std::string m_host;                      ///< Redis主机地址
    int m_port;                              ///< Redis端口
    timeval m_timeout;                       ///< 连接超时时间
    std::atomic<bool> m_isValid;             ///< 连接有效性标记（原子变量）
    std::chrono::steady_clock::time_point m_lastUsedTime;  ///< 最后使用时间
};

#endif // REDISCONN_H
