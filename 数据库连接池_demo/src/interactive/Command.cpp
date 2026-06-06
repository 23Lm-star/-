#include "Command.h"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace InteractiveShell {

std::vector<std::string> CommandParser::tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::string current;
    bool inQuotes = false;
    char quoteChar = '\0';
    
    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        
        if (inQuotes) {
            if (c == quoteChar) {
                inQuotes = false;
                quoteChar = '\0';
            } else {
                current += c;
            }
        } else {
            if (c == '\'' || c == '"') {
                inQuotes = true;
                quoteChar = c;
            } else if (std::isspace(static_cast<unsigned char>(c))) {
                if (!current.empty()) {
                    tokens.push_back(current);
                    current.clear();
                }
            } else {
                current += c;
            }
        }
    }
    
    if (!current.empty()) {
        tokens.push_back(current);
    }
    
    return tokens;
}

ParsedCommand CommandParser::parse(const std::string& input) {
    ParsedCommand cmd;
    cmd.raw = input;
    cmd.type = CommandType::UNKNOWN;
    
    std::string trimmed = input;
    trimmed.erase(trimmed.begin(), std::find_if(trimmed.begin(), trimmed.end(),
        [](int ch) { return !std::isspace(static_cast<unsigned char>(ch)); }));
    trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(),
        [](int ch) { return !std::isspace(static_cast<unsigned char>(ch)); }).base(),
        trimmed.end());
    
    if (trimmed.empty()) {
        return cmd;
    }
    
    std::vector<std::string> tokens = tokenize(trimmed);
    if (tokens.empty()) {
        return cmd;
    }
    
    std::string first = tokens[0];
    std::transform(first.begin(), first.end(), first.begin(), ::tolower);
    
    if (first == "exit" || first == "quit" || first == "q") {
        cmd.type = CommandType::EXIT;
        return cmd;
    }
    
    if (first == "help" || first == "h" || first == "?") {
        cmd.type = CommandType::HELP;
        if (tokens.size() > 1) {
            cmd.action = tokens[1];
        }
        return cmd;
    }
    
    if (first == "mysql" || first == "m") {
        cmd.type = CommandType::MYSQL;
        cmd.dbType = "mysql";
        if (tokens.size() > 1) {
            cmd.action = tokens[1];
            std::transform(cmd.action.begin(), cmd.action.end(), cmd.action.begin(), ::tolower);
            if (tokens.size() > 2) {
                cmd.args.assign(tokens.begin() + 2, tokens.end());
            }
        }
    } else if (first == "redis" || first == "r") {
        cmd.type = CommandType::REDIS;
        cmd.dbType = "redis";
        if (tokens.size() > 1) {
            cmd.action = tokens[1];
            std::transform(cmd.action.begin(), cmd.action.end(), cmd.action.begin(), ::tolower);
            if (tokens.size() > 2) {
                cmd.args.assign(tokens.begin() + 2, tokens.end());
            }
        }
    } else {
        cmd.action = first;
        std::transform(cmd.action.begin(), cmd.action.end(), cmd.action.begin(), ::tolower);
        
        if (cmd.action == "select" || cmd.action == "insert" || 
            cmd.action == "update" || cmd.action == "delete" ||
            cmd.action == "show" || cmd.action == "use") {
            cmd.type = CommandType::MYSQL;
            cmd.dbType = "mysql";
            if (tokens.size() > 1) {
                cmd.args.assign(tokens.begin() + 1, tokens.end());
            }
        } else if (cmd.action == "set" || cmd.action == "get" || 
                 cmd.action == "hset" || cmd.action == "hget" ||
                 cmd.action == "hgetall" || cmd.action == "del") {
            cmd.type = CommandType::REDIS;
            cmd.dbType = "redis";
            if (tokens.size() > 1) {
                cmd.args.assign(tokens.begin() + 1, tokens.end());
            }
        }
    }
    
    return cmd;
}

}