const { exec } = require('child_process');
const path = require('path');

function programESP8266WithArduinoIDE() {
    return new Promise((resolve, reject) => {
        const sketchPath = currentConfig.ARDUINO_SKETCH_PATH;
        const arduinoIDEPath = 'C:\\Program Files (x86)\\Arduino\\arduino_debug.exe'; // Adjust path as needed
        
        console.log(`🔧 Programming ESP8266 with Arduino IDE...`);
        console.log(`📁 Sketch path: ${sketchPath}`);
        console.log(`🔌 COM Port: ${currentConfig.COM_PORT}`);
        
        // Check if Arduino IDE exists
        if (!fs.existsSync(arduinoIDEPath)) {
            const error = `Arduino IDE not found at: ${arduinoIDEPath}`;
            console.error(`❌ ${error}`);
            reject(new Error(error));
            return;
        }
        
        const command = `"${arduinoIDEPath}" --upload --board esp8266:esp8266:nodemcuv2 --port ${currentConfig.COM_PORT} "${sketchPath}"`;
        
        console.log(`🔨 Command: ${command}`);
        
        exec(command, (error, stdout, stderr) => {
            if (error) {
                const errorMsg = `Arduino IDE upload failed: ${error.message}\nSTDOUT: ${stdout}\nSTDERR: ${stderr}`;
                console.error(`❌ ${errorMsg}`);
                reject(new Error(errorMsg));
                return;
            }
            
            console.log(`✅ Upload successful`);
            console.log(`📋 Output: ${stdout}`);
            resolve(stdout);
        });
    });
}

module.exports = { programESP8266WithArduinoIDE }; 