@echo off
echo ========================================
echo Simple Arduino CLI Test
echo ========================================

echo.
echo 1. Checking Arduino CLI...
if exist arduino-cli.exe (
    echo [OK] Arduino CLI found
) else (
    echo [ERROR] Arduino CLI not found
    pause
    exit /b 1
)

echo.
echo 2. Testing Arduino CLI version...
arduino-cli.exe version
if %errorlevel% neq 0 (
    echo [ERROR] Arduino CLI version failed
) else (
    echo [OK] Arduino CLI version works
)

echo.
echo 3. Listing cores...
arduino-cli.exe core list
if %errorlevel% neq 0 (
    echo [ERROR] Core list failed
) else (
    echo [OK] Core list works
)

echo.
echo 4. Listing boards...
arduino-cli.exe board list
if %errorlevel% neq 0 (
    echo [ERROR] Board list failed
) else (
    echo [OK] Board list works
)

echo.
echo 5. Checking COM ports...
wmic path Win32_SerialPort get DeviceID,Description /format:table
if %errorlevel% neq 0 (
    echo [ERROR] COM port check failed
) else (
    echo [OK] COM port check works
)

echo.
echo ========================================
echo Test completed!
echo ========================================
pause 