# 数据库连接池集群部署指南

## 📋 概述

本文档介绍如何将数据库连接池Web服务部署为集群架构，实现高可用和负载均衡。

## 🏗️ 架构设计

```
┌─────────────────────────────────────────────────────────┐
│                   用户请求 (浏览器)                       │
└──────────────────────────┬──────────────────────────────┘
                           │ Port 80
                           ▼
┌─────────────────────────────────────────────────────────┐
│              Nginx 负载均衡器 (端口 80)                   │
│  ┌─────────────────────────────────────────────────┐   │
│  │ least_conn (最少连接优先)                        │   │
│  │ - 127.0.0.1:8080 (节点1)                       │   │
│  │ - 127.0.0.1:8081 (节点2)                       │   │
│  │ - 127.0.0.1:8082 (节点3)                       │   │
│  └─────────────────────────────────────────────────┘   │
└──────────────────────────┬──────────────────────────────┘
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
        ▼                  ▼                  ▼
┌────────────────┐ ┌────────────────┐ ┌────────────────┐
│    节点 1       │ │    节点 2       │ │    节点 3       │
│ ┌────────────┐ │ │ ┌────────────┐ │ │ ┌────────────┐ │
│ │ Web服务器  │ │ │ │ Web服务器  │ │ │ │ Web服务器  │ │
│ │ (端口8080) │ │ │ │ (端口8081) │ │ │ │ (端口8082) │ │
│ └────────────┘ │ │ └────────────┘ │ │ └────────────┘ │
│ ┌────────────┐ │ │ ┌────────────┐ │ │ ┌────────────┐ │
│ │ MySQL连接池│ │ │ │ MySQL连接池│ │ │ │ MySQL连接池│ │
│ └────────────┘ │ │ └────────────┘ │ │ └────────────┘ │
│ ┌────────────┐ │ │ ┌────────────┐ │ │ ┌────────────┐ │
│ │Redis连接池 │ │ │ │Redis连接池 │ │ │ │Redis连接池 │ │
│ └────────────┘ │ │ └────────────┘ │ │ └────────────┘ │
│ ┌────────────┐ │ │ ┌────────────┐ │ │ ┌────────────┐ │
│ │Redis会话   │ │ │ │Redis会话   │ │ │ │Redis会话   │ │
│ │存储(共享)  │ │ │ │存储(共享)  │ │ │ │存储(共享)  │ │
│ └────────────┘ │ │ └────────────┘ │ │ └────────────┘ │
└────────────────┘ └────────────────┘ └────────────────┘
        │                  │                  │
        └──────────────────┼──────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────┐
│                      共享存储层                         │
│  ┌─────────────────┐      ┌─────────────────┐        │
│  │   Redis容器     │      │   MySQL容器     │        │
│  │   (会话存储)    │      │   (数据存储)     │        │
│  │   Port: 6379   │      │   Port: 3306    │        │
│  └─────────────────┘      └─────────────────┘        │
└─────────────────────────────────────────────────────────┘
```

---

## 📁 集群文件结构

```
cluster/
├── nginx.conf           # Nginx负载均衡配置文件
├── start_cluster.sh     # 集群启动脚本
├── stop_cluster.sh      # 集群停止脚本
├── node_8080.log        # 节点1日志
├── node_8081.log        # 节点2日志
└── node_8082.log        # 节点3日志
```

---

## 🚀 快速开始

### 1. 前提条件

确保以下服务已安装并运行：

```bash
# 检查MySQL容器
docker ps | grep mysql

# 检查Redis容器
docker ps | grep redis

# 检查Nginx
nginx -v
```

### 2. 启动集群

```bash
# 进入集群目录
cd cluster

# 启动集群
./start_cluster.sh
```

### 3. 访问服务

| 服务 | 地址 |
|------|------|
| 统一入口 | http://localhost/ |
| 节点1 | http://localhost:8080/ |
| 节点2 | http://localhost:8081/ |
| 节点3 | http://localhost:8082/ |
| 健康检查 | http://localhost/health |

### 4. 停止集群

```bash
./stop_cluster.sh
```

---

## 🔧 手动操作

### 启动单个节点

```bash
# 节点1
./web_server --port 8080

# 节点2
./web_server --port 8081

# 节点3
./web_server --port 8082
```

### 配置Nginx

```bash
# 复制配置文件
sudo cp nginx.conf /etc/nginx/nginx.conf

# 测试配置
sudo nginx -t

# 重启Nginx
sudo systemctl restart nginx

# 或重新加载配置
sudo nginx -s reload
```

### 查看健康状态

```bash
# 检查单个节点
curl http://localhost:8080/health
curl http://localhost:8081/health
curl http://localhost:8082/health

# 通过Nginx
curl http://localhost/health
```

---

## 📊 监控与维护

### 查看节点日志

```bash
tail -f node_8080.log
tail -f node_8081.log
tail -f node_8082.log
```

### 查看Nginx状态

```bash
# 查看Nginx进程
ps aux | grep nginx

# 查看Nginx错误日志
sudo tail -f /var/log/nginx/error.log

# 查看Nginx访问日志
sudo tail -f /var/log/nginx/access.log
```

### 测试负载均衡

```bash
# 多次请求，观察不同节点的响应
for i in {1..10}; do
    curl -s http://localhost/health | jq .port
done
```

---

## ⚙️ 配置说明

### 修改节点数量

编辑 `nginx.conf` 文件：

```nginx
upstream backend {
    least_conn;

    # 添加/删除节点
    server 127.0.0.1:8080 weight=1;
    server 127.0.0.1:8081 weight=1;
    server 127.0.0.1:8082 weight=1;

    # 可选：添加更多节点
    # server 127.0.0.1:8083 weight=1;
}
```

### 修改负载均衡策略

```nginx
# 轮询（默认）
upstream backend {
    server 127.0.0.1:8080;
    server 127.0.0.1:8081;
    server 127.0.0.1:8082;
}

# IP哈希（会话粘性）
upstream backend {
    ip_hash;
    server 127.0.0.1:8080;
    server 127.0.0.1:8081;
    server 127.0.0.1:8082;
}

# 最少连接
upstream backend {
    least_conn;
    server 127.0.0.1:8080;
    server 127.0.0.1:8081;
    server 127.0.0.1:8082;
}
```

---

## 🐛 常见问题

### Q1: 节点启动失败？

**检查端口占用：**
```bash
netstat -tlnp | grep 8080
lsof -i :8080
```

**解决：**
```bash
# 杀死占用端口的进程
kill -9 <PID>
```

### Q2: Nginx启动失败？

**检查配置：**
```bash
sudo nginx -t
```

**常见错误：**
- 权限不足 - 使用 `sudo`
- 端口被占用 - 检查80端口
- 配置语法错误 - 查看错误信息

### Q3: 会话不共享？

**检查Redis连接：**
```bash
redis-cli ping
```

**检查会话数据：**
```bash
redis-cli keys "session:*"
redis-cli get "session:<session_id>"
```

### Q4: 健康检查返回unhealthy？

**检查MySQL：**
```bash
mysql -h localhost -u root -p -e "SELECT 1"
```

**检查Redis：**
```bash
redis-cli ping
```

---

## 📈 性能优化

### 1. 调整连接池大小

```cpp
// 在web_server中调整
DBConnection::initMysqlPool(5, 20, 2, 5000);
```

### 2. 调整Nginx worker进程

```nginx
worker_processes auto;  # 自动检测CPU核心数
worker_connections 1024;  # 单进程连接数
```

### 3. 启用Gzip压缩

```nginx
gzip on;
gzip_types text/plain text/css application/json application/javascript;
```

---

## 🔒 安全建议

### 1. 限制节点访问

```nginx
# 只允许本地访问
server 127.0.0.1:8080;
```

### 2. 添加防火墙规则

```bash
# 只允许本地访问Web节点
sudo iptables -A INPUT -p tcp -s 127.0.0.1 --dport 8080 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 8080 -j DROP
```

### 3. 启用HTTPS

```nginx
server {
    listen 443 ssl;
    ssl_certificate /path/to/cert.pem;
    ssl_certificate_key /path/to/key.pem;

    location / {
        proxy_pass http://backend;
    }
}
```

---

## 📞 技术支持

如有问题，请检查：

1. 所有容器是否正常运行
2. 端口是否被占用
3. 日志文件中的错误信息
4. Redis和MySQL连接是否正常
