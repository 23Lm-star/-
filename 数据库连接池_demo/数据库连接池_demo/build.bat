
@echo off
REM ==============================================================================
REM Windows编译脚本
REM ==============================================================================

echo ========================================
echo   RAII连接池组件 - Windows编译脚本
echo ========================================
echo.

REM 检查是否安装了make或g++
where make >nul 2>&1
if %errorlevel% equ 0 (
    echo [方式一] 检测到 make 工具，使用 Makefile 编译...
    cd src
    make clean
    make
    if %errorlevel% equ 0 (
        echo.
        echo [成功] 编译成功！
        echo.
        echo 可执行文件位置: src/conn_pool_demo.exe
        echo.
    ) else (
        echo.
        echo [错误] 使用 Makefile 编译失败，尝试直接编译...
        goto :compile_with_gcc
    )
    cd ..
    goto :end
)

:compile_with_gcc
REM 检查是否安装了g++
where g++ >nul 2>&1
if %errorlevel% neq 0 (
    echo [错误] 未找到编译器！
    echo 请先安装 MinGW 或使用 Visual Studio + CMake
    echo.
    echo 参考安装方式：
    echo 1. 下载并安装 MinGW-w64
    echo 2. 或者使用 Visual Studio 2019/2022 + CMake
    echo.
    pause
    exit /b 1
)

echo [方式二] 使用 g++ 直接编译...
echo.
echo [1/3] 编译源文件...
cd src

REM 使用g++编译
g++ -std=c++11 -Wall -Wextra -I. -c MysqlConn.cpp -o MysqlConn.o
if %errorlevel% neq 0 goto :error
g++ -std=c++11 -Wall -Wextra -I. -c RedisConn.cpp -o RedisConn.o
if %errorlevel% neq 0 goto :error
g++ -std=c++11 -Wall -Wextra -I. -c ConnPool.cpp -o ConnPool.o
if %errorlevel% neq 0 goto :error
g++ -std=c++11 -Wall -Wextra -I. -c Timer.cpp -o Timer.o
if %errorlevel% neq 0 goto :error
g++ -std=c++11 -Wall -Wextra -I. -c main.cpp -o main.o
if %errorlevel% neq 0 goto :error

echo.
echo [2/3] 链接生成可执行文件...
g++ -o conn_pool_demo.exe MysqlConn.o RedisConn.o ConnPool.o Timer.o main.o -lmysqlclient -lhiredis -lws2_32

if %errorlevel% equ 0 (
    echo.
    echo [3/3] 编译成功！
    echo.
    echo 可执行文件位置: src/conn_pool_demo.exe
    echo.
    echo 运行命令:
    echo   cd src
    echo   conn_pool_demo.exe
    echo.
) else (
    goto :error
)

cd ..
goto :end

:error
echo.
echo [错误] 编译失败！
echo.
echo 可能的原因：
echo 1. 未安装 MySQL C API (libmysqlclient)
echo 2. 未安装 hiredis 库
echo 3. 库文件路径配置不正确
echo.
echo 请参考 README_Windows.md 中的依赖安装说明
echo.
echo 解决方法提示：
echo - 确保已安装 MySQL Server 并包含 C API
echo - 确保已编译并配置好 hiredis 库
echo - 可以尝试修改 build.bat 中的编译命令，添加 -I 和 -L 参数指定路径
echo.
cd ..
goto :end

:end
pause
