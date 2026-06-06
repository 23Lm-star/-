#include "InteractiveShell.h"
#include <iostream>
#include <string>
#include <iomanip>

namespace InteractiveShell {

InteractiveShell::InteractiveShell() : m_running(false) {
}

void InteractiveShell::registerMysqlHandler(std::shared_ptr<MysqlCommandHandler> handler) {
    m_mysqlHandler = std::move(handler);
}

void InteractiveShell::registerRedisHandler(std::shared_ptr<RedisCommandHandler> handler) {
    m_redisHandler = std::move(handler);
}

void InteractiveShell::run() {
    m_running = true;
    showWelcome();
    
    std::string input;
    while (m_running) {
        std::cout << "\n> ";
        std::getline(std::cin, input);
        
        if (input.empty()) {
            continue;
        }
        
        ParsedCommand cmd = CommandParser::parse(input);
        executeCommand(cmd);
    }
    
    std::cout << "\n再见!\n";
}

void InteractiveShell::stop() {
    m_running = false;
}

void InteractiveShell::showWelcome() {
    std::cout << "========================================\n";
    std::cout << "    数据库连接池 - 交互式Shell\n";
    std::cout << "========================================\n";
    std::cout << "\n输入 'help' 查看帮助，输入 'exit' 或 'quit' 退出\n";
    std::cout << "\n已连接的数据库:\n";
    if (m_mysqlHandler) {
        std::cout << "  [x] MySQL\n";
    }
    if (m_redisHandler) {
        std::cout << "  [x] Redis\n";
    }
}

void InteractiveShell::showHelp(const std::string& topic) {
    if (topic == "mysql" && m_mysqlHandler) {
        std::cout << "\n=== MySQL 命令 ===\n";
        auto commands = m_mysqlHandler->getSupportedCommands();
        for (const auto& pair : commands) {
            std::cout << "  " << std::left << std::setw(30) << pair.first << " - " << pair.second << "\n";
        }
        std::cout << "\n示例:\n";
        std::cout << "  select * from users\n";
        std::cout << "  show tables\n";
        std::cout << "  mysql select * from users\n";
    } else if (topic == "redis" && m_redisHandler) {
        std::cout << "\n=== Redis 命令 ===\n";
        auto commands = m_redisHandler->getSupportedCommands();
        for (const auto& pair : commands) {
            std::cout << "  " << std::left << std::setw(30) << pair.first << " - " << pair.second << "\n";
        }
        std::cout << "\n示例:\n";
        std::cout << "  set mykey hello\n";
        std::cout << "  get mykey\n";
        std::cout << "  hset user:1 name zhangsan\n";
        std::cout << "  hgetall user:1\n";
    } else {
        std::cout << "\n=== 帮助 ===\n";
        std::cout << "\n通用命令:\n";
        std::cout << "  help [mysql|redis]    - 显示帮助信息\n";
        std::cout << "  exit|quit|q           - 退出Shell\n";
        
        if (m_mysqlHandler) {
            std::cout << "\nMySQL命令 (直接输入或用 mysql 前缀):\n";
            std::cout << "  select, insert, update, delete, show, use, ...\n";
            std::cout << "  或输入 'help mysql' 查看详细\n";
        }
        
        if (m_redisHandler) {
            std::cout << "\nRedis命令 (直接输入或用 redis 前缀):\n";
            std::cout << "  set, get, hset, hget, hgetall, ...\n";
            std::cout << "  或输入 'help redis' 查看详细\n";
        }
    }
}

void InteractiveShell::executeCommand(const ParsedCommand& cmd) {
    switch (cmd.type) {
        case CommandType::EXIT:
            stop();
            break;
            
        case CommandType::HELP:
            showHelp(cmd.action);
            break;
            
        case CommandType::MYSQL:
            if (m_mysqlHandler) {
                CommandResult result = m_mysqlHandler->handle(cmd);
                displayResult(result);
            } else {
                std::cout << "错误: MySQL未连接\n";
            }
            break;
            
        case CommandType::REDIS:
            if (m_redisHandler) {
                CommandResult result = m_redisHandler->handle(cmd);
                displayResult(result);
            } else {
                std::cout << "错误: Redis未连接\n";
            }
            break;
            
        case CommandType::UNKNOWN:
        default:
            std::cout << "未知命令，请输入 'help' 查看帮助\n";
            break;
    }
}

void InteractiveShell::displayResult(const CommandResult& result) {
    if (result.success) {
        std::cout << "[成功] " << result.message << "\n";
    } else {
        std::cout << "[失败] " << result.message << "\n";
    }
    
    if (!result.data.empty()) {
        displayTable(result.headers, result.data);
    }
    
    if (result.affectedRows > 0) {
        std::cout << "影响行数: " << result.affectedRows << "\n";
    }
}

void InteractiveShell::displayTable(const std::vector<std::string>& headers, 
                                   const std::vector<std::vector<std::string>>& data) {
    if (headers.empty() && data.empty()) {
        return;
    }
    
    std::vector<size_t> widths;
    if (!headers.empty()) {
        for (const auto& header : headers) {
            widths.push_back(header.length());
        }
    } else if (!data.empty()) {
        for (const auto& cell : data[0]) {
            widths.push_back(cell.length());
        }
    }
    
    for (const auto& row : data) {
        for (size_t i = 0; i < row.size() && i < widths.size(); ++i) {
            if (row[i].length() > widths[i]) {
                widths[i] = row[i].length();
            }
        }
    }
    
    for (auto& w : widths) {
        if (w < 5) w = 5;
    }
    
    std::cout << "+";
    for (size_t w : widths) {
        std::cout << std::string(w + 2, '-') << "+";
    }
    std::cout << "\n";
    
    if (!headers.empty()) {
        std::cout << "|";
        for (size_t i = 0; i < headers.size() && i < widths.size(); ++i) {
            std::cout << " " << std::left << std::setw(widths[i]) << headers[i] << " |";
        }
        std::cout << "\n";
        
        std::cout << "+";
        for (size_t w : widths) {
            std::cout << std::string(w + 2, '-') << "+";
        }
        std::cout << "\n";
    }
    
    for (const auto& row : data) {
        std::cout << "|";
        for (size_t i = 0; i < row.size() && i < widths.size(); ++i) {
            std::cout << " " << std::left << std::setw(widths[i]) << row[i] << " |";
        }
        std::cout << "\n";
    }
    
    if (!data.empty()) {
        std::cout << "+";
        for (size_t w : widths) {
            std::cout << std::string(w + 2, '-') << "+";
        }
        std::cout << "\n";
    }
}

}