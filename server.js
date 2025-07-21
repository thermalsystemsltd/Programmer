const express = require('express');
const cors = require('cors');
const bodyParser = require('body-parser');
const fs = require('fs');
const https = require('https');
const { exec } = require('child_process');
const path = require('path');

// Import SerialPort with better compatibility
let SerialPort;
try {
    // Try the newer import first
    SerialPort = require('serialport').SerialPort;
} catch (error) {
    try {
        // Fallback to older import method
        SerialPort = require('serialport');
    } catch (fallbackError) {
        console.warn('SerialPort import failed, 3D printer functionality will be disabled:', fallbackError.message);
        SerialPort = null;
    }
}

// 3D Printer Control
class PrinterController {
    constructor() {
        this.port = null;
        this.isConnected = false;
        this.currentPosition = { x: 0, y: 0, z: 0 };
        this.isHomed = false;
    }

    async connect(comPort, baudRate = 115200) {
        return new Promise((resolve, reject) => {
            try {
                // Check if SerialPort is available
                if (!SerialPort) {
                    throw new Error('SerialPort library not available. Please run: npm install serialport@^11.0.0');
                }
                
                // Try different constructor syntaxes for compatibility
                let port;
                try {
                    // Try new syntax first (SerialPort v12+)
                    port = new SerialPort({ path: comPort, baudRate: parseInt(baudRate) });
                } catch (constructorError) {
                    try {
                        // Fallback to old syntax (SerialPort v11 and below)
                        port = new SerialPort(comPort, { baudRate: parseInt(baudRate) });
                    } catch (fallbackError) {
                        throw new Error(`Failed to create SerialPort: ${fallbackError.message}`);
                    }
                }
                
                this.port = port;
                
                this.port.on('open', () => {
                    this.isConnected = true;
                    console.log(`🔌 3D Printer connected on ${comPort}`);
                    sendLogToClients({ type: 'success', message: `🔌 3D Printer connected on ${comPort}` });
                    resolve();
                });

                this.port.on('error', (error) => {
                    this.isConnected = false;
                    console.error(`❌ 3D Printer connection error: ${error.message}`);
                    sendLogToClients({ type: 'error', message: `❌ 3D Printer connection error: ${error.message}` });
                    reject(error);
                });

                this.port.on('close', () => {
                    this.isConnected = false;
                    console.log('🔌 3D Printer disconnected');
                    sendLogToClients({ type: 'warning', message: '🔌 3D Printer disconnected' });
                });

                this.port.on('data', (data) => {
                    const response = data.toString().trim();
                    if (response.includes('ok')) {
                        console.log(`📡 Printer response: ${response}`);
                    }
                });

            } catch (error) {
                reject(error);
            }
        });
    }

    async disconnect() {
        if (this.port && this.isConnected) {
            this.port.close();
            this.isConnected = false;
        }
    }

    async sendCommand(command) {
        return new Promise((resolve, reject) => {
            if (!this.isConnected) {
                reject(new Error('Printer not connected'));
                return;
            }

            console.log(`📡 Sending G-code: ${command}`);
            sendLogToClients({ type: 'info', message: `📡 Sending G-code: ${command}` });

            this.port.write(command + '\n', (error) => {
                if (error) {
                    reject(error);
                } else {
                    // Wait a bit for the command to be processed
                    setTimeout(() => resolve(), 100);
                }
            });
        });
    }

    async home() {
        await this.sendCommand('G28');
        this.isHomed = true;
        this.currentPosition = { x: 0, y: 0, z: 0 };
        sendLogToClients({ type: 'success', message: '🏠 Printer homed successfully' });
    }

    async moveTo(x, y, z = null) {
        let command = `G0 X${x} Y${y}`;
        if (z !== null) {
            command += ` Z${z}`;
        }
        
        await this.sendCommand(command);
        this.currentPosition = { x, y, z: z || this.currentPosition.z };
        sendLogToClients({ type: 'info', message: `📍 Moved to X${x} Y${y} Z${z || this.currentPosition.z}` });
    }

    async moveRelative(axis, distance) {
        let newX = this.currentPosition.x;
        let newY = this.currentPosition.y;
        let newZ = this.currentPosition.z;
        
        switch (axis.toUpperCase()) {
            case 'X':
                newX += distance;
                break;
            case 'Y':
                newY += distance;
                break;
            case 'Z':
                newZ += distance;
                break;
            default:
                throw new Error(`Invalid axis: ${axis}. Use X, Y, or Z.`);
        }
        
        await this.moveTo(newX, newY, newZ);
    }

    async waitForMovement() {
        await this.sendCommand('M400'); // Wait for moves to finish
    }

    async getPosition() {
        await this.sendCommand('M114');
        return this.currentPosition;
    }
}

// Global printer controller instance
const printerController = new PrinterController();

const app = express();
const PORT = 3000;

// Middleware
app.use(cors());
app.use(bodyParser.json());
app.use(express.static('public'));

// Configuration file path
const CONFIG_FILE = 'config.json';

// Default configuration
const DEFAULT_CONFIG = {
    ARDUINO_SKETCH_PATH: process.platform === 'win32' 
        ? 'C:\\Users\\adamp\\Documents\\Arduino\\LoraTX NEW PCB 12-F VERSION\\SENSOR V105 PCB INA226\\SENSOR\\mock_sketch.ino'
        : '/home/pi/Arduino/TS1_Sensor/TS1_Sensor.ino',
    WEBHOOK_URL: 'https://n8n.ts1cloud.com/webhook/9121d50c-cccf-4e74-a168-8bbbefb3d79a',
    SUCCESS_WEBHOOK_URL: 'https://n8n.ts1cloud.com/webhook-test/565e375e-ab87-4851-bfb2-57037a5a944d',
    SERIAL_LINE_NUMBER: 137,
    COM_PORT: process.platform === 'win32' ? 'COM5' : '/dev/ttyUSB0',
    BAUD_RATE: '115200',
    PRINTER_COM_PORT: process.platform === 'win32' ? 'COM6' : '/dev/ttyUSB1',
    PRINTER_BAUD_RATE: '115200',
    PCB_GRID_X: 30,  // mm between PCBs horizontally
    PCB_GRID_Y: 110, // mm between PCBs vertically
    PCB_ROWS: 2,     // number of rows
    PCB_COLS: 5,     // number of columns per row
    PCB_START_X: 0,  // X position of first PCB
    PCB_START_Y: 0,  // Y position of first PCB
    PCB_Z_UP: 10,    // Z height when moving between PCBs
    PCB_Z_DOWN: 0    // Z height when programming PCBs
};

// Load configuration
function loadConfig() {
    try {
        if (fs.existsSync(CONFIG_FILE)) {
            const configData = fs.readFileSync(CONFIG_FILE, 'utf8');
            return { ...DEFAULT_CONFIG, ...JSON.parse(configData) };
        }
    } catch (error) {
        console.error('Error loading config:', error.message);
    }
    return DEFAULT_CONFIG;
}

// Save configuration
function saveConfig(config) {
    try {
        fs.writeFileSync(CONFIG_FILE, JSON.stringify(config, null, 2));
        return true;
    } catch (error) {
        console.error('Error saving config:', error.message);
        return false;
    }
}

// Get current configuration
let currentConfig = loadConfig();

// Function to make HTTPS request
function fetchLastSerialNumber() {
    return new Promise((resolve, reject) => {
        const req = https.get(currentConfig.WEBHOOK_URL, (res) => {
            let data = '';
            
            res.on('data', (chunk) => {
                data += chunk;
            });
            
            res.on('end', () => {
                try {
                    const response = JSON.parse(data);
                    // Handle n8n response format: {"success":true,"data":{"nextSerial":"131209"}}
                    const nextSerial = response.data?.nextSerial || response.lastSerial || response.serial || response.data;
                    resolve(nextSerial);
                } catch (error) {
                    reject(new Error(`Failed to parse response: ${error.message}`));
                }
            });
        });
        
        req.on('error', (error) => {
            reject(new Error(`Request failed: ${error.message}`));
        });
        
        req.setTimeout(10000, () => {
            req.destroy();
            reject(new Error('Request timeout'));
        });
    });
}

// Function to notify n8n of successful programming
function notifyProgrammingSuccess(serialNumber) {
    return new Promise((resolve, reject) => {
        // Get the current serial from the file to confirm what was programmed
        getCurrentSerialFromFile()
            .then(currentSerialFromFile => {
                const postData = JSON.stringify({
                    serialNumber: serialNumber,
                    lastUsedSerial: serialNumber,
                    currentSerialInFile: currentSerialFromFile,
                    timestamp: new Date().toISOString(),
                    status: 'programmed',
                    device: 'TS1_ESP8266_Sensor',
                    action: 'notify_success'
                });
        
                // Parse the webhook URL from config
                const webhookUrl = new URL(currentConfig.SUCCESS_WEBHOOK_URL);
                
                const options = {
                    hostname: webhookUrl.hostname,
                    port: webhookUrl.port || (webhookUrl.protocol === 'https:' ? 443 : 80),
                    path: webhookUrl.pathname,
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                        'Content-Length': Buffer.byteLength(postData)
                    }
                };
                
                const req = https.request(options, (res) => {
                    let data = '';
                    
                    res.on('data', (chunk) => {
                        data += chunk;
                    });
                    
                    res.on('end', () => {
                        try {
                            console.log(`✅ Success webhook response: ${res.statusCode} - ${data}`);
                            resolve(data);
                        } catch (error) {
                            reject(new Error(`Failed to parse success webhook response: ${error.message}`));
                        }
                    });
                });
                
                req.on('error', (error) => {
                    reject(new Error(`Success webhook request failed: ${error.message}`));
                });
                
                req.setTimeout(10000, () => {
                    req.destroy();
                    reject(new Error('Success webhook request timeout'));
                });
                
                req.write(postData);
                req.end();
            })
            .catch(error => {
                reject(new Error(`Failed to get current serial from file: ${error.message}`));
            });
    });
}

// Function to get current serial number from Arduino file
function getCurrentSerialFromFile() {
    return new Promise((resolve, reject) => {
        fs.readFile(currentConfig.ARDUINO_SKETCH_PATH, 'utf8', (err, data) => {
            if (err) {
                reject(new Error(`Failed to read Arduino file: ${err.message}`));
                return;
            }

            const lines = data.split('\n');
            
            if (lines.length < currentConfig.SERIAL_LINE_NUMBER) {
                reject(new Error(`File has only ${lines.length} lines, but trying to read line ${currentConfig.SERIAL_LINE_NUMBER}`));
                return;
            }

            const lineIndex = currentConfig.SERIAL_LINE_NUMBER - 1;
            const currentLine = lines[lineIndex];
            
            // Extract serial number from the line
            const match = currentLine.match(/String serialNumber\s*=\s*"(\d+)";/);
            if (match) {
                resolve(match[1]);
            } else {
                reject(new Error(`Could not find serial number in line ${currentConfig.SERIAL_LINE_NUMBER}`));
            }
        });
    });
}

// Function to update Arduino file
function updateArduinoFile(newSerial) {
    return new Promise((resolve, reject) => {
        fs.readFile(currentConfig.ARDUINO_SKETCH_PATH, 'utf8', (err, data) => {
            if (err) {
                reject(new Error(`Failed to read Arduino file: ${err.message}`));
                return;
            }

            const lines = data.split('\n');
            
            if (lines.length < currentConfig.SERIAL_LINE_NUMBER) {
                reject(new Error(`File has only ${lines.length} lines, but trying to update line ${currentConfig.SERIAL_LINE_NUMBER}`));
                return;
            }

            const lineIndex = currentConfig.SERIAL_LINE_NUMBER - 1;
            const currentLine = lines[lineIndex];
            
            const updatedLine = currentLine.replace(
                /String serialNumber\s*=\s*"\d+";/,
                `String serialNumber = "${newSerial}";`
            );
            
            lines[lineIndex] = updatedLine;
            const updatedContent = lines.join('\n');

            fs.writeFile(currentConfig.ARDUINO_SKETCH_PATH, updatedContent, 'utf8', (err) => {
                if (err) {
                    reject(new Error(`Failed to write Arduino file: ${err.message}`));
                    return;
                }
                resolve();
            });
        });
    });
}

// Function to toggle ESP8266 into boot mode using esptool
function toggleBootMode() {
    return new Promise((resolve, reject) => {
        const esptoolPath = path.join(__dirname, 'esptool.exe');
        const bootCommand = `"${esptoolPath}" --chip esp8266 --port ${currentConfig.COM_PORT} --baud 115200 --before default_reset --after hard_reset chip_id`;
        
        console.log(`🔧 Toggling ESP8266 into boot mode...`);
        console.log(`🔧 Boot command: ${bootCommand}`);
        sendLogToClients({ type: 'info', message: `🔧 Toggling ESP8266 into boot mode...` });
        sendLogToClients({ type: 'info', message: `🔧 Boot command: ${bootCommand}` });
        
        exec(bootCommand, { timeout: 30000 }, (error, stdout, stderr) => {
            if (error) {
                console.log(`⚠️ Boot mode toggle failed (this might be normal): ${error.message}`);
                sendLogToClients({ type: 'warning', message: `⚠️ Boot mode toggle failed (this might be normal): ${error.message}` });
                // Don't fail the process, just continue
                resolve();
                return;
            }
            
            console.log(`✅ Boot mode toggle completed`);
            sendLogToClients({ type: 'success', message: `✅ Boot mode toggle completed` });
            resolve();
        });
    });
}

// Function to program multiple PCBs in a grid pattern
async function programPCBGrid() {
    return new Promise(async (resolve, reject) => {
        try {
            // Try to connect to 3D printer
            sendLogToClients({ type: 'info', message: '🔌 Attempting to connect to 3D printer...' });
            
            let printerConnected = false;
            try {
                await printerController.connect(currentConfig.PRINTER_COM_PORT, currentConfig.PRINTER_BAUD_RATE);
                printerConnected = true;
                
                // Home the printer
                sendLogToClients({ type: 'info', message: '🏠 Homing 3D printer...' });
                await printerController.home();
                await printerController.waitForMovement();
                
                // Move to safe Z height
                sendLogToClients({ type: 'info', message: `⬆️ Moving to safe Z height: ${currentConfig.PCB_Z_UP}mm` });
                await printerController.moveTo(0, 0, currentConfig.PCB_Z_UP);
                await printerController.waitForMovement();
            } catch (printerError) {
                sendLogToClients({ type: 'warning', message: `⚠️ 3D Printer not available: ${printerError.message}` });
                sendLogToClients({ type: 'info', message: '🔄 Continuing with programming only (no printer movement)' });
            }
            
            let totalPCBs = currentConfig.PCB_ROWS * currentConfig.PCB_COLS;
            let currentPCB = 0;
            
            sendLogToClients({ type: 'info', message: `🎯 Starting PCB grid programming: ${currentConfig.PCB_ROWS} rows × ${currentConfig.PCB_COLS} columns = ${totalPCBs} PCBs` });
            
            for (let row = 0; row < currentConfig.PCB_ROWS; row++) {
                for (let col = 0; col < currentConfig.PCB_COLS; col++) {
                    currentPCB++;
                    
                    // Calculate position relative to start position
                    const x = currentConfig.PCB_START_X + (col * currentConfig.PCB_GRID_X);
                    const y = currentConfig.PCB_START_Y + (row * currentConfig.PCB_GRID_Y);
                    
                    sendLogToClients({ type: 'info', message: `🎯 Programming PCB ${currentPCB}/${totalPCBs} at position (${col}, ${row}) - X${x} Y${y}` });
                    
                    // Move to PCB position at safe Z height (if printer connected)
                    if (printerConnected) {
                        sendLogToClients({ type: 'info', message: `📍 Moving to PCB position X${x} Y${y} Z${currentConfig.PCB_Z_UP}` });
                        await printerController.moveTo(x, y, currentConfig.PCB_Z_UP);
                        await printerController.waitForMovement();
                        
                        // Small delay to ensure stable position
                        await new Promise(resolve => setTimeout(resolve, 1000));
                        
                        // Lower Z to programming height
                        sendLogToClients({ type: 'info', message: `⬇️ Lowering to programming height Z${currentConfig.PCB_Z_DOWN}` });
                        await printerController.moveTo(x, y, currentConfig.PCB_Z_DOWN);
                        await printerController.waitForMovement();
                        
                        // Small delay to ensure stable contact
                        await new Promise(resolve => setTimeout(resolve, 2000));
                    } else {
                        sendLogToClients({ type: 'info', message: `🎯 Programming PCB ${currentPCB}/${totalPCBs} (no printer movement)` });
                        // Small delay for programming
                        await new Promise(resolve => setTimeout(resolve, 1000));
                    }
                    
                    // Program the PCB
                    try {
                        const result = await programESP8266();
                        sendLogToClients({ type: 'success', message: `✅ PCB ${currentPCB}/${totalPCBs} programmed successfully!` });
                    } catch (error) {
                        sendLogToClients({ type: 'error', message: `❌ PCB ${currentPCB}/${totalPCBs} failed: ${error.message}` });
                        // Continue with next PCB instead of stopping
                    }
                    
                    // Raise Z back to safe height (if printer connected)
                    if (printerConnected) {
                        sendLogToClients({ type: 'info', message: `⬆️ Raising to safe height Z${currentConfig.PCB_Z_UP}` });
                        await printerController.moveTo(x, y, currentConfig.PCB_Z_UP);
                        await printerController.waitForMovement();
                    }
                    
                    // Small delay between PCBs
                    await new Promise(resolve => setTimeout(resolve, 1000));
                }
            }
            
            // Return to home position (if printer connected)
            if (printerConnected) {
                sendLogToClients({ type: 'info', message: '🏠 Returning to home position...' });
                await printerController.moveTo(0, 0, currentConfig.PCB_Z_UP);
                await printerController.waitForMovement();
            }
            
            sendLogToClients({ type: 'big-success', message: `🎉 PCB Grid Programming Complete! ${totalPCBs} PCBs processed` });
            resolve();
            
        } catch (error) {
            sendLogToClients({ type: 'error', message: `❌ PCB Grid Programming failed: ${error.message}` });
            reject(error);
        } finally {
            // Disconnect from printer (if connected)
            if (printerConnected) {
                await printerController.disconnect();
            }
        }
    });
}

// Function to program ESP8266
function programESP8266(serialNumber = null) {
    return new Promise((resolve, reject) => {
        const sketchDir = path.dirname(currentConfig.ARDUINO_SKETCH_PATH);
        const arduinoCliPath = path.join(__dirname, process.platform === 'win32' ? 'arduino-cli.exe' : 'arduino-cli');
        
        console.log(`🔧 TS1 Sensor Programming Started...`);
        console.log(`📁 Sketch directory: ${sketchDir}`);
        console.log(`🔌 COM Port: ${currentConfig.COM_PORT}`);
        console.log(`⚙️ Arduino CLI path: ${arduinoCliPath}`);
        
        // Send initial logs to frontend
        sendLogToClients({ type: 'info', message: '🔧 TS1 Sensor Programming Started...' });
        sendLogToClients({ type: 'info', message: `📁 Sketch directory: ${sketchDir}` });
        sendLogToClients({ type: 'info', message: `🔌 COM Port: ${currentConfig.COM_PORT}` });
        sendLogToClients({ type: 'info', message: `⚙️ Arduino CLI path: ${arduinoCliPath}` });
        
        // Check if sketch directory exists
        if (!fs.existsSync(sketchDir)) {
            const error = `Sketch directory does not exist: ${sketchDir}`;
            console.error(`❌ ${error}`);
            sendLogToClients({ type: 'error', message: `❌ ${error}` });
            reject(new Error(error));
            return;
        }
        
        // Check if Arduino CLI exists
        if (!fs.existsSync(arduinoCliPath)) {
            const error = `Arduino CLI not found at: ${arduinoCliPath}`;
            console.error(`❌ ${error}`);
            sendLogToClients({ type: 'error', message: `❌ ${error}` });
            reject(new Error(error));
            return;
        }
        
        const compileCommand = `"${arduinoCliPath}" compile --fqbn esp8266:esp8266:nodemcuv2 "${sketchDir}" --verbose`;
        const uploadCommand = `"${arduinoCliPath}" upload --fqbn esp8266:esp8266:nodemcuv2 -p ${currentConfig.COM_PORT} "${sketchDir}" --verbose`;
        
        console.log(`🔨 Compile command: ${compileCommand}`);
        sendLogToClients({ type: 'info', message: `🔨 Compile command: ${compileCommand}` });
        
        // First compile
        exec(compileCommand, { cwd: sketchDir }, (compileError, compileStdout, compileStderr) => {
            if (compileError) {
                const error = `Compilation failed: ${compileError.message}\nSTDOUT: ${compileStdout}\nSTDERR: ${compileStderr}`;
                console.error(`❌ ${error}`);
                sendLogToClients({ type: 'error', message: `❌ ${error}` });
                reject(new Error(error));
                return;
            }
            
            console.log(`✅ TS1 Sensor Compilation Successful`);
            console.log(`📤 Upload command: ${uploadCommand}`);
            sendLogToClients({ type: 'success', message: `✅ TS1 Sensor Compilation Successful` });
            sendLogToClients({ type: 'info', message: `📤 Upload command: ${uploadCommand}` });
            
            // Toggle device into boot mode before upload
            toggleBootMode()
                .then(() => {
                                        // Then upload
                    console.log(`📤 Starting upload process...`);
                    sendLogToClients({ type: 'info', message: `📤 Starting upload process...` });
                    
                    let uploadSuccess = false;
                    let uploadCompleted = false;
                    
                    const uploadProcess = exec(uploadCommand, { cwd: sketchDir }, (uploadError, uploadStdout, uploadStderr) => {
                        uploadCompleted = true;
                        
                        if (uploadError) {
                            const error = `Upload failed: ${uploadError.message}\nSTDOUT: ${uploadStdout}\nSTDERR: ${uploadStderr}`;
                            console.error(`❌ ${error}`);
                            sendLogToClients({ type: 'error', message: `❌ ${error}` });
                            sendLogToClients({ type: 'error', message: `❌ PROGRAMMING FAILED - No success webhook sent` });
                            reject(new Error(error));
                            return;
                        }
                        
                        // Check if upload was actually successful
                        if (uploadSuccess && uploadStdout.includes('Hash of data verified')) {
                            console.log(`✅ TS1 Sensor Upload Successful`);
                            console.log(`📋 Upload output: ${uploadStdout}`);
                            sendLogToClients({ type: 'success', message: `✅ TS1 Sensor Upload Successful` });
                            sendLogToClients({ type: 'info', message: `📋 Upload output: ${uploadStdout}` });
                            
                            // Notify n8n of successful programming if serial number is provided
                            if (serialNumber) {
                                console.log(`📡 Notifying n8n SUCCESS webhook for serial: ${serialNumber}`);
                                console.log(`📡 Success webhook URL: ${currentConfig.SUCCESS_WEBHOOK_URL}`);
                                sendLogToClients({ type: 'info', message: `📡 Notifying n8n SUCCESS webhook for serial: ${serialNumber}` });
                                sendLogToClients({ type: 'info', message: `📡 Success webhook URL: ${currentConfig.SUCCESS_WEBHOOK_URL}` });
                                
                                notifyProgrammingSuccess(serialNumber)
                                    .then(() => {
                                        console.log(`✅ Success webhook sent successfully`);
                                        sendLogToClients({ type: 'success', message: `✅ Success webhook sent successfully` });
                                        
                                        // Send BIG SUCCESS message
                                        sendLogToClients({ type: 'big-success', message: `🎉 SENSOR PROGRAMMED SUCCESSFULLY! 🎉` });
                                        sendLogToClients({ type: 'big-success', message: `Serial Number: ${serialNumber}` });
                                        sendLogToClients({ type: 'big-success', message: `Device is ready for deployment!` });
                                        
                                        resolve(uploadStdout);
                                    })
                                    .catch((error) => {
                                        console.error(`❌ Success webhook failed: ${error.message}`);
                                        sendLogToClients({ type: 'error', message: `❌ Success webhook failed: ${error.message}` });
                                        // Don't fail the programming process if webhook fails
                                        resolve(uploadStdout);
                                    });
                            } else {
                                // Send BIG SUCCESS message even without webhook
                                sendLogToClients({ type: 'big-success', message: `🎉 SENSOR PROGRAMMED SUCCESSFULLY! 🎉` });
                                sendLogToClients({ type: 'big-success', message: `Device is ready for deployment!` });
                                resolve(uploadStdout);
                            }
                        } else {
                            const error = `Upload completed but verification failed. Output: ${uploadStdout}`;
                            console.error(`❌ ${error}`);
                            sendLogToClients({ type: 'error', message: `❌ ${error}` });
                            sendLogToClients({ type: 'error', message: `❌ PROGRAMMING FAILED - No success webhook sent` });
                            reject(new Error(error));
                        }
                    });
        
        // Add timeout and better process handling
        uploadProcess.stdout.on('data', (data) => {
            const output = data.toString().trim();
            console.log(`📤 Upload progress: ${output}`);
            sendLogToClients({ type: 'progress', message: `📤 Upload progress: ${output}` });
            
            // Check for successful completion indicators
            if (output.includes('Hash of data verified')) {
                uploadSuccess = true;
                console.log(`✅ Upload verification detected: ${output}`);
                sendLogToClients({ type: 'success', message: `✅ Upload verification detected: ${output}` });
            } else if (output.includes('Leaving...') || output.includes('Hard resetting')) {
                console.log(`✅ Upload completion detected: ${output}`);
                sendLogToClients({ type: 'success', message: `✅ Upload completion detected: ${output}` });
            }
        });
        
        uploadProcess.stderr.on('data', (data) => {
            const output = data.toString().trim();
            console.log(`📤 Upload stderr: ${output}`);
            sendLogToClients({ type: 'warning', message: `📤 Upload stderr: ${output}` });
        });
        
        // Set a longer timeout for upload (3 minutes)
        const uploadTimeout = setTimeout(() => {
            if (uploadProcess.exitCode === null) {
                console.log(`⚠️ Upload timeout reached, but process may still be running`);
            }
        }, 180000);
        
        // Clear timeout when process completes
        uploadProcess.on('close', (code) => {
            clearTimeout(uploadTimeout);
            console.log(`📤 Upload process closed with code: ${code}`);
            sendLogToClients({ type: 'info', message: `📤 Upload process closed with code: ${code}` });
        });
                })
                .catch((error) => {
                    console.error(`❌ Boot mode toggle failed: ${error.message}`);
                    sendLogToClients({ type: 'error', message: `❌ Boot mode toggle failed: ${error.message}` });
                    reject(error);
                });
        });
    });
}

// Store active connections for real-time logging
let activeConnections = [];

// SSE endpoint for real-time logging
app.get('/api/logs', (req, res) => {
    res.writeHead(200, {
        'Content-Type': 'text/event-stream',
        'Cache-Control': 'no-cache',
        'Connection': 'keep-alive',
        'Access-Control-Allow-Origin': '*'
    });

    const clientId = Date.now();
    const newClient = {
        id: clientId,
        res
    };

    activeConnections.push(newClient);

    // Send initial connection message
    res.write(`data: ${JSON.stringify({ type: 'connection', message: 'Connected to TS1 Sensor Programmer logs' })}\n\n`);

    req.on('close', () => {
        activeConnections = activeConnections.filter(client => client.id !== clientId);
    });
});

// Function to send logs to all connected clients
function sendLogToClients(logData) {
    activeConnections.forEach(client => {
        client.res.write(`data: ${JSON.stringify(logData)}\n\n`);
    });
}

// API Routes
app.get('/api/status', (req, res) => {
    res.json({ 
        status: 'running',
        timestamp: new Date().toISOString(),
        arduinoPath: currentConfig.ARDUINO_SKETCH_PATH,
        webhookUrl: currentConfig.WEBHOOK_URL,
        successWebhookUrl: currentConfig.SUCCESS_WEBHOOK_URL,
        serialLineNumber: currentConfig.SERIAL_LINE_NUMBER,
        comPort: currentConfig.COM_PORT,
        baudRate: currentConfig.BAUD_RATE
    });
});

app.get('/api/config', (req, res) => {
    res.json(currentConfig);
});

app.post('/api/config', (req, res) => {
    try {
        const { webhookUrl, successWebhookUrl, arduinoSketchPath, serialLineNumber, comPort, baudRate, printerComPort, printerBaudRate, pcbGridX, pcbGridY, pcbRows, pcbCols, pcbStartX, pcbStartY, pcbZUp, pcbZDown } = req.body;
        
        // Update configuration
        if (webhookUrl) currentConfig.WEBHOOK_URL = webhookUrl;
        if (successWebhookUrl) currentConfig.SUCCESS_WEBHOOK_URL = successWebhookUrl;
        if (arduinoSketchPath) currentConfig.ARDUINO_SKETCH_PATH = arduinoSketchPath;
        if (serialLineNumber) currentConfig.SERIAL_LINE_NUMBER = parseInt(serialLineNumber);
        if (comPort) currentConfig.COM_PORT = comPort;
        if (baudRate) currentConfig.BAUD_RATE = baudRate;
        if (printerComPort) currentConfig.PRINTER_COM_PORT = printerComPort;
        if (printerBaudRate) currentConfig.PRINTER_BAUD_RATE = printerBaudRate;
        if (pcbGridX) currentConfig.PCB_GRID_X = parseInt(pcbGridX);
        if (pcbGridY) currentConfig.PCB_GRID_Y = parseInt(pcbGridY);
        if (pcbRows) currentConfig.PCB_ROWS = parseInt(pcbRows);
        if (pcbCols) currentConfig.PCB_COLS = parseInt(pcbCols);
        if (pcbStartX) currentConfig.PCB_START_X = parseInt(pcbStartX);
        if (pcbStartY) currentConfig.PCB_START_Y = parseInt(pcbStartY);
        if (pcbZUp) currentConfig.PCB_Z_UP = parseInt(pcbZUp);
        if (pcbZDown) currentConfig.PCB_Z_DOWN = parseInt(pcbZDown);
        
        // Save to file
        if (saveConfig(currentConfig)) {
            res.json({
                success: true,
                message: 'Configuration updated successfully',
                config: currentConfig
            });
        } else {
            res.status(500).json({
                success: false,
                error: 'Failed to save configuration'
            });
        }
    } catch (error) {
        res.status(500).json({
            success: false,
            error: error.message
        });
    }
});

app.post('/api/fetch-serial', async (req, res) => {
    try {
        const lastSerial = await fetchLastSerialNumber();
        const nextSerial = (parseInt(lastSerial) + 1).toString();
        
        res.json({
            success: true,
            lastSerial,
            nextSerial
        });
    } catch (error) {
        res.status(500).json({
            success: false,
            error: error.message
        });
    }
});

// Proxy endpoint to handle CORS
app.get('/api/proxy-webhook', async (req, res) => {
    try {
        const webhookUrl = req.query.url || currentConfig.WEBHOOK_URL;
        
        const response = await fetch(webhookUrl);
        const data = await response.json();
        
        res.json({
            success: true,
            data: data
        });
    } catch (error) {
        res.status(500).json({
            success: false,
            error: error.message
        });
    }
});

app.post('/api/update-serial', async (req, res) => {
    try {
        const { serialNumber } = req.body;
        
        if (!serialNumber) {
            return res.status(400).json({
                success: false,
                error: 'Serial number is required'
            });
        }

        await updateArduinoFile(serialNumber);
        
        res.json({
            success: true,
            message: `Serial number updated to ${serialNumber}`
        });
    } catch (error) {
        res.status(500).json({
            success: false,
            error: error.message
        });
    }
});

app.post('/api/program-device', async (req, res) => {
    try {
        const { serialNumber } = req.body;
        const output = await programESP8266(serialNumber);
        
        res.json({
            success: true,
            message: 'Device programmed successfully',
            output
        });
    } catch (error) {
        res.status(500).json({
            success: false,
            error: error.message
        });
    }
});

app.post('/api/test-arduino-cli', async (req, res) => {
    try {
        const arduinoCliPath = path.join(__dirname, process.platform === 'win32' ? 'arduino-cli.exe' : 'arduino-cli');
        
        // Test Arduino CLI version
        const versionCommand = `"${arduinoCliPath}" version`;
        const versionResult = await new Promise((resolve, reject) => {
            exec(versionCommand, (error, stdout, stderr) => {
                if (error) {
                    reject(new Error(`Version check failed: ${error.message}`));
                    return;
                }
                resolve(stdout);
            });
        });
        
        // Test core list
        const coreCommand = `"${arduinoCliPath}" core list`;
        const coreResult = await new Promise((resolve, reject) => {
            exec(coreCommand, (error, stdout, stderr) => {
                if (error) {
                    reject(new Error(`Core list failed: ${error.message}`));
                    return;
                }
                resolve(stdout);
            });
        });
        
        // Test board list
        const boardCommand = `"${arduinoCliPath}" board list`;
        const boardResult = await new Promise((resolve, reject) => {
            exec(boardCommand, (error, stdout, stderr) => {
                if (error) {
                    reject(new Error(`Board list failed: ${error.message}`));
                    return;
                }
                resolve(stdout);
            });
        });
        
        res.json({
            success: true,
            arduinoCliPath,
            version: versionResult,
            cores: coreResult,
            boards: boardResult
        });
    } catch (error) {
        res.status(500).json({
            success: false,
            error: error.message
        });
    }
});

// Test success webhook endpoint
app.post('/api/test-success-webhook', async (req, res) => {
    try {
        const { serialNumber } = req.body;
        
        if (!serialNumber) {
            return res.status(400).json({
                success: false,
                error: 'Serial number is required'
            });
        }
        
        console.log(`🧪 Testing success webhook with serial: ${serialNumber}`);
        const result = await notifyProgrammingSuccess(serialNumber);
        
        res.json({
            success: true,
            message: 'Success webhook test completed',
            result: result
        });
    } catch (error) {
        res.status(500).json({
            success: false,
            error: error.message
        });
    }
});

// 3D Printer Control Endpoints
app.post('/api/printer/connect', async (req, res) => {
    try {
        const { comPort, baudRate } = req.body;
        const port = comPort || currentConfig.PRINTER_COM_PORT;
        const baud = baudRate || currentConfig.PRINTER_BAUD_RATE;
        
        await printerController.connect(port, baud);
        
        res.json({
            success: true,
            message: '3D Printer connected successfully'
        });
    } catch (error) {
        res.status(500).json({
            success: false,
            error: error.message
        });
    }
});

app.post('/api/printer/disconnect', async (req, res) => {
    try {
        await printerController.disconnect();
        
        res.json({
            success: true,
            message: '3D Printer disconnected successfully'
        });
    } catch (error) {
        res.status(500).json({
            success: false,
            error: error.message
        });
    }
});

app.post('/api/printer/home', async (req, res) => {
    try {
        await printerController.home();
        
        res.json({
            success: true,
            message: '3D Printer homed successfully'
        });
    } catch (error) {
        res.status(500).json({
            success: false,
            error: error.message
        });
    }
});

app.post('/api/printer/move', async (req, res) => {
    try {
        const { x, y, z, axis, distance } = req.body;
        
        if (axis && distance !== undefined) {
            // Relative movement by axis
            await printerController.moveRelative(axis, distance);
            res.json({
                success: true,
                message: `Moved ${axis} by ${distance}mm`
            });
        } else if (x !== undefined && y !== undefined) {
            // Absolute movement to coordinates
            await printerController.moveTo(x, y, z);
            res.json({
                success: true,
                message: `Moved to X${x} Y${y} Z${z || 'current'}`
            });
        } else {
            return res.status(400).json({
                success: false,
                error: 'Either (x,y,z) coordinates or (axis,distance) are required'
            });
        }
    } catch (error) {
        res.status(500).json({
            success: false,
            error: error.message
        });
    }
});

app.post('/api/printer/grid-program', async (req, res) => {
    try {
        // Start the grid programming process
        programPCBGrid()
            .then(() => {
                console.log('PCB Grid programming completed');
            })
            .catch((error) => {
                console.error('PCB Grid programming failed:', error);
            });
        
        res.json({
            success: true,
            message: 'PCB Grid programming started'
        });
    } catch (error) {
        res.status(500).json({
            success: false,
            error: error.message
        });
    }
});

app.get('/api/printer/status', async (req, res) => {
    try {
        const position = await printerController.getPosition();
        
        res.json({
            success: true,
            connected: printerController.isConnected,
            homed: printerController.isHomed,
            position: position
        });
    } catch (error) {
        res.status(500).json({
            success: false,
            error: error.message
        });
    }
});

// Get connected devices and COM ports
app.get('/api/connected-devices', async (req, res) => {
    try {
        const devices = [];
        const isWindows = process.platform === 'win32';
        
        if (isWindows) {
            // Windows: Using WMI to get serial ports
            const wmiCommand = 'wmic path Win32_SerialPort get DeviceID,Description,Name /format:csv';
            const wmiResult = await new Promise((resolve, reject) => {
                exec(wmiCommand, (error, stdout, stderr) => {
                    if (error) {
                        reject(new Error(`WMI command failed: ${error.message}`));
                        return;
                    }
                    resolve(stdout);
                });
            });
            
            // Parse WMI results
            const lines = wmiResult.split('\n').filter(line => line.trim() && !line.includes('Node,DeviceID,Description,Name'));
            lines.forEach(line => {
                const parts = line.split(',');
                if (parts.length >= 4) {
                    const deviceId = parts[1]?.trim();
                    const description = parts[2]?.trim();
                    const name = parts[3]?.trim();
                    
                    if (deviceId && deviceId.startsWith('COM')) {
                        devices.push({
                            port: deviceId,
                            description: description || 'Unknown Device',
                            name: name || description || 'Unknown Device',
                            type: getDeviceType(description || name || '')
                        });
                    }
                }
            });
            
            // Method 2: Using Registry as backup
            if (devices.length === 0) {
                const registryCommand = 'reg query "HKLM\\HARDWARE\\DEVICEMAP\\SERIALCOMM" /v /f "*"';
                const registryResult = await new Promise((resolve, reject) => {
                    exec(registryCommand, (error, stdout, stderr) => {
                        if (error) {
                            resolve(''); // Don't fail, just return empty
                            return;
                        }
                        resolve(stdout);
                    });
                });
                
                const registryLines = registryResult.split('\n').filter(line => line.includes('COM'));
                registryLines.forEach(line => {
                    const match = line.match(/COM\d+/);
                    if (match) {
                        devices.push({
                            port: match[0],
                            description: 'Serial Device (Registry)',
                            name: match[0],
                            type: 'unknown'
                        });
                    }
                });
            }
            
            // Sort devices by port number
            devices.sort((a, b) => {
                const aNum = parseInt(a.port.replace('COM', ''));
                const bNum = parseInt(b.port.replace('COM', ''));
                return aNum - bNum;
            });
        } else {
            // Linux: Using ls and lsusb to get serial ports
            try {
                const lsResult = await new Promise((resolve, reject) => {
                    exec('ls -la /dev/tty* | grep -E "(USB|ACM)"', (error, stdout, stderr) => {
                        if (error) {
                            resolve(''); // Don't fail, just return empty
                            return;
                        }
                        resolve(stdout);
                    });
                });
                
                const lsLines = lsResult.split('\n').filter(line => line.trim());
                lsLines.forEach(line => {
                    const match = line.match(/\/dev\/tty\w+/);
                    if (match) {
                        const port = match[0];
                        devices.push({
                            port: port,
                            description: `USB Serial Device (${port})`,
                            name: port,
                            type: 'serial'
                        });
                    }
                });
                
                // Get additional USB info
                try {
                    const usbResult = await new Promise((resolve, reject) => {
                        exec('lsusb', (error, stdout, stderr) => {
                            if (error) {
                                resolve('');
                                return;
                            }
                            resolve(stdout);
                        });
                    });
                    
                    const usbLines = usbResult.split('\n').filter(line => line.trim());
                    usbLines.forEach((usbLine, index) => {
                        if (devices[index]) {
                            devices[index].description = usbLine.trim();
                            devices[index].type = getDeviceType(usbLine.toLowerCase());
                        }
                    });
                } catch (usbError) {
                    // USB info failed, continue with basic device list
                }
                
                // Sort devices by port name
                devices.sort((a, b) => a.port.localeCompare(b.port));
            } catch (lsError) {
                // If ls command fails, provide some default devices
                devices.push(
                    { port: '/dev/ttyUSB0', description: 'USB Serial Device 0', name: '/dev/ttyUSB0', type: 'serial' },
                    { port: '/dev/ttyUSB1', description: 'USB Serial Device 1', name: '/dev/ttyUSB1', type: 'serial' },
                    { port: '/dev/ttyACM0', description: 'ACM Serial Device 0', name: '/dev/ttyACM0', type: 'serial' }
                );
            }
        }
        
        res.json({
            success: true,
            devices: devices,
            count: devices.length
        });
    } catch (error) {
        res.status(500).json({
            success: false,
            error: error.message,
            devices: []
        });
    }
});

// Helper function to determine device type
function getDeviceType(description) {
    const desc = description.toLowerCase();
    if (desc.includes('ch340') || desc.includes('ch341')) return 'esp8266';
    if (desc.includes('cp210') || desc.includes('silicon labs')) return 'esp8266';
    if (desc.includes('ftdi')) return 'esp8266';
    if (desc.includes('arduino')) return 'arduino';
    if (desc.includes('esp')) return 'esp8266';
    if (desc.includes('nodemcu')) return 'esp8266';
    if (desc.includes('usb serial')) return 'serial';
    if (desc.includes('bluetooth')) return 'bluetooth';
    return 'unknown';
}

app.post('/api/full-process', async (req, res) => {
    try {
        // Step 1: Fetch last serial number
        const lastSerial = await fetchLastSerialNumber();
        const nextSerial = (parseInt(lastSerial) + 1).toString();
        
        // Step 2: Update Arduino file
        await updateArduinoFile(nextSerial);
        
        // Step 3: Program device
        const output = await programESP8266(nextSerial);
        
        res.json({
            success: true,
            lastSerial,
            nextSerial,
            message: 'Full process completed successfully',
            output
        });
    } catch (error) {
        res.status(500).json({
            success: false,
            error: error.message
        });
    }
});

// Serve the main page
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

// Start server
app.listen(PORT, () => {
    console.log(`🚀 TS1 Sensor Programmer running on http://localhost:${PORT}`);
    console.log(`📁 Arduino sketch path: ${currentConfig.ARDUINO_SKETCH_PATH}`);
    console.log(`🔗 Webhook URL: ${currentConfig.WEBHOOK_URL}`);
    console.log(`📝 Serial line number: ${currentConfig.SERIAL_LINE_NUMBER}`);
    console.log(`🔌 COM Port: ${currentConfig.COM_PORT}`);
    console.log(`⚡ Baud Rate: ${currentConfig.BAUD_RATE}`);
}); 