#ifndef INTERACTIVE_COMMAND_H
#define INTERACTIVE_COMMAND_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <iostream>

namespace InteractiveShell {

enum class CommandType {
    MYSQL,
    REDIS,
    HELP,
    EXIT,
    UNKNOWN
};

struct ParsedCommand {
    CommandType type;
    std::string raw;
    std::string dbType;
    std::string action;
    std::vector<std::string> args;
    std::map<std::string, std::string> options;
};

struct CommandResult {
    bool success;
    std::string message;
    std::vector<std::vector<std::string>> data;
    std::vector<std::string> headers;
    int affectedRows;
    
    CommandResult() : success(false), affectedRows(0) {}
};

class ICommandHandler {
public:
    virtual ~ICommandHandler() = default;
    
    virtual CommandResult handle(const ParsedCommand& cmd) = 0;
    
    virtual std::map<std::string, std::string> getSupportedCommands() const = 0;
    
    virtual std::string getName() const = 0;
};

class CommandParser {
public:
    static ParsedCommand parse(const std::string& input);
    
    static std::vector<std::string> tokenize(const std::string& input);
};

}

#endif