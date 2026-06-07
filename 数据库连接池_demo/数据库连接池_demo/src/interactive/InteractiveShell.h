#ifndef INTERACTIVE_SHELL_H
#define INTERACTIVE_SHELL_H

#include "Command.h"
#include "MysqlCommandHandler.h"
#include "RedisCommandHandler.h"
#include <memory>
#include <vector>

namespace InteractiveShell {

class InteractiveShell {
public:
    InteractiveShell();
    
    void registerMysqlHandler(std::shared_ptr<MysqlCommandHandler> handler);
    
    void registerRedisHandler(std::shared_ptr<RedisCommandHandler> handler);
    
    void run();
    
    void stop();
    
private:
    std::shared_ptr<MysqlCommandHandler> m_mysqlHandler;
    std::shared_ptr<RedisCommandHandler> m_redisHandler;
    bool m_running;
    
    void showWelcome();
    
    void showHelp(const std::string& topic = "");
    
    void executeCommand(const ParsedCommand& cmd);
    
    void displayResult(const CommandResult& result);
    
    void displayTable(const std::vector<std::string>& headers, 
                     const std::vector<std::vector<std::string>>& data);
};

}

#endif