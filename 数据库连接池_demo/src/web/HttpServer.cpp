#include "HttpServer.h"
#include "../auth/SessionManager.h"
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
        "/login.html"
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
    char buffer[4096] = {0};
    int bytesRead = read(clientSocket, buffer, sizeof(buffer));
    
    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    getpeername(clientSocket, (sockaddr*)&clientAddr, &clientLen);
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, INET_ADDRSTRLEN);
    std::string clientIP = ipStr;
    
    if (bytesRead <= 0) {
        close(clientSocket);
        return;
    }
    
    std::string request(buffer, bytesRead);
    HttpRequest req = parseRequest(request);
    req.clientIP = clientIP;
    
    std::string key = req.method + " " + req.path;
    HttpResponse res;
    
    // 如果启用了认证，则检查是否为受保护的路径
    if (m_authRequired && !isPublicPath(req.path)) {
        // 验证session_id
        std::string sessionId = getCookie(req, "session_id");
        if (!Auth::SessionManager::instance().validateSession(sessionId)) {
            // 未登录或会话无效，重定向到登录页
            res.statusCode = 302;
            res.headers["Location"] = "http://172.22.136.134:8080/login.html";
            std::string response = buildResponse(res);
            write(clientSocket, response.c_str(), response.size());
            close(clientSocket);
            return;
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
    std::istringstream iss(request);
    std::string line;
    
    std::getline(iss, line);
    std::istringstream firstLine(line);
    firstLine >> req.method >> req.path;
    
    while (std::getline(iss, line) && line != "\r") {
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 2);
            if (value.back() == '\r') value.pop_back();
            req.headers[key] = value;
        }
    }
    
    std::getline(iss, line);
    req.body = line;
    
    size_t qmark = req.path.find('?');
    if (qmark != std::string::npos) {
        std::string query = req.path.substr(qmark + 1);
        req.path = req.path.substr(0, qmark);
        
        parseParams(query, req.params);
    }
    
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
    oss << "HTTP/1.1 " << response.statusCode << " OK\r\n";
    oss << "Content-Type: " << response.contentType << "\r\n";
    oss << "Content-Length: " << response.body.size() << "\r\n";
    oss << "Access-Control-Allow-Origin: http://172.22.136.134:8080\r\n";
    oss << "Access-Control-Allow-Credentials: true\r\n";
    oss << "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n";
    oss << "Access-Control-Allow-Headers: Content-Type, Cookie\r\n";
    
    // 输出自定义响应头（如 Set-Cookie）
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