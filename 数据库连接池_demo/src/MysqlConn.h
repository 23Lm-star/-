
#ifndef MYSQLCONN_H
#define MYSQLCONN_H

#include <mysql/mysql.h>
#include <string>
#include <atomic>
#include <chrono>

/**
 * @brief MySQL连接封装类
 * 
 * 封装MySQL原生连接，提供完整的数据库操作接口，
 * 内置连接失效检测和重新连接功能。
 */
class MysqlConn {
public:
    /**
     * @brief 构造函数
     * @param host 数据库主机地址
     * @param port 数据库端口
     * @param user 用户名
     * @param password 密码
     * @param dbname 数据库名称
     */
    MysqlConn(const std::string& host, int port,
              const std::string& user, const std::string& password,
              const std::string& dbname);
    
    /**
     * @brief 析构函数
     * 
     * 关闭数据库连接。
     */
    ~MysqlConn();
    
    /**
     * @brief 建立数据库连接
     * @return 连接成功返回true，失败返回false
     */
    bool connect();
    
    /**
     * @brief 断开数据库连接
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
     * @brief 创建数据表
     * @param sql CREATE TABLE语句
     * @return 执行成功返回true，失败返回false
     */
    bool executeCreateTable(const std::string& sql);
    
    /**
     * @brief 执行插入操作
     * @param sql INSERT语句
     * @return 执行成功返回true，失败返回false
     */
    bool executeInsert(const std::string& sql);
    
    /**
     * @brief 执行更新操作
     * @param sql UPDATE语句
     * @return 执行成功返回true，失败返回false
     */
    bool executeUpdate(const std::string& sql);
    
    /**
     * @brief 执行删除操作
     * @param sql DELETE语句
     * @return 执行成功返回true，失败返回false
     */
    bool executeDelete(const std::string& sql);
    
    /**
     * @brief 执行查询操作
     * @param sql SELECT语句
     * @return 返回查询结果集，失败返回nullptr
     */
    MYSQL_RES* executeQuery(const std::string& sql);
    
    /**
     * @brief 开启事务
     * @return 成功返回true，失败返回false
     */
    bool beginTransaction();
    
    /**
     * @brief 提交事务
     * @return 成功返回true，失败返回false
     */
    bool commitTransaction();
    
    /**
     * @brief 回滚事务
     * @return 成功返回true，失败返回false
     */
    bool rollbackTransaction();
    
    /**
     * @brief 心跳检测
     * 
     * 执行SELECT 1检查连接是否存活。
     * @return 检测成功返回true，失败返回false
     */
    bool ping();
    
    /**
     * @brief 获取影响的行数
     * @return 受影响的行数
     */
    unsigned long long getAffectedRows() const;
    
    /**
     * @brief 获取最后插入的ID
     * @return 最后插入记录的自增ID
     */
    unsigned long long getLastInsertId() const;
    
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
    /**
     * @brief 执行SQL语句（非查询）
     * @param sql SQL语句
     * @return 执行成功返回true，失败返回false
     */
    bool execute(const std::string& sql);

private:
    MYSQL* m_conn;                          ///< MySQL原生连接指针
    std::string m_host;                      ///< 数据库主机地址
    int m_port;                              ///< 数据库端口
    std::string m_user;                      ///< 用户名
    std::string m_password;                  ///< 密码
    std::string m_dbname;                    ///< 数据库名称
    std::atomic<bool> m_isValid;             ///< 连接有效性标记（原子变量）
    std::chrono::steady_clock::time_point m_lastUsedTime;  ///< 最后使用时间
};

#endif // MYSQLCONN_H
