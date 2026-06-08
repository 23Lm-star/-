#!/bin/bash

# 数据库连接池集群停止脚本

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "========================================"
echo "   数据库连接池 - 集群停止脚本"
echo "========================================"
echo ""

# 停止Nginx
echo -e "${YELLOW}停止Nginx负载均衡器...${NC}"
sudo pkill nginx 2>/dev/null || true
echo -e "${GREEN}✓${NC} Nginx已停止"

# 停止Web节点
echo ""
echo -e "${YELLOW}停止Web集群节点...${NC}"

for port in 8080 8081 8082; do
    pid=$(pgrep -f "web_server.*--port $port" 2>/dev/null || echo "")

    if [ -n "$pid" ]; then
        kill $pid 2>/dev/null || true
        sleep 1

        # 再次检查是否停止
        if pgrep -f "web_server.*--port $port" > /dev/null; then
            kill -9 $pid 2>/dev/null || true
            echo -e "${RED}✗${NC} 节点 (端口 $port) - 强制终止"
        else
            echo -e "${GREEN}✓${NC} 节点 (端口 $port) - 已停止"
        fi
    else
        echo -e "${YELLOW}○${NC} 节点 (端口 $port) - 未运行"
    fi
done

echo ""
echo "========================================"
echo -e "   ${GREEN}集群已完全停止${NC}"
echo "========================================"
