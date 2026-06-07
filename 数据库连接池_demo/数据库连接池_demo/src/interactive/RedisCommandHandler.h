#ifndef INTERACTIVE_REDIS_COMMAND_HANDLER_H
#define INTERACTIVE_REDIS_COMMAND_HANDLER_H

#include "Command.h"
#include <functional>
#include <memory>

class RedisConn;

namespace InteractiveShell {

class IRedisConnectionProvider {
public:
    virtual ~IRedisConnectionProvider() = default;
    virtual std::shared_ptr<RedisConn> getConnection() = 0;
};

class RedisCommandHandler : public ICommandHandler {
public:
    explicit RedisCommandHandler(std::shared_ptr<IRedisConnectionProvider> provider);
    
    CommandResult handle(const ParsedCommand& cmd) override;
    
    std::map<std::string, std::string> getSupportedCommands() const override;
    
    std::string getName() const override;
    
private:
    std::shared_ptr<IRedisConnectionProvider> m_provider;
    
    CommandResult handleSet(const ParsedCommand& cmd);
    CommandResult handleGet(const ParsedCommand& cmd);
    CommandResult handleHset(const ParsedCommand& cmd);
    CommandResult handleHget(const ParsedCommand& cmd);
    CommandResult handleHgetall(const ParsedCommand& cmd);
    CommandResult handleDel(const ParsedCommand& cmd);
    CommandResult handleKeys(const ParsedCommand& cmd);
    CommandResult handleRaw(const ParsedCommand& cmd);
};

}

#endif