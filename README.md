```markdown
# 数据库连接池管理系统

<p align="center">
  <img src="https://img.shields.io/badge/C++-11-blue.svg" alt="C++11">
  <img src="https://img.shields.io/badge/MySQL-Supported-green.svg" alt="MySQL">
  <img src="https://img.shields.io/badge/Redis-Supported-green.svg" alt="Redis">
  <img src="https://img.shields.io/badge/Web-Interface-orange.svg" alt="Web Interface">
  <img src="https://img.shields.io/badge/License-MIT-yellow.svg" alt="License">
</p>

> 基于C++11开发的RAII数据库连接池组件，支持MySQL和Redis，提供Web管理界面和命令行交互两种操作方式。

## ✨ 功能特性

### 核心功能
- 🔄 **连接池管理** - 动态连接创建与回收，自动重连机制
- 🔒 **线程安全** - 多线程环境下安全使用
- 📊 **实时监控** - 连接池状态实时监控
- 🎯 **RAII设计** - 资源自动管理，连接自动释放

### 用户认证
- 🔐 **安全密码存储** - SHA-256 + 随机盐值加密
- 👥 **角色权限管理** - 超级管理员和普通用户
- 📝 **用户审批流程** - 注册需要管理员审批

### 操作界面
- 🌐 **Web管理界面** - 数据表管理、用户管理
- 💻 **命令行交互** - 交互式Shell操作
- 📈 **监控仪表盘** - 连接池状态可视化

## 🚀 快速开始

### 环境要求
- C++11 或更高版本
- MySQL 5.7+
- Redis 3.0+
- Linux/macOS/WSL (Windows)

### 编译项目

```bash
# 克隆项目
git clone <repository-url>
cd 数据库连接池_demo/src

# 编译所有目标
make all

# 或单独编译
make web      # Web服务
make interactive  # 交互式程序
```

### 运行服务

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

### 默认账号

```
用户名: admin
密码: admin123
```

## 📖 使用指南

### 用户注册流程

```
1. 访问登录页面 → 点击"立即注册"
2. 填写用户名和密码 → 点击"注册"
3. 注册成功，状态为"待审批"
4. 等待超级管理员审批
5. 审批通过后正常登录
```

### Web界面功能

#### MySQL管理
- 📋 查看所有数据表
- ➕ 创建新表
- ✏️ 编辑表结构
- 📝 增删改查数据

#### Redis管理
- 🔑 Key-Value操作
- 📊 Hash操作
- 📈 List操作

#### 用户管理（超级管理员）
- 👥 查看所有用户
- ✅ 审批新用户
- 🔐 修改用户密码
- 🗑️ 删除用户

## 🏗️ 项目架构

```
数据库连接池_demo/
├── src/
│   ├── auth/                 # 用户认证模块
│   │   ├── AuthHandler      # 认证处理器
│   │   ├── PasswordHash     # 密码加密
│   │   ├── SessionManager   # 会话管理
│   │   └── UserDBInit       # 用户数据库
│   ├── interactive/          # 命令行交互
│   │   ├── Command          # 命令基类
│   │   ├── InteractiveShell # 交互式Shell
│   │   ├── MysqlCommandHandler
│   │   └── RedisCommandHandler
│   ├── web/                 # Web服务
│   │   ├── HttpServer       # HTTP服务器
│   │   ├── MysqlApiHandler  # MySQL API
│   │   └── RedisApiHandler  # Redis API
│   ├── ConnPool.hpp         # 连接池模板
│   ├── MysqlConn           # MySQL连接
│   └── RedisConn           # Redis连接
└── Makefile
```

## 🔌 API接口

### 认证接口
| 接口 | 方法 | 说明 |
|------|------|------|
| `/api/login` | POST | 用户登录 |
| `/api/register` | POST | 用户注册 |
| `/api/logout` | POST | 用户登出 |

### MySQL接口
| 接口 | 方法 | 说明 |
|------|------|------|
| `/api/mysql/tables` | GET | 获取表列表 |
| `/api/mysql/table` | GET | 获取表数据 |
| `/api/mysql/table/create` | POST | 创建表 |
| `/api/mysql/record` | POST/PUT/DELETE | 增删改数据 |

### Redis接口
| 接口 | 方法 | 说明 |
|------|------|------|
| `/api/redis/get` | GET | 获取值 |
| `/api/redis/set` | POST | 设置值 |
| `/api/redis/del` | DELETE | 删除键 |

### 用户管理接口（管理员）
| 接口 | 方法 | 说明 |
|------|------|------|
| `/api/users` | GET | 获取用户列表 |
| `/api/users/pending` | GET | 待审批用户 |
| `/api/users/approve` | POST | 审批用户 |

## 🔐 安全机制

### 密码安全
- 使用SHA-256哈希算法
- 每个用户独立随机盐值
- 不存储明文密码

### 会话管理
- HttpOnly Cookie
- 24小时会话超时
- 安全的会话ID生成

### 权限控制
- 前端：基于角色的UI显示控制
- 后端：每个API的权限验证
- 数据：敏感表对普通用户隐藏

## 📊 数据库表结构

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

## 🛠️ 配置说明

### MySQL配置
```cpp
host: "localhost"
port: 3306
user: "root"
password: "your_password"
database: "test_db"
```

### Redis配置
```cpp
host: "localhost"
port: 6379
timeout: 5000
```

### 连接池配置
```cpp
initialSize: 5   // 初始连接数
maxSize: 20      // 最大连接数
minIdle: 2       // 最小空闲连接
maxWaitTime: 5000 // 最大等待时间(ms)
```

## 🐛 常见问题

### Q: 端口8080被占用？
```bash
# Linux/macOS
lsof -i :8080
kill -9 <PID>

# Windows
netstat -ano | findstr :8080
taskkill /F /PID <PID>
```

### Q: 数据库连接失败？
1. 检查MySQL/Redis服务是否运行
2. 验证用户名、密码、数据库名
3. 检查网络连接

### Q: 用户无法登录？
1. 确认用户状态为 `approved`
2. 检查密码是否正确
3. 确认用户已注册

## 📝 更新日志

### v1.0.0
- ✅ MySQL/Redis连接池实现
- ✅ Web管理界面
- ✅ 命令行交互Shell
- ✅ 用户认证系统
- ✅ 角色权限控制
- ✅ 用户审批流程
- ✅ 连接池监控

## 📄 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

## 📧 联系方式

如有问题，请提交 Issue 或联系开发者。
```
