
#!/bin/bash
# ==============================================================================
# 连接池组件依赖安装脚本
# 适用于 Ubuntu/Debian 系统（包括 WSL）
# ==============================================================================

set -e

echo "========================================"
echo "  连接池组件依赖安装脚本"
echo "========================================"

# 检测是否为 WSL 环境
is_wsl=false
if grep -qi microsoft /proc/version; then
    is_wsl=true
    echo "[检测] 当前环境：WSL"
else
    echo "[检测] 当前环境：原生 Linux"
fi

# 更新系统
echo ""
echo "[1/5] 更新系统软件包..."
sudo apt-get update -y

# 安装 MySQL 客户端库
echo ""
echo "[2/5] 安装 MySQL 客户端库..."
sudo apt-get install -y libmysqlclient-dev

# 安装 Redis 客户端库
echo ""
echo "[3/5] 安装 Redis 客户端库..."
sudo apt-get install -y libhiredis-dev

# 安装编译工具
echo ""
echo "[4/5] 安装编译工具..."
sudo apt-get install -y g++ make

# 安装 MySQL 和 Redis 服务器（用于测试）
echo ""
echo "[5/5] 安装 MySQL 和 Redis 服务器..."

# 安装 MySQL Server
echo "  安装 MySQL Server..."
sudo apt-get install -y mysql-server

# 安装 Redis Server
echo "  安装 Redis Server..."
sudo apt-get install -y redis-server

# 启动服务
echo "  启动服务..."

if [ "$is_wsl" = true ]; then
    # WSL 环境使用 service 命令启动
    echo "  WSL环境：使用 service 命令启动服务..."
    
    # 启动 MySQL
    sudo service mysql start
    
    # 启动 Redis
    sudo service redis-server start
else
    # 原生 Linux 使用 systemctl
    echo "  原生Linux环境：使用 systemctl 命令启动服务..."
    
    # 启动 MySQL
    sudo systemctl start mysql
    sudo systemctl enable mysql
    
    # 启动 Redis
    sudo systemctl start redis-server
    sudo systemctl enable redis-server
fi

# 等待服务启动
echo "  等待服务启动..."
sleep 3

# 创建测试数据库和用户
echo "  配置 MySQL 测试环境..."

# 使用 sudo 连接 MySQL（解决 auth_socket 认证问题）
sudo mysql -u root <<EOF
ALTER USER 'root'@'localhost' IDENTIFIED WITH mysql_native_password BY '123456';
CREATE DATABASE IF NOT EXISTS test;
GRANT ALL PRIVILEGES ON test.* TO 'root'@'localhost';
FLUSH PRIVILEGES;
EOF

echo ""
echo "========================================"
echo "  依赖安装完成！"
echo "========================================"
echo ""
echo "数据库配置信息："
echo "  MySQL: host=localhost, port=3306, user=root, password=123456, db=test"
echo "  Redis: host=localhost, port=6379"
echo ""
echo "接下来执行："
echo "  cd src"
echo "  make"
echo "  ./conn_pool_demo"
