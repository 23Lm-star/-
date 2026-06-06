#ifndef MYSQL_API_HANDLER_H
#define MYSQL_API_HANDLER_H

#include "../DBConnection.h"
#include "HttpServer.h"
#include <memory>
#include <map>

namespace WebServer {

class MysqlApiHandler {
public:
    // 用户认证API
    static HttpResponse login(const HttpRequest& req);
    static HttpResponse logout(const HttpRequest& req);
    static HttpResponse currentUser(const HttpRequest& req);
    
    // 用户注册API
    static HttpResponse registerUser(const HttpRequest& req);
    
    // 用户管理API（超级管理员专用）
    static HttpResponse getUsers(const HttpRequest& req);
    static HttpResponse getPendingUsers(const HttpRequest& req);
    static HttpResponse approveUser(const HttpRequest& req);
    static HttpResponse rejectUser(const HttpRequest& req);
    static HttpResponse deleteUser(const HttpRequest& req);
    static HttpResponse changePassword(const HttpRequest& req);
    
    // MySQL数据库操作API
    static HttpResponse getTables(const HttpRequest& req);
    static HttpResponse getTableData(const HttpRequest& req);
    static HttpResponse insertRow(const HttpRequest& req);
    static HttpResponse updateRow(const HttpRequest& req);
    static HttpResponse deleteRow(const HttpRequest& req);
    static HttpResponse getTableStructure(const HttpRequest& req);
    static HttpResponse createTable(const HttpRequest& req);
    static HttpResponse dropTable(const HttpRequest& req);
    static HttpResponse addColumn(const HttpRequest& req);
    static HttpResponse dropColumn(const HttpRequest& req);
    static HttpResponse setPrimaryKey(const HttpRequest& req);
    static HttpResponse modifyColumn(const HttpRequest& req);
    static HttpResponse poolStatus(const HttpRequest& req);
    static HttpResponse updatePoolConfig(const HttpRequest& req);
    
private:
    // 从请求中获取用户名（辅助函数）
    static std::string getUsernameFromSession(const HttpRequest& req);
    
    static std::string toJson(const std::vector<std::vector<std::string>>& data, 
                              const std::vector<std::string>& headers);
    static std::string toJson(const std::map<std::string, std::string>& data);
};

}

#endif