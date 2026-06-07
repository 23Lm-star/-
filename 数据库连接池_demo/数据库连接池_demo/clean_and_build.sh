#!/bin/bash

cd /mnt/d/Trae\ CN/trae_demo/数据库连接池_demo/src

echo "=== 清理旧的编译文件 ==="
rm -f *.o conn_pool_demo

echo "=== 开始编译 ==="
make clean
make

echo "=== 编译完成 ==="