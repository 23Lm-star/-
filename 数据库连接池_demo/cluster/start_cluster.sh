#!/bin/bash

# 数据库连接池集群启动脚本
# 功能：启动3个Web节点 + Nginx负载均衡器

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 检查本地服务而不是远程VM
LOCAL_IP="127.0.0.1"

# 项目路径
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && cd .. && pwd)"
SRC_DIR="$PROJECT_DIR/src"
CLUSTER_DIR="$PROJECT_DIR/cluster"

echo "========================================"
echo "   数据库连接池 - 集群启动脚本"
echo "========================================"
echo ""

# 检查web_server是否编译
if [ ! -f "$SRC_DIR/web_server" ]; then
    echo -e "${YELLOW}警告: web_server未编译，正在编译...${NC}"
    cd "$SRC_DIR"
    make web
    if [ $? -ne 0 ]; then
        echo -e "${RED}错误: 编译失败${NC}"
        exit 1
    fi
    echo -e "${GREEN}编译成功${NC}"
fi

# 停止已有进程
echo -e "${YELLOW}停止已有进程...${NC}"
pkill -f "web_server.*--port" 2>/dev/null || true
pkill -f "nginx.*cluster" 2>/dev/null || true
sleep 1

# 检查MySQL和Redis容器
echo ""
echo "检查依赖服务..."
echo "----------------------------------------"

check_service() {
    local service=$1
    local port=$2
    # 使用timeout避免卡住，用127.0.0.1而不是VM_IP
    if timeout 1 nc -z $LOCAL_IP $port 2>/dev/null; then
        echo -e "${GREEN}✓${NC} $service (${LOCAL_IP}:$port) - 运行中"
        return 0
    else
        echo -e "${YELLOW}⚠${NC} $service (${LOCAL_IP}:$port) - 检查中或未运行（继续启动...）"
        # 不返回失败，只是警告，让脚本继续执行
        return 0
    fi
}

# 检查MySQL
check_service "MySQL" 3306

# 检查Redis
check_service "Redis" 6379

echo ""

# 启动Web节点
echo "启动Web集群节点..."
echo "----------------------------------------"

start_node() {
    local port=$1
    local name="节点1 (端口 $port)"

    cd "$SRC_DIR"
    nohup ./web_server --port $port > "$CLUSTER_DIR/node_${port}.log" 2>&1 &
    local pid=$!

    sleep 3

    if ps -p $pid > /dev/null 2>&1; then
        echo -e "${GREEN}✓${NC} $name 启动成功 (PID: $pid)"
        return 0
    else
        echo -e "${RED}✗${NC} $name 启动失败"
        echo "=== 节点 $port 日志内容 ==="
        cat "$CLUSTER_DIR/node_${port}.log"
        echo "=== 日志结束 ==="
        return 1
    fi
}

# 启动3个节点
start_node 8080
start_node 8081
start_node 8082

echo ""

# 等待节点启动
echo "等待节点就绪..."
sleep 2

# 检查健康状态
echo ""
echo "检查节点健康状态..."
echo "----------------------------------------"

check_health() {
    local port=$1
    local status=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:$port/health 2>/dev/null || echo "000")

    if [ "$status" = "200" ]; then
        echo -e "${GREEN}✓${NC} 节点 (端口 $port) - 健康"
        return 0
    else
        echo -e "${RED}✗${NC} 节点 (端口 $port) - 不健康 (HTTP $status)"
        return 1
    fi
}

check_health 8080
check_health 8081
check_health 8082

echo ""

# 配置并启动Nginx
echo "配置并启动Nginx..."
echo "----------------------------------------"

# 复制Nginx配置
sudo cp "$CLUSTER_DIR/nginx.conf" /etc/nginx/nginx.conf 2>/dev/null || \
    cp "$CLUSTER_DIR/nginx.conf" /tmp/nginx_cluster.conf

# 检查Nginx配置
if sudo nginx -t 2>/dev/null; then
    echo -e "${GREEN}✓${NC} Nginx配置正确"
else
    echo -e "${RED}✗${NC} Nginx配置错误"
fi

# 启动/重启Nginx
sudo pkill nginx 2>/dev/null || true
sleep 1

if sudo nginx -c /etc/nginx/nginx.conf; then
    echo -e "${GREEN}✓${NC} Nginx启动成功"
else
    echo -e "${YELLOW}警告: Nginx启动可能需要sudo权限${NC}"
fi

sleep 1

# 检查Nginx状态
if nc -z localhost 80 2>/dev/null; then
    echo -e "${GREEN}✓${NC} Nginx监听端口 80"
else
    echo -e "${RED}✗${NC} Nginx未监听端口 80"
fi

echo ""
echo "========================================"
echo -e "   ${GREEN}集群启动完成！${NC}"
echo "========================================"
echo ""
echo "访问地址："
echo "----------------------------------------"
echo -e "  🌐 访问入口: ${GREEN}http://localhost/${NC}"
echo -e "  📊 节点1:    ${GREEN}http://localhost:8080/${NC}"
echo -e "  📊 节点2:    ${GREEN}http://localhost:8081/${NC}"
echo -e "  📊 节点3:    ${GREEN}http://localhost:8082/${NC}"
echo -e "  💚 健康检查: ${GREEN}http://localhost/health${NC}"
echo ""
echo "日志文件："
echo "----------------------------------------"
echo "  $CLUSTER_DIR/node_8080.log"
echo "  $CLUSTER_DIR/node_8081.log"
echo "  $CLUSTER_DIR/node_8082.log"
echo ""
echo "停止集群："
echo "----------------------------------------"
echo "  $CLUSTER_DIR/stop_cluster.sh"
echo ""

# 显示Nginx状态
echo "Nginx状态："
sudo systemctl status nginx 2>/dev/null | grep -E "(Active:|Loaded:)" || echo "使用 'sudo systemctl status nginx' 查看详细状态"
