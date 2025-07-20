# Simple Arduino Test Script - No Crash Version
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Simple Arduino CLI Test" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# Step 1: Check if arduino-cli exists
Write-Host "`n1. Checking Arduino CLI..." -ForegroundColor Yellow
$arduinoCliPath = Join-Path $PSScriptRoot "arduino-cli.exe"
if (Test-Path $arduinoCliPath) {
    Write-Host "✅ Arduino CLI found at: $arduinoCliPath" -ForegroundColor Green
} else {
    Write-Host "❌ Arduino CLI not found at: $arduinoCliPath" -ForegroundColor Red
    Write-Host "Press any key to exit..."
    $null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
    exit 1
}

# Step 2: Test Arduino CLI version
Write-Host "`n2. Testing Arduino CLI version..." -ForegroundColor Yellow
try {
    $versionResult = & $arduinoCliPath version 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ Arduino CLI version: $versionResult" -ForegroundColor Green
    } else {
        Write-Host "❌ Arduino CLI version failed: $versionResult" -ForegroundColor Red
    }
} catch {
    Write-Host "❌ Error running Arduino CLI: $_" -ForegroundColor Red
}

# Step 3: List available cores
Write-Host "`n3. Listing available cores..." -ForegroundColor Yellow
try {
    $coreResult = & $arduinoCliPath core list 2>&1
    Write-Host "Cores found:" -ForegroundColor Green
    Write-Host $coreResult -ForegroundColor White
} catch {
    Write-Host "❌ Error listing cores: $_" -ForegroundColor Red
}

# Step 4: List available boards
Write-Host "`n4. Listing available boards..." -ForegroundColor Yellow
try {
    $boardResult = & $arduinoCliPath board list 2>&1
    Write-Host "Boards found:" -ForegroundColor Green
    Write-Host $boardResult -ForegroundColor White
} catch {
    Write-Host "❌ Error listing boards: $_" -ForegroundColor Red
}

# Step 5: Check COM ports
Write-Host "`n5. Checking COM ports..." -ForegroundColor Yellow
try {
    $ports = Get-WmiObject -Class Win32_SerialPort -ErrorAction SilentlyContinue
    if ($ports) {
        Write-Host "Available COM ports:" -ForegroundColor Green
        $ports | ForEach-Object {
            Write-Host "  $($_.DeviceID) - $($_.Description)" -ForegroundColor White
        }
    } else {
        Write-Host "❌ No COM ports found" -ForegroundColor Red
    }
} catch {
    Write-Host "❌ Error checking COM ports: $_" -ForegroundColor Red
}

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Test completed!" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Press any key to exit..."
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown") 