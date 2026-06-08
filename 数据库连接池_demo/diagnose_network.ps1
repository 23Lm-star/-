
# 诊断脚本 - 检查WSL到Linux虚拟机的网络连接
Write-Host "========================================" -ForegroundColor Green
Write-Host "  网络诊断工具" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""

# 测试Windows到Linux虚拟机的连接
Write-Host "[1] 测试Windows到Linux虚拟机 (192.168.232.155)" -ForegroundColor Cyan
try {
    $ping = Test-Connection -ComputerName 192.168.232.155 -Count 1 -Quiet
    Write-Host "    Ping: $(if ($ping) { 'OK' } else { 'FAIL' })" -ForegroundColor $(if ($ping) { 'Green' } else { 'Red' })
    
    $tcp = New-Object System.Net.Sockets.TcpClient
    $tcp.ReceiveTimeout = 3000
    $mysqlOk = $false
    $redisOk = $false
    
    try {
        $tcp.Connect("192.168.232.155", 3306)
        $mysqlOk = $tcp.Connected
        Write-Host "    MySQL:3306: OK" -ForegroundColor Green
    } catch {
        Write-Host "    MySQL:3306: FAIL - $_" -ForegroundColor Red
    } finally {
        $tcp.Close()
    }
    
    try {
        $tcp = New-Object System.Net.Sockets.TcpClient
        $tcp.Connect("192.168.232.155", 6379)
        $redisOk = $tcp.Connected
        Write-Host "    Redis:6379: OK" -ForegroundColor Green
    } catch {
        Write-Host "    Redis:6379: FAIL - $_" -ForegroundColor Red
    } finally {
        $tcp.Close()
    }
} catch {
    Write-Host "    测试失败: $_" -ForegroundColor Red
}

Write-Host ""
Write-Host "[2] 测试WSL到Linux虚拟机的连接" -ForegroundColor Cyan

# 通过WSL运行测试
wsl -d Ubuntu -- bash -c @"
echo '  [WSL] 测试连接到 192.168.232.155'
echo '  ---'

# 测试ping
ping -c 1 -W 1 192.168.232.155 2>/dev/null
if [ \$? -eq 0 ]; then
    echo '    Ping: OK'
else
    echo '    Ping: FAIL'
fi

# 测试MySQL端口
echo -n '    MySQL:3306: '
timeout 2 bash -c 'echo > /dev/tcp/192.168.232.155/3306' 2>/dev/null
if [ \$? -eq 0 ]; then
    echo 'OK'
else
    echo 'FAIL'
fi

# 测试Redis端口
echo -n '    Redis:6379: '
timeout 2 bash -c 'echo > /dev/tcp/192.168.232.155/6379' 2>/dev/null
if [ \$? -eq 0 ]; then
    echo 'OK'
else
    echo 'FAIL'
fi

echo '  ---'
echo '  WSL网络信息:'
ip route | head -1
"@

Write-Host ""
Write-Host "[3] 检查当前配置" -ForegroundColor Cyan
Write-Host "    DBConnection.h MySQL: 192.168.232.155:3306"
Write-Host "    DBConnection.h Redis: 192.168.232.155:6379"
Write-Host "    web_main.cpp Session: 192.168.232.155:6379"
Write-Host ""
Write-Host "========================================" -ForegroundColor Green
