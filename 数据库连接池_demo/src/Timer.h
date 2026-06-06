
#ifndef TIMER_H
#define TIMER_H

#include <thread>
#include <atomic>
#include <functional>
#include <chrono>

/**
 * @brief 定时器模块
 * 
 * 提供独立的定时线程，周期性执行指定任务。
 * 主要用于连接池的空闲连接回收和保活检测。
 */
class Timer {
public:
    /**
     * @brief 构造函数
     * @param intervalMs 定时执行间隔（毫秒）
     * @param task 定时执行的任务函数
     */
    Timer(int intervalMs, std::function<void()> task);
    
    /**
     * @brief 析构函数
     * 
     * 停止定时器线程并等待其退出。
     */
    ~Timer();
    
    /**
     * @brief 启动定时器
     * 
     * 创建并启动定时执行线程。
     */
    void start();
    
    /**
     * @brief 停止定时器
     * 
     * 设置停止标志，等待线程退出。
     */
    void stop();

private:
    /**
     * @brief 定时任务执行循环
     * 
     * 在独立线程中循环执行定时任务。
     */
    void run();

private:
    std::thread m_thread;              ///< 定时器线程
    int m_intervalMs;                  ///< 定时执行间隔（毫秒）
    std::atomic<bool> m_isRunning;     ///< 定时器运行状态
    std::function<void()> m_task;      ///< 定时执行的任务函数
};

#endif // TIMER_H
