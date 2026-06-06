#include "Logger.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    m_logFilePath = "../db_operations.log";
    m_logFile.open(m_logFilePath, std::ios::out | std::ios::app);
    if (!m_logFile.is_open()) {
        std::cerr << "❌ 无法打开日志文件: " << m_logFilePath << std::endl;
    } else {
        std::cout << "✅ 日志文件已初始化: " << m_logFilePath << std::endl;
    }
}

Logger::~Logger() {
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
}

std::string Logger::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm localTime;
    
#ifdef _WIN32
    localtime_s(&localTime, &nowTime);
#else
    localtime_r(&nowTime, &localTime);
#endif

    std::ostringstream oss;
    oss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string Logger::logTypeToString(LogType type) {
    switch (type) {
        case LogType::MYSQL:
            return "MYSQL";
        case LogType::REDIS:
            return "REDIS";
        default:
            return "UNKNOWN";
    }
}

void Logger::log(LogType type,
                 const std::string& clientIP,
                 const std::string& operation,
                 const std::string& command,
                 bool success,
                 long long executionTimeMs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_logFile.is_open()) {
        return;
    }

    std::ostringstream logLine;
    logLine << "[" << getCurrentTime() << "] "
            << "[" << logTypeToString(type) << "] "
            << "[IP: " << (clientIP.empty() ? "unknown" : clientIP) << "] "
            << "[Operation: " << operation << "] "
            << "[Success: " << (success ? "YES" : "NO") << "] ";
    
    if (executionTimeMs > 0) {
        logLine << "[Time: " << executionTimeMs << "ms] ";
    }
    
    logLine << "[Command: " << command << "]"
            << std::endl;

    m_logFile << logLine.str();
    m_logFile.flush();
    
    std::cout << logLine.str();
}