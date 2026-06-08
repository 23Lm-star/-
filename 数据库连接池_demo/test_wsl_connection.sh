#!/bin/bash

# 测试WSL到Linux虚拟机的连接
VM_IP="192.168.232.157"

echo "========================================"
echo "  WSL到Linux虚拟机连接测试"
echo "========================================"
echo ""

echo "[1] WSL网络配置:"
ip addr show | grep -E "inet|eth"
echo ""

echo "[2] 测试Ping $VM_IP"
ping -c 2 -W 1 $VM_IP
echo ""

echo "[3] 测试MySQL端口 $VM_IP:3306"
nc -zv -w 2 $VM_IP 3306
echo ""

echo "[4] 测试Redis端口 $VM_IP:6379"
nc -zv -w 2 $VM_IP 6379
echo ""

echo "[5] 测试HTTP连接到Web服务器"
if nc -zv -w 2 localhost 8080; then
    echo "✓ localhost:8080 可访问"
else
    echo "✗ localhost:8080 不可访问"
fi
echo ""

echo "========================================"
