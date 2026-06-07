
#include "Timer.h"

Timer::Timer(int intervalMs, std::function<void()> task)
    : m_intervalMs(intervalMs), m_task(task), m_isRunning(false) {
}

Timer::~Timer() {
    stop();
}

void Timer::start() {
    if (!m_isRunning.load()) {
        m_isRunning.store(true);
        m_thread = std::thread(&Timer::run, this);
    }
}

void Timer::stop() {
    if (m_isRunning.load()) {
        m_isRunning.store(false);
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }
}

void Timer::run() {
    while (m_isRunning.load()) {
        try {
            m_task();
        } catch (const std::exception& e) {
            // 捕获任务执行异常，避免定时器线程崩溃
        }
        
        // 等待指定间隔
        std::this_thread::sleep_for(std::chrono::milliseconds(m_intervalMs));
    }
}
