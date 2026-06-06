
# Windows 快速开始指南

## 🏃‍♂️ 最快的方式：使用 WSL2 (推荐)

如果安装了 WSL2，可以直接使用 Linux 环境，非常简单：

```cmd
# 打开 WSL
wsl

# 进入项目目录
cd /mnt/d/Trae\ CN/trae_demo/数据库连接池_demo

# 运行 Linux 安装脚本
chmod +x install_deps.sh
./install_deps.sh

# 编译运行
./run.sh
```

---

## 🖥️ 本地 Windows 环境快速开始

### 步骤 1: 安装编译器

**使用 MinGW (推荐最简单):**
- 下载地址：https://github.com/niXman/mingw-builds-binaries/releases
- 选择 x86_64-posix-seh 版本
- 解压后添加到 PATH 环境变量

**验证安装:**
```cmd
g++ --version
make --version  # 可选，但推荐
```

### 步骤 2: 安装 MySQL

1. 下载 MySQL Installer：https://dev.mysql.com/downloads/installer/
2. 安装时选择 "Developer Default"（会包含 C API）
3. 安装后创建 test 数据库
4. 记住 root 密码（设置为 123456）

```sql
-- 在 MySQL 命令行中执行
CREATE DATABASE IF NOT EXISTS test;
```

### 步骤 3: 安装 Redis (可选)

**方式一：使用 Docker (最简单)**
```cmd
docker run -p 6379:6379 redis
```

**方式二：下载 Windows 版本**
- 从：https://github.com/microsoftarchive/redis/releases 下载
- 解压后运行 redis-server.exe

### 步骤 4: 编译项目

```cmd
# 双击运行或命令行执行
build.bat
```

### 步骤 5: 运行测试

```cmd
# 双击运行或命令行执行
run.bat
```

---

## 📦 预编译库快速获取（不想自己编译）

### MySQL C API
- 直接安装 MySQL Server，安装程序会自动包含
- 或者单独下载 MySQL Connector/C：https://dev.mysql.com/downloads/connector/c/

### hiredis (Redis 库)
最简单的方式：
1. 使用 vcpkg 包管理器：
   ```cmd
   vcpkg install hiredis:x64-windows
   ```
2. 或者从 vcpkg 获取预编译版本

---

## ❓ 遇到问题？

### 问题1: 找不到 mysqlclient
解决：在 build.bat 的编译命令中手动指定路径：
```cmd
g++ -std=c++11 -Wall -I"C:\Program Files\MySQL\MySQL Server 8.0\include" -c MysqlConn.cpp ...
g++ -o conn_pool_demo.exe ... -L"C:\Program Files\MySQL\MySQL Server 8.0\lib" -lmysqlclient ...
```

### 问题2: 不想用 MySQL/Redis
如果不想配置这些，可以修改 main.cpp 只测试连接池逻辑：
- 注释掉 MySQL 或 Redis 的测试部分
- 但这样可能需要修改更多代码

### 问题3: 简单测试版本
可以要求我创建一个不依赖 MySQL/Redis 的简化版，只展示连接池和 RAII 机制的代码。

---

## 💡 总结

Windows 环境最推荐的使用顺序：
1. **首选**：使用 WSL2 运行 Linux 版本
2. **次选**：使用 MinGW + Docker 运行 MySQL/Redis
3. **或者**：使用 Visual Studio + CMake

如需帮助，请参考 `README_Windows.md` 或告诉我具体遇到的问题！
