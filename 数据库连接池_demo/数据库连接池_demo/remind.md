# 数据库连接池项目文档

## 📋 项目概述

本项目是一个基于C++11开发的RAII连接池组件，支持MySQL和Redis数据库连接管理。项目提供了三种交互方式：命令行交互式Shell、Web界面操作和Web监控仪表板。

---

## 🏗️ 项目架构

```
数据库连接池_demo/
├── src/                          # 源代码目录
│   ├── auth/                     # 用户认证模块
│   │   ├── AuthHandler.h/cpp     # 认证处理器
│   │   ├── PasswordHash.h/cpp    # 密码哈希工具
│   │   ├── SessionManager.h/cpp  # 会话管理器
│   │   └── UserDBInit.h/cpp      # 用户数据库初始化
│   ├── interactive/              # 命令行交互模块
│   │   ├── Command.h/cpp         # 命令基类
│   │   ├── InteractiveShell.h/cpp # 交互式Shell
│   │   ├── MysqlCommandHandler.h/cpp # MySQL命令处理
│   │   └── RedisCommandHandler.h/cpp # Redis命令处理
│   ├── web/                      # Web服务模块
│   │   ├── HttpServer.h/cpp      # HTTP服务器
│   │   ├── MysqlApiHandler.h/cpp # MySQL API处理
│   │   ├── RedisApiHandler.h/cpp # Redis API处理
│   │   ├── index.html            # 主页面
│   │   ├── login.html            # 登录/注册页面
│   │   └── monitor.html          # 监控页面
│   ├── AutoReconnectPtr.h        # 自动重连智能指针
│   ├── ConnPool.h/hpp            # 连接池模板
│   ├── DBConnection.h            # 数据库连接管理
│   ├── MysqlConn.h/cpp           # MySQL连接封装
│   ├── RedisConn.h/cpp           # Redis连接封装
│   ├── Logger.h/cpp              # 日志记录器
│   ├── Timer.h/cpp               # 定时器
│   ├── main.cpp                  # 演示程序入口
│   ├── interactive_main.cpp      # 交互式程序入口
│   └── web_main.cpp              # Web服务入口
├── README.md                     # 项目说明
└── Makefile                      # 编译配置
```

---

## 🔧 核心功能模块

### 1. 连接池管理

| 功能 | 说明 |
|------|------|
| 动态连接管理 | 根据需求自动创建和回收连接 |
| 连接重用 | 复用空闲连接，减少连接开销 |
| 自动重连 | 连接断开后自动重新连接 |
| 连接超时 | 支持连接超时设置 |
| 线程安全 | 多线程环境下安全使用 |

### 2. 用户认证系统

| 功能 | 说明 |
|------|------|
| 用户注册 | 新用户注册，状态为pending |
| 用户登录 | 验证用户名和密码 |
| 会话管理 | 基于Cookie的会话管理 |
| 密码安全 | SHA-256 + 随机盐值加密 |
| 角色权限 | 超级管理员和普通用户角色 |

### 3. Web管理界面

| 功能 | 说明 |
|------|------|
| 数据表管理 | 创建、查看、编辑、删除表 |
| 数据操作 | 增删改查数据记录 |
| 用户管理 | 审批用户、修改密码、删除用户 |
| 连接池监控 | 实时查看连接池状态 |
| Redis操作 | Key-Value、Hash等操作 |

---

## 👥 用户角色与权限

### 角色定义

| 角色 | 说明 | 权限 |
|------|------|------|
| `super_admin` | 超级管理员 | 所有权限，包括用户管理 |
| `user` | 普通用户 | 数据操作，无用户管理权限 |

### 用户状态

| 状态 | 说明 |
|------|------|
| `pending` | 待审批，注册后默认状态 |
| `approved` | 已批准，可以正常登录 |
| `rejected` | 已拒绝，无法登录 |

### 默认超级管理员

```
用户名: admin
密码: admin123
```

---

## 🌐 API接口文档

### 认证相关API

| 接口 | 方法 | 说明 | 权限 |
|------|------|------|------|
| `/api/login` | POST | 用户登录 | 公开 |
| `/api/logout` | POST | 用户登出 | 登录 |
| `/api/register` | POST | 用户注册 | 公开 |
| `/api/current-user` | GET | 获取当前用户信息 | 登录 |

### 用户管理API

| 接口 | 方法 | 说明 | 权限 |
|------|------|------|------|
| `/api/users` | GET | 获取所有用户列表 | 超级管理员 |
| `/api/users/pending` | GET | 获取待审批用户 | 超级管理员 |
| `/api/users/approve` | POST | 审批用户 | 超级管理员 |
| `/api/users/reject` | POST | 拒绝用户 | 超级管理员 |
| `/api/users` | DELETE | 删除用户 | 超级管理员 |
| `/api/users/password` | PUT | 修改密码 | 登录用户 |

### MySQL操作API

| 接口 | 方法 | 说明 |
|------|------|------|
| `/api/mysql/tables` | GET | 获取所有表列表 |
| `/api/mysql/table` | GET | 获取表数据 |
| `/api/mysql/table/create` | POST | 创建新表 |
| `/api/mysql/table/drop` | DELETE | 删除表 |
| `/api/mysql/record` | POST | 插入记录 |
| `/api/mysql/record` | PUT | 更新记录 |
| `/api/mysql/record` | DELETE | 删除记录 |

### Redis操作API

| 接口 | 方法 | 说明 |
|------|------|------|
| `/api/redis/get` | GET | 获取Key值 |
| `/api/redis/set` | POST | 设置Key值 |
| `/api/redis/del` | DELETE | 删除Key |
| `/api/redis/hget` | GET | 获取Hash字段 |
| `/api/redis/hset` | POST | 设置Hash字段 |
| `/api/redis/hgetall` | GET | 获取所有Hash字段 |

---

## 🗄️ 数据库表结构

### sys_users 用户表

```sql
CREATE TABLE sys_users (
    id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(50) NOT NULL UNIQUE,
    password_hash VARCHAR(64) NOT NULL,
    salt VARCHAR(64) NOT NULL,
    role VARCHAR(20) DEFAULT 'user',
    status VARCHAR(20) DEFAULT 'pending',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

| 字段 | 类型 | 说明 |
|------|------|------|
| id | INT | 用户ID（主键） |
| username | VARCHAR(50) | 用户名（唯一） |
| password_hash | VARCHAR(64) | 密码哈希值 |
| salt | VARCHAR(64) | 密码盐值 |
| role | VARCHAR(20) | 角色（super_admin/user） |
| status | VARCHAR(20) | 状态（pending/approved/rejected） |
| created_at | TIMESTAMP | 创建时间 |

---

## 🔐 安全特性

### 密码安全

- **加密算法**: SHA-256
- **盐值**: 32位随机字符串
- **存储**: 仅存储哈希值和盐值，不存储明文密码

### 会话安全

- **会话ID**: 随机生成的唯一标识
- **Cookie属性**: HttpOnly, Path=/
- **会话超时**: 默认24小时

### 权限控制

- **前端控制**: 根据角色显示/隐藏功能模块
- **后端验证**: 每个API都验证用户权限
- **数据隔离**: sys_users表对普通用户不可见

---

## 🚀 编译与运行

### 编译命令

```bash
# 编译所有目标
make all

# 编译Web服务
make web

# 编译交互式程序
make interactive

# 清理编译文件
make clean
```

### 运行方式

```bash
# 运行Web服务
./web_server

# 运行交互式Shell
./interactive_shell

# 运行演示程序
./conn_pool_demo
```

### 访问地址

| 服务 | 地址 |
|------|------|
| 主页面 | http://172.22.136.134:8080/ |
| 登录页面 | http://172.22.136.134:8080/login.html |
| 监控页面 | http://172.22.136.134:8080/monitor |

---

## 📝 用户操作流程

### 用户注册流程

```
1. 访问登录页面 → 点击"立即注册"
2. 填写用户名和密码 → 点击"注册"
3. 注册成功，状态为pending
4. 等待超级管理员审批
5. 审批通过后，状态变为approved
6. 用户可以正常登录
```

### 超级管理员操作流程

```
1. 使用admin/admin123登录
2. 点击"👥 用户管理"标签
3. 点击"📋 待审批用户"查看待审批列表
4. 点击"✅ 审批"或"❌ 拒绝"处理用户申请
5. 点击"📊 所有用户"查看所有用户
6. 可以修改用户密码或删除用户
```

---

## 🔧 配置说明

### MySQL配置

```cpp
// 默认配置
host: "localhost"
port: 3306
user: "root"
password: "your_password"
database: "test_db"
```

### Redis配置

```cpp
// 默认配置
host: "localhost"
port: 6379
timeout: 5000  // 毫秒
```

### 连接池配置

```cpp
// 默认配置
initialSize: 5      // 初始连接数
maxSize: 20         // 最大连接数
minIdle: 2          // 最小空闲连接数
maxWaitTime: 5000   // 最大等待时间（毫秒）
```

---

## 📊 日志系统

### 日志类型

| 类型 | 说明 |
|------|------|
| MYSQL | MySQL操作日志 |
| REDIS | Redis操作日志 |
| AUTH | 认证相关日志 |
| SYSTEM | 系统运行日志 |

### 日志格式

```
[时间] [类型] [客户端IP] [操作] [SQL/命令] [成功/失败] [耗时ms]
```

---

## 🐛 已知问题与解决方案

### 问题1: 端口占用

**现象**: 启动失败，提示"绑定端口 8080 失败"

**解决方案**:
```bash
# 查找占用端口的进程
netstat -tlnp | grep 8080
# 终止进程
kill -9 <PID>
```

### 问题2: 数据库连接失败

**现象**: 提示"无法获取MySQL连接"

**解决方案**:
1. 检查MySQL服务是否运行
2. 检查用户名、密码、数据库名是否正确
3. 检查网络连接是否正常

### 问题3: 用户无法登录

**现象**: 提示"用户名或密码错误"

**可能原因**:
1. 用户状态不是approved
2. 密码输入错误
3. 用户不存在

---

## 🔄 更新日志

### v1.0.0 (当前版本)

- ✅ 实现MySQL/Redis连接池
- ✅ 实现命令行交互式Shell
- ✅ 实现Web管理界面
- ✅ 实现用户认证系统
- ✅ 实现角色权限控制
- ✅ 实现密码安全加密
- ✅ 实现用户注册审批流程
- ✅ 实现连接池监控
- ✅ 过滤sys_users表显示
- ✅ 支持超级管理员修改用户密码

---

## 📞 技术支持

如有问题，请检查以下内容：

1. 确认MySQL和Redis服务正常运行
2. 确认配置参数正确
3. 查看日志文件 `server.log`
4. 检查网络连接和防火墙设置
