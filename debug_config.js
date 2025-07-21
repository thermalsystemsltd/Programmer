const fs = require('fs');

// Load config function (same as server.js)
function loadConfig() {
    const CONFIG_FILE = 'config.json';
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
        PCB_GRID_X: 30,
        PCB_GRID_Y: 110,
        PCB_ROWS: 2,
        PCB_COLS: 5,
        PCB_START_X: 0,
        PCB_START_Y: 0,
        PCB_Z_UP: 10,
        PCB_Z_DOWN: 0
    };

    try {
        if (fs.existsSync(CONFIG_FILE)) {
            console.log('📁 Config file exists');
            const configData = fs.readFileSync(CONFIG_FILE, 'utf8');
            console.log('📄 Raw config data:');
            console.log(configData);
            
            const parsedConfig = JSON.parse(configData);
            console.log('🔧 Parsed config:');
            console.log(JSON.stringify(parsedConfig, null, 2));
            
            const finalConfig = { ...DEFAULT_CONFIG, ...parsedConfig };
            console.log('🎯 Final merged config:');
            console.log('PRINTER_COM_PORT:', finalConfig.PRINTER_COM_PORT);
            console.log('COM_PORT:', finalConfig.COM_PORT);
            
            return finalConfig;
        } else {
            console.log('❌ Config file does not exist, using defaults');
            return DEFAULT_CONFIG;
        }
    } catch (error) {
        console.error('❌ Error loading config:', error.message);
        return DEFAULT_CONFIG;
    }
}

// Run the debug
console.log('🔍 Debugging configuration loading...');
console.log('📂 Current directory:', process.cwd());
console.log('🖥️ Platform:', process.platform);

const config = loadConfig();

console.log('\n📋 Final Configuration:');
console.log('PRINTER_COM_PORT:', config.PRINTER_COM_PORT);
console.log('COM_PORT:', config.COM_PORT);
console.log('ARDUINO_SKETCH_PATH:', config.ARDUINO_SKETCH_PATH); 