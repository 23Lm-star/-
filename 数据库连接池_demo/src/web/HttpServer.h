#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <string>
#include <functional>
#include <unordered_map>
#include <memory>

namespace WebServer {

struct HttpRequest {
    std::string method;
    std::string path;
    std::string body;
    std::string clientIP;
    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> params;
};

struct HttpResponse {
    int statusCode;
    std::string contentType;
    std::string body;
    std::unordered_map<std::string, std::string> headers;
    
    HttpResponse() : statusCode(200), contentType("text/plain") {}
};

typedef std::function<HttpResponse(const HttpRequest&)> RequestHandler;

class HttpServer {
public:
    HttpServer(int port = 8080);
    ~HttpServer();
    
    bool start();
    void stop();
    
    void get(const std::string& path, RequestHandler handler);
    void post(const std::string& path, RequestHandler handler);
    void put(const std::string& path, RequestHandler handler);
    void del(const std::string& path, RequestHandler handler);
    
    void serveStatic(const std::string& path, const std::string& file);
    
    // 设置是否启用认证（默认启用）
    void setAuthRequired(bool required);
    
private:
    int m_port;
    int m_serverSocket;
    bool m_running;
    bool m_authRequired;  // 是否启用认证
    std::unordered_map<std::string, RequestHandler> m_handlers;
    
    // 检查路径是否为公开路径（无需认证）
    bool isPublicPath(const std::string& path);
    // 从请求头中获取Cookie
    std::string getCookie(const HttpRequest& req, const std::string& cookieName);
    
    void handleClient(int clientSocket);
    HttpRequest parseRequest(const std::string& request);
    void parseParams(const std::string& query, std::unordered_map<std::string, std::string>& params);
    std::string buildResponse(const HttpResponse& response);
    std::string urlDecode(const std::string& str);
};

}

#endif