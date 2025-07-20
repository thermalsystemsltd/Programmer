# Check Available COM Ports Script
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Available COM Ports" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# Method 1: Using WMI
Write-Host "`nMethod 1: Using WMI" -ForegroundColor Yellow
try {
    $ports = Get-WmiObject -Class Win32_SerialPort | Select-Object Name, DeviceID, Description
    if ($ports) {
        $ports | Format-Table -AutoSize
    } else {
        Write-Host "No COM ports found via WMI" -ForegroundColor Red
    }
} catch {
    Write-Host "Error getting ports via WMI: $_" -ForegroundColor Red
}

# Method 2: Using Registry
Write-Host "`nMethod 2: Using Registry" -ForegroundColor Yellow
try {
    $registryPorts = Get-ItemProperty -Path "HKLM:\HARDWARE\DEVICEMAP\SERIALCOMM" -ErrorAction SilentlyContinue
    if ($registryPorts) {
        $registryPorts.PSObject.Properties | Where-Object { $_.Name -ne "PSPath" -and $_.Name -ne "PSParentPath" -and $_.Name -ne "PSChildName" -and $_.Name -ne "PSProvider" } | ForEach-Object {
            Write-Host "$($_.Name) -> $($_.Value)" -ForegroundColor Green
        }
    } else {
        Write-Host "No COM ports found in registry" -ForegroundColor Red
    }
} catch {
    Write-Host "Error getting ports from registry: $_" -ForegroundColor Red
}

# Method 3: Using Device Manager (simplified)
Write-Host "`nMethod 3: Using PowerShell Device Commands" -ForegroundColor Yellow
try {
    $devices = Get-PnpDevice -Class "Ports" -ErrorAction SilentlyContinue
    if ($devices) {
        $devices | Where-Object { $_.FriendlyName -like "*COM*" } | ForEach-Object {
            Write-Host "$($_.FriendlyName) - Status: $($_.Status)" -ForegroundColor Green
        }
    } else {
        Write-Host "No COM ports found via PnP" -ForegroundColor Red
    }
} catch {
    Write-Host "Error getting PnP devices: $_" -ForegroundColor Red
}

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Troubleshooting Steps:" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "1. Make sure your ESP8266 is connected via USB" -ForegroundColor Yellow
Write-Host "2. Check if the device shows up in Device Manager" -ForegroundColor Yellow
Write-Host "3. Install CH340 or CP210x drivers if needed" -ForegroundColor Yellow
Write-Host "4. Try a different USB cable or port" -ForegroundColor Yellow
Write-Host "5. Put device in programming mode (hold FLASH while connecting)" -ForegroundColor Yellow 