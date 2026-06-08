# 测试Windows到Linux虚拟机的连接
$VM_IP = "192.168.232.157"

Write-Host "========================================" -ForegroundColor Green
Write-Host "  Windows到Linux虚拟机连接测试" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""

# 测试Ping
Write-Host "[1] 测试Ping $VM_IP" -ForegroundColor Cyan
try {
    $ping = Test-Connection -ComputerName $VM_IP -Count 2 -Quiet -ErrorAction Stop
    Write-Host "Ping: $(if ($ping) { 'OK' } else { 'FAIL' })" -ForegroundColor $(if ($ping) { 'Green' } else { 'Red' })
} catch {
    Write-Host "Ping: FAIL - $_" -ForegroundColor Red
}
Write-Host ""

# 测试MySQL端口
Write-Host "[2] 测试MySQL端口 $VM_IP`:3306" -ForegroundColor Cyan
try {
    $tcp = New-Object System.Net.Sockets.TcpClient
    $tcp.Connect($VM_IP, 3306)
    if ($tcp.Connected) {
        Write-Host "MySQL:3306 - OK" -ForegroundColor Green
    } else {
        Write-Host "MySQL:3306 - FAIL" -ForegroundColor Red
    }
    $tcp.Close()
} catch {
    Write-Host "MySQL:3306 - FAIL - $_" -ForegroundColor Red
}
Write-Host ""

# 测试Redis端口
Write-Host "[3] 测试Redis端口 $VM_IP`:6379" -ForegroundColor Cyan
try {
    $tcp = New-Object System.Net.Sockets.TcpClient
    $tcp.Connect($VM_IP, 6379)
    if ($tcp.Connected) {
        Write-Host "Redis:6379 - OK" -ForegroundColor Green
    } else {
        Write-Host "Redis:6379 - FAIL" -ForegroundColor Red
    }
    $tcp.Close()
} catch {
    Write-Host "Redis:6379 - FAIL - $_" -ForegroundColor Red
}
Write-Host ""

Write-Host "========================================" -ForegroundColor Green
