# 🔧 TS1 Sensor Programmer Troubleshooting Guide

## Problem: "path" is not defined: undefined when connecting to 3D printer

### ✅ What We've Fixed:
1. **SerialPort Version Issue**: Updated from serialport v12.0.0 to v11.0.0 for better compatibility
2. **Import Error Handling**: Added try-catch around SerialPort import to prevent crashes
3. **Constructor Fix**: Updated SerialPort constructor to use object syntax: `{ path: comPort, baudRate: baudRate }`

### 🔍 Solution Steps:

#### 1. **Run the Fix Script (Recommended)**
```bash
chmod +x fix_serialport.sh
./fix_serialport.sh
```

#### 2. **Manual Fix**
If the script doesn't work, run these commands manually:
```bash
# Stop the service
sudo systemctl stop ts1-programmer

# Remove problematic serialport
npm uninstall serialport

# Install stable version
npm install serialport@^11.0.0

# Clear cache and reinstall
npm cache clean --force
rm -rf node_modules package-lock.json
npm install

# Restart service
sudo systemctl start ts1-programmer
```

#### 3. **Verify the Fix**
Check the logs to confirm the fix worked:
```bash
sudo journalctl -u ts1-programmer -f
```

You should see: `🔌 3D Printer connected on [COM_PORT]` instead of the path error.

### 🚨 Why This Happens:
- SerialPort v12.0.0 has compatibility issues on Raspberry Pi
- The library tries to access a `path` variable that's undefined in certain environments
- Version 11.0.0 is more stable and widely compatible

---

## Problem: Programming fails silently without debug information

### ✅ What We've Fixed:
1. **Enhanced Error Logging**: Added detailed console logging to the `programESP8266()` function
2. **Separated Compile/Upload**: Split the process into two steps for better error detection
3. **Added Debug Endpoint**: Created `/api/test-arduino-cli` to test Arduino CLI setup
4. **Added Debug Button**: Added "Test Arduino CLI" button to the TS1 Sensor Programmer interface

### 🔍 Step-by-Step Troubleshooting:

#### 1. **Test Arduino CLI Setup**
- Click the "Test Arduino CLI" button in the TS1 Sensor Programmer interface
- Check the log output for any errors
- Verify that Arduino CLI version, cores, and boards are detected

#### 2. **Check File Paths**
- Verify the Arduino sketch path in your config is correct
- Ensure the sketch directory exists and contains a `.ino` file
- Check that `arduino-cli.exe` exists in your project directory

#### 3. **Manual Arduino CLI Test**
Open Command Prompt in your project directory and run:
```bash
.\arduino-cli.exe version
.\arduino-cli.exe core list
.\arduino-cli.exe board list
```

#### 4. **Check ESP8266 Core Installation**
```bash
.\arduino-cli.exe core install esp8266:esp8266
```

#### 5. **Test Compilation Manually**
```bash
.\arduino-cli.exe compile --fqbn esp8266:esp8266:nodemcuv2 "C:\path\to\your\sketch\directory"
```

#### 6. **Check Device Connection**
- Ensure ESP8266 is connected to the correct COM port
- Verify the device is in programming mode
- Check device drivers are installed

### 🚨 Common Issues and Solutions:

#### **Issue 1: Arduino CLI not found**
**Solution**: Run the download script:
```bash
download_arduino_cli.bat
```

#### **Issue 2: ESP8266 core not installed**
**Solution**: Install the core:
```bash
.\arduino-cli.exe core install esp8266:esp8266
```

#### **Issue 3: Sketch path doesn't exist**
**Solution**: Update the path in config.json to point to a valid Arduino sketch

#### **Issue 4: Device not in programming mode**
**Solution**: 
- Hold FLASH button while connecting power
- Or press RESET while holding FLASH, then release FLASH

#### **Issue 5: Wrong COM port**
**Solution**: Check Device Manager for the correct COM port

### 🔄 Alternative Solutions:

#### **Option A: Use Arduino IDE Instead**
If Arduino CLI continues to fail, use the Arduino IDE:
1. Install Arduino IDE
2. Add ESP8266 board manager
3. Use the `arduino_ide_programmer.js` alternative

#### **Option B: Manual Programming**
1. Update the sketch manually with the new serial number
2. Use Arduino IDE to compile and upload
3. Verify the upload was successful

### 📋 Debug Checklist:
- [ ] Arduino CLI executable exists and runs
- [ ] ESP8266 core is installed
- [ ] Sketch path is valid and contains .ino file
- [ ] Device is connected to correct COM port
- [ ] Device is in programming mode
- [ ] No other programs are using the COM port
- [ ] Device drivers are properly installed

### 🆘 If All Else Fails:
1. **Restart the Node.js server**
2. **Restart your computer**
3. **Try a different USB cable**
4. **Try a different USB port**
5. **Reinstall Arduino CLI**
6. **Use Arduino IDE as fallback**

### 📞 Getting Help:
If you're still having issues, provide:
1. Output from "Test Arduino CLI" button
2. Server console logs
3. Arduino CLI version
4. ESP8266 board model
5. Operating system version 