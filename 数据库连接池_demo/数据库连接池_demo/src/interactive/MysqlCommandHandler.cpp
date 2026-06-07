#include "MysqlCommandHandler.h"
#include "../MysqlConn.h"
#include <mysql/mysql.h>
#include <sstream>
#include <algorithm>

namespace InteractiveShell {

MysqlCommandHandler::MysqlCommandHandler(std::shared_ptr<IMysqlConnectionProvider> provider)
    : m_provider(std::move(provider)) {
}

std::string MysqlCommandHandler::getName() const {
    return "MySQL";
}

std::map<std::string, std::string> MysqlCommandHandler::getSupportedCommands() const {
    return {
        {"select", "执行SELECT查询"},
        {"insert", "执行INSERT语句"},
        {"update", "执行UPDATE语句"},
        {"delete", "执行DELETE语句"},
        {"show", "显示表或数据库列表"},
        {"use", "切换数据库"},
        {"raw", "直接执行任意SQL语句"}
    };
}

std::string MysqlCommandHandler::joinArgs(const std::vector<std::string>& args, size_t start) const {
    std::string result;
    for (size_t i = start; i < args.size(); ++i) {
        if (i > start) {
            result += " ";
        }
        result += args[i];
    }
    return result;
}

CommandResult MysqlCommandHandler::handle(const ParsedCommand& cmd) {
    CommandResult result;
    
    if (!m_provider) {
        result.success = false;
        result.message = "MySQL连接提供者未初始化";
        return result;
    }
    
    if (cmd.action == "select") {
        return handleSelect(cmd);
    } else if (cmd.action == "insert") {
        return handleInsert(cmd);
    } else if (cmd.action == "update") {
        return handleUpdate(cmd);
    } else if (cmd.action == "delete") {
        return handleDelete(cmd);
    } else if (cmd.action == "show") {
        return handleShow(cmd);
    } else if (cmd.action == "use") {
        return handleUse(cmd);
    } else {
        return handleRaw(cmd);
    }
}

CommandResult MysqlCommandHandler::handleSelect(const ParsedCommand& cmd) {
    CommandResult result;
    std::string sql = "SELECT " + joinArgs(cmd.args);
    
    try {
        auto conn = m_provider->getConnection();
        if (!conn || !conn->isValid()) {
            result.success = false;
            result.message = "无法获取有效的MySQL连接";
            return result;
        }
        
        MYSQL_RES* mysqlResult = conn->executeQuery(sql);
        if (!mysqlResult) {
            result.success = false;
            result.message = "查询执行失败";
            return result;
        }
        
        MYSQL_FIELD* fields = mysql_fetch_fields(mysqlResult);
        unsigned int numFields = mysql_num_fields(mysqlResult);
        for (unsigned int i = 0; i < numFields; ++i) {
            result.headers.push_back(fields[i].name);
        }
        
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(mysqlResult))) {
            std::vector<std::string> rowData;
            for (unsigned int i = 0; i < numFields; ++i) {
                rowData.push_back(row[i] ? row[i] : "NULL");
            }
            result.data.push_back(rowData);
        }
        
        mysql_free_result(mysqlResult);
        result.success = true;
        result.message = "查询成功，返回 " + std::to_string(result.data.size()) + " 行";
        
    } catch (const std::exception& e) {
        result.success = false;
        result.message = "执行出错: " + std::string(e.what());
    }
    
    return result;
}

CommandResult MysqlCommandHandler::handleInsert(const ParsedCommand& cmd) {
    CommandResult result;
    std::string sql = "INSERT " + joinArgs(cmd.args);
    
    try {
        auto conn = m_provider->getConnection();
        if (!conn || !conn->isValid()) {
            result.success = false;
            result.message = "无法获取有效的MySQL连接";
            return result;
        }
        
        bool ret = conn->executeInsert(sql);
        if (ret) {
            result.success = true;
            result.affectedRows = 1;
            result.message = "插入成功，ID: " + std::to_string(conn->getLastInsertId());
        } else {
            result.success = false;
            result.message = "插入失败";
        }
        
    } catch (const std::exception& e) {
        result.success = false;
        result.message = "执行出错: " + std::string(e.what());
    }
    
    return result;
}

CommandResult MysqlCommandHandler::handleUpdate(const ParsedCommand& cmd) {
    CommandResult result;
    std::string sql = "UPDATE " + joinArgs(cmd.args);
    
    try {
        auto conn = m_provider->getConnection();
        if (!conn || !conn->isValid()) {
            result.success = false;
            result.message = "无法获取有效的MySQL连接";
            return result;
        }
        
        bool ret = conn->executeUpdate(sql);
        if (ret) {
            result.success = true;
            result.affectedRows = conn->getAffectedRows();
            result.message = "更新成功，影响 " + std::to_string(result.affectedRows) + " 行";
        } else {
            result.success = false;
            result.message = "更新失败";
        }
        
    } catch (const std::exception& e) {
        result.success = false;
        result.message = "执行出错: " + std::string(e.what());
    }
    
    return result;
}

CommandResult MysqlCommandHandler::handleDelete(const ParsedCommand& cmd) {
    CommandResult result;
    std::string sql = "DELETE " + joinArgs(cmd.args);
    
    try {
        auto conn = m_provider->getConnection();
        if (!conn || !conn->isValid()) {
            result.success = false;
            result.message = "无法获取有效的MySQL连接";
            return result;
        }
        
        bool ret = conn->executeUpdate(sql);
        if (ret) {
            result.success = true;
            result.affectedRows = conn->getAffectedRows();
            result.message = "删除成功，影响 " + std::to_string(result.affectedRows) + " 行";
        } else {
            result.success = false;
            result.message = "删除失败";
        }
        
    } catch (const std::exception& e) {
        result.success = false;
        result.message = "执行出错: " + std::string(e.what());
    }
    
    return result;
}

CommandResult MysqlCommandHandler::handleShow(const ParsedCommand& cmd) {
    CommandResult result;
    std::string sql = "SHOW " + joinArgs(cmd.args);
    
    try {
        auto conn = m_provider->getConnection();
        if (!conn || !conn->isValid()) {
            result.success = false;
            result.message = "无法获取有效的MySQL连接";
            return result;
        }
        
        MYSQL_RES* mysqlResult = conn->executeQuery(sql);
        if (!mysqlResult) {
            result.success = false;
            result.message = "查询执行失败";
            return result;
        }
        
        MYSQL_FIELD* fields = mysql_fetch_fields(mysqlResult);
        unsigned int numFields = mysql_num_fields(mysqlResult);
        for (unsigned int i = 0; i < numFields; ++i) {
            result.headers.push_back(fields[i].name);
        }
        
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(mysqlResult))) {
            std::vector<std::string> rowData;
            for (unsigned int i = 0; i < numFields; ++i) {
                rowData.push_back(row[i] ? row[i] : "NULL");
            }
            result.data.push_back(rowData);
        }
        
        mysql_free_result(mysqlResult);
        result.success = true;
        result.message = "查询成功";
        
    } catch (const std::exception& e) {
        result.success = false;
        result.message = "执行出错: " + std::string(e.what());
    }
    
    return result;
}

CommandResult MysqlCommandHandler::handleUse(const ParsedCommand& cmd) {
    CommandResult result;
    if (cmd.args.empty()) {
        result.success = false;
        result.message = "请指定数据库名: use <database>";
        return result;
    }
    
    std::string sql = "USE " + cmd.args[0];
    
    try {
        auto conn = m_provider->getConnection();
        if (!conn || !conn->isValid()) {
            result.success = false;
            result.message = "无法获取有效的MySQL连接";
            return result;
        }
        
        bool ret = conn->executeUpdate(sql);
        if (ret) {
            result.success = true;
            result.message = "已切换到数据库: " + cmd.args[0];
        } else {
            result.success = false;
            result.message = "切换数据库失败";
        }
        
    } catch (const std::exception& e) {
        result.success = false;
        result.message = "执行出错: " + std::string(e.what());
    }
    
    return result;
}

CommandResult MysqlCommandHandler::handleRaw(const ParsedCommand& cmd) {
    CommandResult result;
    std::string sql = cmd.raw;
    
    if (cmd.dbType == "mysql" && !cmd.action.empty()) {
        sql = cmd.action + " " + joinArgs(cmd.args);
    }
    
    try {
        auto conn = m_provider->getConnection();
        if (!conn || !conn->isValid()) {
            result.success = false;
            result.message = "无法获取有效的MySQL连接";
            return result;
        }
        
        std::string upperSql = sql;
        std::transform(upperSql.begin(), upperSql.end(), upperSql.begin(), ::toupper);
        
        if (upperSql.find("SELECT") == 0 || upperSql.find("SHOW") == 0 || 
            upperSql.find("DESCRIBE") == 0 || upperSql.find("DESC") == 0) {
            MYSQL_RES* mysqlResult = conn->executeQuery(sql);
            if (!mysqlResult) {
                result.success = false;
                result.message = "查询执行失败";
                return result;
            }
            
            MYSQL_FIELD* fields = mysql_fetch_fields(mysqlResult);
            unsigned int numFields = mysql_num_fields(mysqlResult);
            for (unsigned int i = 0; i < numFields; ++i) {
                result.headers.push_back(fields[i].name);
            }
            
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(mysqlResult))) {
                std::vector<std::string> rowData;
                for (unsigned int i = 0; i < numFields; ++i) {
                    rowData.push_back(row[i] ? row[i] : "NULL");
                }
                result.data.push_back(rowData);
            }
            
            mysql_free_result(mysqlResult);
            result.success = true;
            result.message = "查询成功，返回 " + std::to_string(result.data.size()) + " 行";
        } else {
            bool ret = conn->executeUpdate(sql);
            if (ret) {
                result.success = true;
                result.affectedRows = conn->getAffectedRows();
                result.message = "执行成功，影响 " + std::to_string(result.affectedRows) + " 行";
            } else {
                result.success = false;
                result.message = "执行失败";
            }
        }
        
    } catch (const std::exception& e) {
        result.success = false;
        result.message = "执行出错: " + std::string(e.what());
    }
    
    return result;
}

}