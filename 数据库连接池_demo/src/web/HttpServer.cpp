#include "HttpServer.h"
#include "../auth/SessionManager.h"
#include "../auth/AuthHandler.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <fstream>
#include <thread>
#include <algorithm>
#include <vector>
#include <mutex>

namespace WebServer {

HttpServer::HttpServer(int port) : m_port(port), m_serverSocket(-1), m_running(false), m_authRequired(true) {}

HttpServer::~HttpServer() {
    stop();
}

bool HttpServer::start() {
    m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_serverSocket < 0) {
        std::cerr << "❌ 创建socket失败\n";
        return false;
    }
    
    int opt = 1;
    setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(m_port);
    
    if (bind(m_serverSocket, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "❌ 绑定端口 " << m_port << " 失败，可能端口已被占用\n";
        close(m_serverSocket);
        return false;
    }
    
    if (listen(m_serverSocket, 10) < 0) {
        std::cerr << "❌ 监听端口 " << m_port << " 失败\n";
        close(m_serverSocket);
        return false;
    }
    
    m_running = true;
    std::thread([this]() {
        while (m_running) {
            sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
            int clientSocket = accept(m_serverSocket, (sockaddr*)&clientAddr, &clientLen);
            if (clientSocket >= 0) {
                std::thread(&HttpServer::handleClient, this, clientSocket).detach();
            }
        }
    }).detach();
    
    return true;
}

void HttpServer::stop() {
    m_running = false;
    if (m_serverSocket >= 0) {
        close(m_serverSocket);
        m_serverSocket = -1;
    }
}

void HttpServer::get(const std::string& path, RequestHandler handler) {
    m_handlers["GET " + path] = handler;
}

void HttpServer::post(const std::string& path, RequestHandler handler) {
    m_handlers["POST " + path] = handler;
}

void HttpServer::put(const std::string& path, RequestHandler handler) {
    m_handlers["PUT " + path] = handler;
}

void HttpServer::del(const std::string& path, RequestHandler handler) {
    m_handlers["DELETE " + path] = handler;
}

void HttpServer::serveStatic(const std::string& path, const std::string& file) {
    m_handlers["GET " + path] = [file](const HttpRequest& req) {
        HttpResponse res;
        std::ifstream f(file);
        if (f.is_open()) {
            std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            res.body = content;
            if (file.find(".html") != std::string::npos) {
                res.contentType = "text/html; charset=utf-8";
            } else if (file.find(".js") != std::string::npos) {
                res.contentType = "text/javascript";
            } else if (file.find(".css") != std::string::npos) {
                res.contentType = "text/css";
            } else {
                res.contentType = "text/plain";
            }
        } else {
            res.statusCode = 404;
            res.body = "Not Found";
        }
        return res;
    };
}

void HttpServer::setAuthRequired(bool required) {
    m_authRequired = required;
}

// 检查路径是否为公开路径（无需认证即可访问）
bool HttpServer::isPublicPath(const std::string& path) {
    // 公开路径列表
    static const std::vector<std::string> publicPaths = {
        "/login",
        "/api/login",
        "/api/logout",
        "/api/current-user",
        "/api/register",
        "/api/cluster/",
        "/api/mysql/poolStatus",
        "/api/redis/poolStatus",
        "/api/mysql/updatePoolConfig",
        "/api/redis/updatePoolConfig",
        "/login.html",
        "/health",
        "/test"
    };
    
    for (const auto& publicPath : publicPaths) {
        if (path == publicPath || path.find(publicPath) == 0) {
            return true;
        }
    }
    return false;
}

// 从请求头中获取指定Cookie的值
std::string HttpServer::getCookie(const HttpRequest& req, const std::string& cookieName) {
    auto it = req.headers.find("Cookie");
    if (it != req.headers.end()) {
        std::string cookies = it->second;
        std::string target = cookieName + "=";
        size_t pos = cookies.find(target);
        if (pos != std::string::npos) {
            size_t start = pos + target.length();
            size_t end = cookies.find(';', start);
            if (end == std::string::npos) {
                end = cookies.length();
            }
            return cookies.substr(start, end - start);
        }
    }
    return "";
}

void HttpServer::handleClient(int clientSocket) {
    // 使用更大的缓冲区，并循环读取直到收到完整请求
    std::string requestData;
    char buffer[8192];
    
    while (true) {
        int bytesRead = read(clientSocket, buffer, sizeof(buffer));
        if (bytesRead <= 0) {
            if (requestData.empty()) {
                close(clientSocket);
                return;
            }
            break;
        }
        requestData.append(buffer, bytesRead);
        
        // 检查是否已收到完整的 HTTP 请求头（以 \r\n\r\n 结尾）
        size_t headerEnd = requestData.find("\r\n\r\n");
        if (headerEnd == std::string::npos) {
            continue;  // 头部未接收完，继续读
        }
        
        // 检查 Content-Length，判断 body 是否接收完整
        size_t clPos = requestData.find("Content-Length: ");
        if (clPos != std::string::npos && clPos < headerEnd) {
            size_t clStart = clPos + 16;
            size_t clEnd = requestData.find("\r\n", clStart);
            if (clEnd != std::string::npos) {
                int contentLength = std::stoi(requestData.substr(clStart, clEnd - clStart));
                int bodyReceived = (int)requestData.size() - (int)(headerEnd + 4);
                if (bodyReceived < contentLength) {
                    continue;  // body 未接收完，继续读
                }
            }
        }
        break;  // 请求已完整
    }
    
    if (requestData.empty()) {
        close(clientSocket);
        return;
    }
    
    // 获取客户端 IP
    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    getpeername(clientSocket, (sockaddr*)&clientAddr, &clientLen);
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, INET_ADDRSTRLEN);
    std::string clientIP = ipStr;
    
    HttpRequest req = parseRequest(requestData);
    req.clientIP = clientIP;
    
    std::string key = req.method + " " + req.path;
    HttpResponse res;
    
    std::cout << "[DEBUG] Handling request: " << key << ", client: " << clientIP << std::endl;
    
    // 如果启用了认证，则检查是否为受保护的路径
    if (m_authRequired && !isPublicPath(req.path)) {
        // 验证session_id
        std::string sessionId = getCookie(req, "session_id");
        std::cout << "[DEBUG] Session ID from cookie: " << sessionId << std::endl;
        
        if (!Auth::SessionManager::instance().validateSession(sessionId)) {
            // 未登录或会话无效，重定向到登录页
            std::cout << "[DEBUG] Session invalid, redirecting to login.html" << std::endl;
            res.statusCode = 302;
            res.headers["Location"] = "/login.html";
            std::string response = buildResponse(res);
            write(clientSocket, response.c_str(), response.size());
            close(clientSocket);
            return;
        }
        
        std::cout << "[DEBUG] Session valid" << std::endl;
        
        // 检查是否是监控页面路径，只有超级管理员可以访问
        if (req.path == "/monitor") {
            std::string username = Auth::SessionManager::instance().getSessionUser(sessionId);
            std::string role = Auth::SessionManager::instance().getSessionRole(sessionId);
            std::cout << "[DEBUG] Monitor access check - username: " << username << ", role: " << role << std::endl;
            if (role != "super_admin") {
                // 不是超级管理员，返回403禁止访问
                res.statusCode = 403;
                res.contentType = "text/html; charset=utf-8";
                res.body = "<!DOCTYPE html><html><head><meta charset='utf-8'><title>权限不足</title>";
                res.body += "<style>body{font-family:Arial,sans-serif;display:flex;justify-content:center;align-items:center;min-height:100vh;background:#f5f5f5;}";
                res.body += ".error-box{text-align:center;padding:40px;background:white;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1);}";
                res.body += "h1{color:#e74c3c;margin-bottom:20px;}p{color:#666;}a{color:#667eea;text-decoration:none;}</style></head>";
                res.body += "<body><div class='error-box'><h1>🚫 权限不足</h1><p>您没有权限访问监控页面</p>";
                res.body += "<p>当前角色: " + role + "</p>";
                res.body += "<p><a href='/'>返回首页</a></p></div></body></html>";
                std::string response = buildResponse(res);
                write(clientSocket, response.c_str(), response.size());
                close(clientSocket);
                return;
            }
        }
    }
    
    auto it = m_handlers.find(key);
    if (it != m_handlers.end()) {
        res = it->second(req);
    } else {
        res.statusCode = 404;
        res.body = "Not Found";
    }
    
    std::string response = buildResponse(res);
    write(clientSocket, response.c_str(), response.size());
    close(clientSocket);
}

HttpRequest HttpServer::parseRequest(const std::string& request) {
    HttpRequest req;
    
    // 先找出头部结束位置
    size_t headerEnd = request.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        headerEnd = request.size();
    }
    
    std::string headerPart = request.substr(0, headerEnd);
    std::istringstream iss(headerPart);
    std::string line;
    
    // 解析请求行
    std::getline(iss, line);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    std::istringstream firstLine(line);
    firstLine >> req.method >> req.path;
    
    // 解析请求头
    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) break;
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 1);
            size_t vstart = value.find_first_not_of(' ');
            if (vstart != std::string::npos) value = value.substr(vstart);
            size_t vend = value.find_last_not_of(" \r\n");
            if (vend != std::string::npos) value = value.substr(0, vend + 1);
            req.headers[key] = value;
        }
    }
    
    // 解析 body
    if (headerEnd + 4 < request.size()) {
        req.body = request.substr(headerEnd + 4);
    }
    
    // 解析 URL 查询参数
    size_t qmark = req.path.find('?');
    if (qmark != std::string::npos) {
        std::string query = req.path.substr(qmark + 1);
        req.path = req.path.substr(0, qmark);
        parseParams(query, req.params);
    }
    
    // 解析 POST/PUT body 参数
    if ((req.method == "POST" || req.method == "PUT") && !req.body.empty()) {
        parseParams(req.body, req.params);
    }
    
    return req;
}

void HttpServer::parseParams(const std::string& query, std::unordered_map<std::string, std::string>& params) {
    std::istringstream qss(query);
    std::string param;
    while (std::getline(qss, param, '&')) {
        size_t eq = param.find('=');
        if (eq != std::string::npos) {
            std::string key = urlDecode(param.substr(0, eq));
            std::string value = urlDecode(param.substr(eq + 1));
            params[key] = value;
        }
    }
}

std::string HttpServer::buildResponse(const HttpResponse& response) {
    std::ostringstream oss;
    // 根据状态码输出正确的状态文本
    std::string statusText = "OK";
    if (response.statusCode == 301) statusText = "Moved Permanently";
    else if (response.statusCode == 302) statusText = "Found";
    else if (response.statusCode == 400) statusText = "Bad Request";
    else if (response.statusCode == 401) statusText = "Unauthorized";
    else if (response.statusCode == 403) statusText = "Forbidden";
    else if (response.statusCode == 404) statusText = "Not Found";
    else if (response.statusCode == 500) statusText = "Internal Server Error";
    
    oss << "HTTP/1.1 " << response.statusCode << " " << statusText << "\r\n";
    oss << "Content-Type: " << response.contentType << "\r\n";
    oss << "Content-Length: " << response.body.size() << "\r\n";
    oss << "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n";
    oss << "Access-Control-Allow-Headers: Content-Type, Cookie\r\n";
    
    // 输出自定义响应头（如 Set-Cookie、Location）
    for (const auto& header : response.headers) {
        oss << header.first << ": " << header.second << "\r\n";
    }
    
    oss << "\r\n";
    oss << response.body;
    return oss.str();
}

std::string HttpServer::urlDecode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%' && i + 2 < str.size()) {
            int hex = std::stoi(str.substr(i + 1, 2), nullptr, 16);
            result += static_cast<char>(hex);
            i += 2;
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

}