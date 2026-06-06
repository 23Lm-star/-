#ifndef REDIS_API_HANDLER_H
#define REDIS_API_HANDLER_H

#include "../DBConnection.h"
#include "HttpServer.h"

namespace WebServer {

class RedisApiHandler {
public:
    static HttpResponse get(const HttpRequest& req);
    static HttpResponse set(const HttpRequest& req);
    static HttpResponse hget(const HttpRequest& req);
    static HttpResponse hset(const HttpRequest& req);
    static HttpResponse hgetall(const HttpRequest& req);
    static HttpResponse hdel(const HttpRequest& req);
    static HttpResponse del(const HttpRequest& req);
    static HttpResponse keys(const HttpRequest& req);
    static HttpResponse poolStatus(const HttpRequest& req);
    static HttpResponse updatePoolConfig(const HttpRequest& req);
};

}

#endif