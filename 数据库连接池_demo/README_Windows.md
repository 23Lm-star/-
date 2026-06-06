
# Windows 环境部署指南

本指南将帮助您在 Windows 系统上成功编译和运行 RAII 连接池组件。

## 目录

1. [环境准备](#环境准备)
2. [依赖安装](#依赖安装)
3. [编译运行](#编译运行)
4. [常见问题](#常见问题)

---

## 环境准备

### 方案一：使用 MinGW（推荐，简单）

1. **下载并安装 MinGW-w64**
   - 访问：https://www.mingw-w64.org/
   - 或者使用 MSYS2：https://www.msys2.org/

2. **安装 MinGW-w64**
   - 下载安装包并运行
   - 确保选择 "posix" 线程模型
   - 安装后将 MinGW 的 bin 目录添加到 PATH 环境变量

3. **验证安装**
   ```cmd
   g++ --version
   ```

### 方案二：使用 Visual Studio + CMake

1. **安装 Visual Studio 2019/2022**
   - 下载地址：https://visualstudio.microsoft.com/
   - 安装时选择 "使用 C++ 的桌面开发" 工作负载

2. **安装 CMake**
   - 下载地址：https://cmake.org/download/
   - 安装时选择 "Add CMake to the system PATH"

---

## 依赖安装

### 1. MySQL C API (libmysqlclient)

**方式一：使用 MySQL Installer（推荐）**

1. 下载 MySQL Installer：https://dev.mysql.com/downloads/installer/
2. 运行安装程序，选择 "Developer Default"
3. 安装 MySQL Server 和 MySQL C API
4. 默认安装路径（根据版本可能有所不同）：
   - 头文件：`C:\Program Files\MySQL\MySQL Server 8.0\include`
   - 库文件：`C:\Program Files\MySQL\MySQL Server 8.0\lib`

**方式二：手动安装**

1. 下载 MySQL Connector/C：https://dev.mysql.com/downloads/connector/c/
2. 解压到某个目录，例如：`C:\Program Files\MySQL`
3. 将 lib 目录添加到 PATH 环境变量

### 2. hiredis (Redis C 客户端库)

**方式一：使用预编译库**

1. 下载预编译的 hiredis 库：
   - GitHub 搜索 "hiredis windows precompiled"
   - 或者从以下地址下载：
     - 头文件：https://github.com/redis/hiredis/releases
     - 预编译库需要自行编译或寻找社区构建版本

2. 解压到 `third_party/hiredis` 目录（项目根目录下）

**方式二：从源代码编译**

1. 克隆或下载 hiredis 源码：https://github.com/redis/hiredis
2. 使用 CMake 编译：
   ```cmd
   cd hiredis
   mkdir build
   cd build
   cmake ..
   cmake --build . --config Release
   ```
3. 将编译好的文件复制到项目的 `third_party/hiredis` 目录

### 3. 配置库文件路径

确保以下文件路径在编译时可访问：

| 库 | 头文件位置 | 库文件位置 |
|:---|:---|:---|
| MySQL | `C:\Program Files\MySQL\MySQL Server 8.0\include` | `C:\Program Files\MySQL\MySQL Server 8.0\lib\mysqlclient.lib` |
| hiredis | `third_party/hiredis/include` | `third_party/hiredis/lib/hiredis.lib` |

---

## 编译运行

### 方式一：使用 build.bat（推荐，简单）

```cmd
# 直接运行编译脚本
build.bat
```

然后运行测试：
```cmd
# 运行测试
run.bat
```

### 方式二：使用 CMake

```cmd
# 创建 build 目录
mkdir build
cd build

# 配置 CMake
cmake ..

# 编译
cmake --build . --config Release

# 运行
Release\conn_pool_demo.exe
```

---

## 数据库服务准备

### 1. MySQL 数据库配置

确保 MySQL 服务已启动并正确配置：

| 参数 | 配置值 |
|:---|:---|
| host | localhost |
| port | 3306 |
| user | root |
| password | 123456 |
| database | test |

如果需要修改配置，请编辑 `src/MysqlConn.h` 中的默认值。

创建测试数据库：
```sql
CREATE DATABASE IF NOT EXISTS test;
```

### 2. Redis 服务

确保 Redis 服务已启动并运行在 localhost:6379。

**Windows 安装 Redis 方式：**

1. 下载 Redis for Windows：https://github.com/microsoftarchive/redis/releases
2. 或者使用 WSL2 中的 Redis
3. 或者使用 Docker：`docker run -p 6379:6379 redis`

---

## 常见问题

### Q1: 编译时提示找不到 libmysqlclient

**A:** 确保已安装 MySQL C API，并检查以下内容：
1. MySQL 的 include 目录是否在编译器搜索路径中
2. MySQL 的 lib 目录是否在链接器搜索路径中
3. mysqlclient.lib 文件是否存在

可以尝试在编译命令中指定路径：
```cmd
g++ -std=c++11 -Wall -I"C:\Program Files\MySQL\MySQL Server 8.0\include" -c *.cpp
g++ -o conn_pool_demo *.o -L"C:\Program Files\MySQL\MySQL Server 8.0\lib" -lmysqlclient ...
```

### Q2: 编译时提示找不到 hiredis

**A:** 确保 hiredis 库已正确安装：
1. 检查 hiredis 头文件是否在正确位置
2. 检查 hiredis 库文件是否可访问
3. 如果没有预编译库，需要从源代码编译 hiredis

### Q3: 运行时提示找不到 mysqlclient.dll

**A:** 将 mysqlclient.dll 复制到可执行文件目录，或添加到 PATH 环境变量：
- 从 MySQL 安装目录的 lib 文件夹中找到 mysqlclient.dll
- 复制到 src 目录，或添加到系统 PATH

### Q4: 连接 MySQL 失败

**A:** 检查以下几点：
1. MySQL 服务是否正在运行
2. 用户名密码是否正确
3. test 数据库是否存在
4. 防火墙是否阻止了 3306 端口

### Q5: 连接 Redis 失败

**A:** 检查以下几点：
1. Redis 服务是否正在运行
2. Redis 是否监听在 6379 端口
3. 防火墙是否阻止了 6379 端口

---

## 完整的快速开始步骤

1. **安装 MinGW-w64 或 Visual Studio**
2. **安装 MySQL Server（并创建 test 数据库）**
3. **安装 MySQL C API (libmysqlclient)**
4. **编译并安装 hiredis 库**
5. **运行 build.bat 编译项目**
6. **运行 run.bat 启动测试**

---

## 技术支持

如果遇到其他问题，请查看：
- MySQL 官方文档：https://dev.mysql.com/doc/
- hiredis GitHub：https://github.com/redis/hiredis
- 项目 README.md：基础说明和使用示例
