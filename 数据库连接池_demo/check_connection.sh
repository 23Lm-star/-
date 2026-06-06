#!/bin/bash
# ============================================
# 数据库连接检查脚本
# 用于诊断MySQL和Redis连接问题
# 使用方法: bash check_connection.sh
# ============================================

echo "============================================="
echo "    数据库连接诊断工具"
echo "============================================="
echo ""

# 1. 检查Docker容器状态
echo "[步骤1] 检查Docker容器状态"
echo "-------------------------"
docker ps | grep -E "mysql|redis"
echo ""

# 2. 检查Docker端口映射
echo "[步骤2] 检查Docker端口映射"
echo "-------------------------"
echo "MySQL端口:"
docker port mysql_container 2>/dev/null || echo "  ❌ MySQL容器未运行"
echo ""
echo "Redis端口:"
docker port redis_container 2>/dev/null || echo "  ❌ Redis容器未运行"
echo ""

# 3. 获取虚拟机IP
echo "[步骤3] 获取当前网络配置"
echo "-------------------------"
echo "Linux虚拟机IP地址:"
ip addr show | grep "inet " | grep -v "127.0.0.1" | awk '{print "  " $2}'
echo ""

# 4. 测试本地连接
echo "[步骤4] 测试本地数据库连接"
echo "-------------------------"

# 测试MySQL本地连接
echo "MySQL本地连接测试:"
if docker exec -it mysql_container mysql -uroot -p123456 -e "SELECT 1;" 2>/dev/null; then
    echo "  ✅ MySQL本地连接成功"
else
    echo "  ❌ MySQL本地连接失败"
fi
echo ""

# 测试Redis本地连接
echo "Redis本地连接测试:"
if docker exec -it redis_container redis-cli ping 2>/dev/null | grep -q "PONG"; then
    echo "  ✅ Redis本地连接成功"
else
    echo "  ❌ Redis本地连接失败"
fi
echo ""

# 5. 检查WSL网络配置（如果在WSL中运行）
echo "[步骤5] 检查WSL网络配置"
echo "-------------------------"
echo "WSL IP地址:"
hostname -I
echo ""
echo "默认网关:"
ip route show | grep default | awk '{print "  " $3}'
echo ""

# 6. 测试远程连接（需要用户输入虚拟机IP）
read -p "请输入Linux虚拟机的IP地址（用于测试WSL到Docker的连接）: " VM_IP

if [ -n "$VM_IP" ]; then
    echo ""
    echo "[步骤6] 测试WSL到虚拟机的网络连通性"
    echo "-------------------------------------"
    
    # 测试MySQL端口
    echo "测试MySQL端口 $VM_IP:3306"
    timeout 3 nc -zv "$VM_IP" 3306 2>&1 || echo "  ❌ MySQL端口无法访问"
    
    # 测试Redis端口
    echo "测试Redis端口 $VM_IP:6379"
    timeout 3 nc -zv "$VM_IP" 6379 2>&1 || echo "  ❌ Redis端口无法访问"
    
    echo ""
    echo "============================================="
    echo "连接配置建议"
    echo "============================================="
    echo "请更新 src/DBConnection.h 文件中的IP地址:"
    echo ""
    echo "MySQL: $VM_IP:3306"
    echo "Redis: $VM_IP:6379"
    echo ""
    echo "修改后重新编译:"
    echo "  cd src && touch DBConnection.h && make web"
fi

echo ""
echo "============================================="
echo "诊断完成!"
echo "============================================="
