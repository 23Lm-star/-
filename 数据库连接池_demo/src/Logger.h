#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>

class Logger {
public:
    enum class LogType {
        MYSQL,
        REDIS
    };

    static Logger& getInstance();

    void log(LogType type,
             const std::string& clientIP,
             const std::string& operation,
             const std::string& command,
             bool success,
             long long executionTimeMs = 0);

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string getCurrentTime();
    std::string logTypeToString(LogType type);

    std::ofstream m_logFile;
    std::mutex m_mutex;
    std::string m_logFilePath;
};

#endif // LOGGER_H