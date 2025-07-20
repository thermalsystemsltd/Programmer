# Arduino Upload Test Script
param(
    [string]$SketchPath = "C:\Users\adamp\Documents\Arduino\LoraTX NEW PCB 12-F VERSION\SENSOR V105 PCB INA226\SENSOR",
    [string]$ComPort = ""
)

# Function to detect ESP8266 COM port
function Get-ESP8266ComPort {
    try {
        $ports = Get-WmiObject -Class Win32_SerialPort | Where-Object { 
            $_.Description -like "*CH340*" -or 
            $_.Description -like "*CP210*" -or 
            $_.Description -like "*USB Serial*" -or
            $_.Description -like "*ESP*" -or
            $_.Description -like "*NodeMCU*"
        }
        
        if ($ports) {
            $port = $ports[0].DeviceID
            Write-Host "Auto-detected ESP8266 port: $port" -ForegroundColor Green
            return $port
        } else {
            Write-Host "No ESP8266 device detected automatically" -ForegroundColor Yellow
            return $null
        }
    } catch {
        Write-Host "Error detecting COM ports: $_" -ForegroundColor Red
        return $null
    }
}

# Auto-detect COM port if not specified
if (-not $ComPort) {
    $ComPort = Get-ESP8266ComPort
    if (-not $ComPort) {
        Write-Host "ERROR: No COM port specified and auto-detection failed" -ForegroundColor Red
        Write-Host "Please specify COM port manually: .\test_upload.ps1 -ComPort COM3" -ForegroundColor Yellow
        exit 1
    }
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Arduino Upload Test Script" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# Check if sketch path exists
if (-not (Test-Path $SketchPath)) {
    Write-Host "ERROR: Sketch path does not exist: $SketchPath" -ForegroundColor Red
    exit 1
}

# Check if arduino-cli exists
$arduinoCliPath = Join-Path $PSScriptRoot "arduino-cli.exe"
if (-not (Test-Path $arduinoCliPath)) {
    Write-Host "ERROR: Arduino CLI not found at: $arduinoCliPath" -ForegroundColor Red
    exit 1
}

Write-Host "Sketch Path: $SketchPath" -ForegroundColor Yellow
Write-Host "COM Port: $ComPort" -ForegroundColor Yellow
Write-Host "Arduino CLI: $arduinoCliPath" -ForegroundColor Yellow

# Step 1: Compile
Write-Host "`n1. Compiling sketch..." -ForegroundColor Green
$compileCommand = "& `"$arduinoCliPath`" compile --fqbn esp8266:esp8266:nodemcuv2 `"$SketchPath`""
Write-Host "Command: $compileCommand" -ForegroundColor Gray

try {
    Invoke-Expression $compileCommand
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Compilation failed with exit code $LASTEXITCODE" -ForegroundColor Red
        exit 1
    }
    Write-Host "✅ Compilation successful" -ForegroundColor Green
} catch {
    Write-Host "ERROR: Compilation failed: $_" -ForegroundColor Red
    exit 1
}

# Step 2: Upload
Write-Host "`n2. Uploading to device..." -ForegroundColor Green
$uploadCommand = "& `"$arduinoCliPath`" upload --fqbn esp8266:esp8266:nodemcuv2 -p $ComPort `"$SketchPath`""
Write-Host "Command: $uploadCommand" -ForegroundColor Gray

try {
    Write-Host "Starting upload process (this may take a few minutes)..." -ForegroundColor Yellow
    Invoke-Expression $uploadCommand
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Upload failed with exit code $LASTEXITCODE" -ForegroundColor Red
        exit 1
    }
    Write-Host "✅ Upload successful" -ForegroundColor Green
} catch {
    Write-Host "ERROR: Upload failed: $_" -ForegroundColor Red
    exit 1
}

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "All tests completed successfully!" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan 