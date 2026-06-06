
@echo off
REM ==============================================================================
REM Windows运行脚本
REM ==============================================================================

echo ========================================
echo   RAII连接池组件测试
echo ========================================
echo.

REM 检查可执行文件是否存在
if not exist "src\conn_pool_demo.exe" (
    echo [1/2] 项目未编译，开始编译...
    call build.bat
    if %errorlevel% neq 0 (
        echo.
        echo [错误] 编译失败，无法运行！
        pause
        exit /b 1
    )
    echo.
) else (
    echo [1/2] 项目已编译，跳过编译步骤
    echo.
)

echo [2/2] 运行测试...
echo.
cd src
conn_pool_demo.exe
cd ..

echo.
echo ========================================
echo   测试完成！
echo ========================================
echo.
pause
