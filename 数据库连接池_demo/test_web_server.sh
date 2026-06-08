#!/bin/bash

cd /mnt/d/Trae\ CN/trae_demo/数据库连接池_demo/src

echo "=== 测试Web服务器启动 ==="
echo "时间: $(date)"
echo ""

# 测试直接运行
echo "1. 直接运行web_server:"
echo "----------------------------------------"
./web_server --port 8080 > /tmp/web_server_test.log 2>&1 &
pid=$!
sleep 3

# 检查进程是否仍在运行
if ps -p $pid > /dev/null; then
    echo "✓ Web服务器启动成功 (PID: $pid)"
    kill $pid
else
    echo "✗ Web服务器启动失败"
    echo "日志内容:"
    cat /tmp/web_server_test.log
fi

echo ""
echo "=== 测试完成 ==="
