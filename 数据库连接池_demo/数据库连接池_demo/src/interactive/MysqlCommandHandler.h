#ifndef INTERACTIVE_MYSQL_COMMAND_HANDLER_H
#define INTERACTIVE_MYSQL_COMMAND_HANDLER_H

#include "Command.h"
#include <functional>
#include <memory>

class MysqlConn;

namespace InteractiveShell {

class IMysqlConnectionProvider {
public:
    virtual ~IMysqlConnectionProvider() = default;
    virtual std::shared_ptr<MysqlConn> getConnection() = 0;
};

class MysqlCommandHandler : public ICommandHandler {
public:
    explicit MysqlCommandHandler(std::shared_ptr<IMysqlConnectionProvider> provider);
    
    CommandResult handle(const ParsedCommand& cmd) override;
    
    std::map<std::string, std::string> getSupportedCommands() const override;
    
    std::string getName() const override;
    
private:
    std::shared_ptr<IMysqlConnectionProvider> m_provider;
    
    CommandResult handleSelect(const ParsedCommand& cmd);
    CommandResult handleInsert(const ParsedCommand& cmd);
    CommandResult handleUpdate(const ParsedCommand& cmd);
    CommandResult handleDelete(const ParsedCommand& cmd);
    CommandResult handleShow(const ParsedCommand& cmd);
    CommandResult handleUse(const ParsedCommand& cmd);
    CommandResult handleRaw(const ParsedCommand& cmd);
    
    std::string joinArgs(const std::vector<std::string>& args, size_t start = 0) const;
};

}

#endif