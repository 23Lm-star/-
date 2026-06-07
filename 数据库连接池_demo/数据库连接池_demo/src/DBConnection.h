
#ifndef DBCONNECTION_H
#define DBCONNECTION_H

#include "AutoReconnectPtr.h"
#include "ConnPool.h"
#include "MysqlConn.h"
#include "RedisConn.h"

class DBConnection {
public:
    static AutoReconnectPtr<MysqlConn> getMysqlConnection() {
        return AutoReconnectPtr<MysqlConn>([]() {
            return getMysqlPool()->getConnection();
        });
    }
    
    static AutoReconnectPtr<RedisConn> getRedisConnection() {
        return AutoReconnectPtr<RedisConn>([]() {
            return getRedisPool()->getConnection();
        });
    }
    
    // 初始化连接池，minIdlePercent 为最小空闲连接数百分比（相对于当前总连接数）
    static bool initMysqlPool(int initSize = 5, int minIdlePercent = 20, int maxActive = 20,
                              int maxWaitMs = 3000, int idleTimeoutMs = 30000, 
                              int timerIntervalMs = 5000) {
        return getMysqlPool()->init(initSize, minIdlePercent, maxActive,
                                    maxWaitMs, idleTimeoutMs, timerIntervalMs);
    }
    
    static bool initRedisPool(int initSize = 5, int minIdlePercent = 20, int maxActive = 20,
                              int maxWaitMs = 3000, int idleTimeoutMs = 30000, 
                              int timerIntervalMs = 5000) {
        return getRedisPool()->init(initSize, minIdlePercent, maxActive,
                                    maxWaitMs, idleTimeoutMs, timerIntervalMs);
    }
    
    static size_t getMysqlPoolIdleCount() { return getMysqlPool()->getIdleCount(); }
    static size_t getMysqlPoolActiveCount() { return getMysqlPool()->getActiveCount(); }
    static size_t getRedisPoolIdleCount() { return getRedisPool()->getIdleCount(); }
    static size_t getRedisPoolActiveCount() { return getRedisPool()->getActiveCount(); }
    
    static bool setMysqlPoolMaxActive(int maxActive) { return getMysqlPool()->setMaxActive(maxActive); }
    static void setMysqlPoolMinIdlePercent(int minIdlePercent) { getMysqlPool()->setMinIdlePercent(minIdlePercent); }
    static void setMysqlPoolMaxWaitMs(int maxWaitMs) { getMysqlPool()->setMaxWaitMs(maxWaitMs); }
    static void setMysqlPoolIdleTimeoutMs(int idleTimeoutMs) { getMysqlPool()->setIdleTimeoutMs(idleTimeoutMs); }
    
    static int getMysqlPoolMaxActive() { return getMysqlPool()->getMaxActive(); }
    static int getMysqlPoolMinIdlePercent() { return getMysqlPool()->getMinIdlePercent(); }
    static int getMysqlPoolCurrentMinIdle() { return getMysqlPool()->getCurrentMinIdle(); }
    static int getMysqlPoolMaxWaitMs() { return getMysqlPool()->getMaxWaitMs(); }
    static int getMysqlPoolIdleTimeoutMs() { return getMysqlPool()->getIdleTimeoutMs(); }
    
    static bool setRedisPoolMaxActive(int maxActive) { return getRedisPool()->setMaxActive(maxActive); }
    static void setRedisPoolMinIdlePercent(int minIdlePercent) { getRedisPool()->setMinIdlePercent(minIdlePercent); }
    static void setRedisPoolMaxWaitMs(int maxWaitMs) { getRedisPool()->setMaxWaitMs(maxWaitMs); }
    static void setRedisPoolIdleTimeoutMs(int idleTimeoutMs) { getRedisPool()->setIdleTimeoutMs(idleTimeoutMs); }
    
    static int getRedisPoolMaxActive() { return getRedisPool()->getMaxActive(); }
    static int getRedisPoolMinIdlePercent() { return getRedisPool()->getMinIdlePercent(); }
    static int getRedisPoolCurrentMinIdle() { return getRedisPool()->getCurrentMinIdle(); }
    static int getRedisPoolMaxWaitMs() { return getRedisPool()->getMaxWaitMs(); }
    static int getRedisPoolIdleTimeoutMs() { return getRedisPool()->getIdleTimeoutMs(); }
    
private:
    static ConnPool<MysqlConn>* getMysqlPool() {
        static ConnPool<MysqlConn>* pool = new ConnPool<MysqlConn>(
            []() { 
                MysqlConn* conn = new MysqlConn("192.168.232.155", 3306, "root", "123456", "test"); 
                conn->connect(); 
                return conn; 
            },
            [](MysqlConn* conn) { delete conn; },
            [](MysqlConn* conn) { return conn->isValid(); },
            [](MysqlConn* conn) { return conn->ping(); }
        );
        return pool;
    }
    
    static ConnPool<RedisConn>* getRedisPool() {
        static ConnPool<RedisConn>* pool = new ConnPool<RedisConn>(
            []() { 
                RedisConn* conn = new RedisConn("192.168.232.155", 6379, 3000); 
                conn->connect(); 
                return conn; 
            },
            [](RedisConn* conn) { delete conn; },
            [](RedisConn* conn) { return conn->isValid(); },
            [](RedisConn* conn) { return conn->ping(); }
        );
        return pool;
    }
};

#endif // DBCONNECTION_H
