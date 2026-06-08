
#ifndef CONNPOOL_HPP
#define CONNPOOL_HPP

#include "ConnPool.h"
#include "Timer.h"
#include <iostream>

template<typename T>
ConnPool<T>::ConnPool(CreateFunc createFunc, DestroyFunc destroyFunc,
                      ValidateFunc validateFunc, PingFunc pingFunc)
    : m_createFunc(createFunc), m_destroyFunc(destroyFunc),
      m_validateFunc(validateFunc), m_pingFunc(pingFunc),
      m_initSize(5), m_minIdlePercent(20), m_maxActive(20),
      m_maxWaitMs(3000), m_idleTimeoutMs(30000), m_isRunning(false) {
}

template<typename T>
ConnPool<T>::~ConnPool() {
    m_isRunning.store(false);
    if (m_timer) {
        m_timer->stop();
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    while (!m_idleConnections.empty()) {
        T* conn = m_idleConnections.front();
        m_idleConnections.pop();
        m_destroyFunc(conn);
    }
    
    for (T* conn : m_activeConnections) {
        m_destroyFunc(conn);
    }
    m_activeConnections.clear();
}

template<typename T>
bool ConnPool<T>::init(int initSize, int minIdlePercent, int maxActive,
                       int maxWaitMs, int idleTimeoutMs, int timerIntervalMs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_isRunning.load()) {
        return true;
    }
    
    m_initSize = initSize;
    m_minIdlePercent = minIdlePercent;
    m_maxActive = maxActive;
    m_maxWaitMs = maxWaitMs;
    m_idleTimeoutMs = idleTimeoutMs;
    
    int successCount = 0;
    for (int i = 0; i < m_initSize; ++i) {
        T* conn = m_createFunc();
        if (conn && m_validateFunc(conn)) {
            m_idleConnections.push(conn);
            ++successCount;
        } else {
            if (conn) {
                m_destroyFunc(conn);
            }
            std::cerr << "[ConnPool] init: connection #" << (i + 1)
                      << " failed to create or validate, skipped." << std::endl;
        }
    }
    
    std::cout << "[ConnPool] init complete: " << successCount << "/" << m_initSize
              << " connections created successfully." << std::endl;
    
    m_timer.reset(new Timer(timerIntervalMs, std::bind(&ConnPool<T>::timerCallback, this)));
    m_timer->start();
    m_isRunning.store(true);
    
    return successCount > 0;  // 至少有一个连接创建成功才算 init 成功
}

template<typename T>
std::unique_ptr<T, std::function<void(T*)>> ConnPool<T>::getConnection() {
    std::unique_lock<std::mutex> lock(m_mutex);
    
    size_t idleBefore = m_idleConnections.size();
    size_t activeBefore = m_activeConnections.size();
    
    auto waitResult = m_cond.wait_for(lock, std::chrono::milliseconds(m_maxWaitMs),
        [this] { return !m_idleConnections.empty() || m_activeConnections.size() < static_cast<size_t>(m_maxActive); });
    
    if (!waitResult) {
        std::cout << "[ConnPool] Get connection timeout! Active: " << m_activeConnections.size() 
                  << ", Idle: " << m_idleConnections.size() << std::endl;
        return nullptr;
    }
    
    T* conn = nullptr;
    bool isNewConnection = false;
    
    while (!m_idleConnections.empty()) {
        conn = m_idleConnections.front();
        m_idleConnections.pop();
        
        if (m_validateFunc(conn)) {
            break;
        } else {
            m_destroyFunc(conn);
            conn = nullptr;
        }
    }
    
    if (!conn) {
        if (m_activeConnections.size() < static_cast<size_t>(m_maxActive)) {
            conn = m_createFunc();
            isNewConnection = true;
            if (conn && !m_validateFunc(conn)) {
                m_destroyFunc(conn);
                conn = nullptr;
            }
        }
    }
    
    if (conn) {
        m_activeConnections.insert(conn);
        
        if (isNewConnection) {
            std::cout << "[ConnPool] Created new connection. Active: " 
                      << activeBefore << " -> " << m_activeConnections.size() 
                      << ", Idle: " << idleBefore << " -> " << m_idleConnections.size() << std::endl;
        } else {
            std::cout << "[ConnPool] Acquired from idle pool. Active: " 
                      << activeBefore << " -> " << m_activeConnections.size() 
                      << ", Idle: " << idleBefore << " -> " << m_idleConnections.size() << std::endl;
        }
    }
    
    return std::unique_ptr<T, std::function<void(T*)>>(
        conn,
        [this](T* c) { this->returnConnection(c); }
    );
}

template<typename T>
void ConnPool<T>::returnConnection(T* conn) {
    if (!conn) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    size_t activeBefore = m_activeConnections.size();
    size_t idleBefore = m_idleConnections.size();
    
    m_activeConnections.erase(conn);
    
    if (m_validateFunc(conn)) {
        m_idleConnections.push(conn);
        m_cond.notify_one();
        std::cout << "[ConnPool] Connection returned to idle pool. Active: " 
                  << activeBefore-1 << " -> " << m_activeConnections.size() 
                  << ", Idle: " << idleBefore << " -> " << m_idleConnections.size() << std::endl;
    } else {
        m_destroyFunc(conn);
        std::cout << "[ConnPool] Connection destroyed (invalid). Active: " 
                  << activeBefore-1 << " -> " << m_activeConnections.size() 
                  << ", Idle remains: " << idleBefore << std::endl;
    }
}

template<typename T>
size_t ConnPool<T>::getIdleCount() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_idleConnections.size();
}

template<typename T>
size_t ConnPool<T>::getActiveCount() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_activeConnections.size();
}

template<typename T>
int ConnPool<T>::getCurrentMinIdle() {
    std::lock_guard<std::mutex> lock(m_mutex);
    // 动态计算：当前总连接数 * 最小空闲百分比 / 100
    size_t totalConnections = m_idleConnections.size() + m_activeConnections.size();
    int minIdle = static_cast<int>(totalConnections * m_minIdlePercent / 100);
    // 至少保证1个空闲连接
    return std::max(1, minIdle);
}

template<typename T>
bool ConnPool<T>::setMaxActive(int maxActive) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (maxActive > 0 && maxActive >= static_cast<int>(m_activeConnections.size())) {
        m_maxActive = maxActive;
        return true;
    }
    return false;
}

template<typename T>
void ConnPool<T>::setMinIdlePercent(int minIdlePercent) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (minIdlePercent >= 0 && minIdlePercent <= 100) {
        m_minIdlePercent = minIdlePercent;
    }
}

template<typename T>
void ConnPool<T>::setMaxWaitMs(int maxWaitMs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (maxWaitMs > 0) {
        m_maxWaitMs = maxWaitMs;
    }
}

template<typename T>
void ConnPool<T>::setIdleTimeoutMs(int idleTimeoutMs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (idleTimeoutMs > 0) {
        m_idleTimeoutMs = idleTimeoutMs;
    }
}

template<typename T>
void ConnPool<T>::timerCallback() {
    adjustIdleConnections();  // 调整空闲连接数（补充或销毁）
    keepAliveCheck();
}

template<typename T>
void ConnPool<T>::adjustIdleConnections() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // 计算当前总连接数
    size_t totalConnections = m_idleConnections.size() + m_activeConnections.size();
    
    // 动态计算最小空闲连接数：当前总连接数 * 最小空闲百分比 / 100
    int minIdle = static_cast<int>(totalConnections * m_minIdlePercent / 100);
    // 至少保证1个空闲连接
    minIdle = std::max(1, minIdle);
    
    size_t currentIdle = m_idleConnections.size();
    
    // 如果空闲连接数小于最小值，需要补充（多余空闲连接不销毁，只保证下限）
    if (currentIdle < static_cast<size_t>(minIdle)) {
        size_t needCreate = minIdle - currentIdle;
        for (size_t i = 0; i < needCreate; ++i) {
            // 检查是否超过最大连接数限制
            if (totalConnections + i >= static_cast<size_t>(m_maxActive)) {
                break;
            }
            T* conn = m_createFunc();
            if (conn && m_validateFunc(conn)) {
                m_idleConnections.push(conn);
            } else {
                if (conn) {
                    m_destroyFunc(conn);
                }
            }
        }
    }
}

template<typename T>
void ConnPool<T>::keepAliveCheck() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::queue<T*> validConnections;
    
    while (!m_idleConnections.empty()) {
        T* conn = m_idleConnections.front();
        m_idleConnections.pop();
        
        if (m_pingFunc(conn)) {
            validConnections.push(conn);
        } else {
            m_destroyFunc(conn);
        }
    }
    
    while (!validConnections.empty()) {
        m_idleConnections.push(validConnections.front());
        validConnections.pop();
    }
}

#endif // CONNPOOL_HPP
