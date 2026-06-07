
#ifndef CONNPOOL_H
#define CONNPOOL_H

#include <queue>
#include <unordered_set>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <functional>
#include <chrono>

template<typename T>
class ConnPool {
public:
    using CreateFunc = std::function<T*()>;
    using DestroyFunc = std::function<void(T*)>;
    using ValidateFunc = std::function<bool(T*)>;
    using PingFunc = std::function<bool(T*)>;
    
    ConnPool(CreateFunc createFunc, DestroyFunc destroyFunc, 
             ValidateFunc validateFunc, PingFunc pingFunc);
    
    ~ConnPool();
    
    bool init(int initSize = 5, int minIdlePercent = 20, int maxActive = 20,
              int maxWaitMs = 3000, int idleTimeoutMs = 30000, int timerIntervalMs = 5000);
    
    std::unique_ptr<T, std::function<void(T*)>> getConnection();
    
    void returnConnection(T* conn);
    
    size_t getIdleCount();
    
    size_t getActiveCount();
    
    bool setMaxActive(int maxActive);
    void setMinIdlePercent(int minIdlePercent);
    void setMaxWaitMs(int maxWaitMs);
    void setIdleTimeoutMs(int idleTimeoutMs);
    
    int getMaxActive() const { return m_maxActive; }
    int getMinIdlePercent() const { return m_minIdlePercent; }
    int getMaxWaitMs() const { return m_maxWaitMs; }
    int getIdleTimeoutMs() const { return m_idleTimeoutMs; }
    
    // 获取当前动态计算的最小空闲连接数
    int getCurrentMinIdle();
    
private:
    void timerCallback();
    void adjustIdleConnections();  // 调整空闲连接数（补充或销毁）
    void keepAliveCheck();
    
private:
    CreateFunc m_createFunc;
    DestroyFunc m_destroyFunc;
    ValidateFunc m_validateFunc;
    PingFunc m_pingFunc;
    
    std::queue<T*> m_idleConnections;
    std::unordered_set<T*> m_activeConnections;
    int m_initSize;
    int m_minIdlePercent;  // 最小空闲连接数百分比（相对于当前总连接数）
    int m_maxActive;
    int m_maxWaitMs;
    int m_idleTimeoutMs;
    std::mutex m_mutex;
    std::condition_variable m_cond;
    std::unique_ptr<class Timer> m_timer;
    std::atomic<bool> m_isRunning;
};

#include "ConnPool.hpp"

#endif // CONNPOOL_H
