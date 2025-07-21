const fs = require('fs');

// Load config
function loadConfig() {
    try {
        if (fs.existsSync('config.json')) {
            const configData = fs.readFileSync('config.json', 'utf8');
            return JSON.parse(configData);
        }
    } catch (error) {
        console.error('Error loading config:', error.message);
    }
    return {
        PRINTER_COM_PORT: 'COM9',
        PRINTER_BAUD_RATE: '115200'
    };
}

// Test SerialPort connection
async function testPrinterConnection() {
    const config = loadConfig();
    console.log(`🔧 Testing printer connection on ${config.PRINTER_COM_PORT} at ${config.PRINTER_BAUD_RATE} baud`);
    
    // Import SerialPort with fallback
    let SerialPort;
    try {
        SerialPort = require('serialport').SerialPort;
        console.log('✅ Using SerialPort v12+ import method');
    } catch (error) {
        try {
            SerialPort = require('serialport');
            console.log('✅ Using SerialPort v11 import method');
        } catch (fallbackError) {
            console.error('❌ SerialPort import failed:', fallbackError.message);
            return;
        }
    }
    
    return new Promise((resolve, reject) => {
        let port;
        try {
            // Try new syntax first
            port = new SerialPort({ path: config.PRINTER_COM_PORT, baudRate: parseInt(config.PRINTER_BAUD_RATE) });
            console.log('✅ Using SerialPort v12+ constructor syntax');
        } catch (constructorError) {
            try {
                // Fallback to old syntax
                port = new SerialPort(config.PRINTER_COM_PORT, { baudRate: parseInt(config.PRINTER_BAUD_RATE) });
                console.log('✅ Using SerialPort v11 constructor syntax');
            } catch (fallbackError) {
                console.error('❌ Failed to create SerialPort:', fallbackError.message);
                reject(fallbackError);
                return;
            }
        }
        
        port.on('open', () => {
            console.log(`🔌 Successfully connected to printer on ${config.PRINTER_COM_PORT}`);
            console.log('📡 Sending test command: M115 (Get firmware info)');
            
            port.write('M115\n', (error) => {
                if (error) {
                    console.error('❌ Error sending command:', error.message);
                    port.close();
                    reject(error);
                } else {
                    console.log('✅ Test command sent successfully');
                    
                    // Wait for response
                    setTimeout(() => {
                        port.close();
                        console.log('🔌 Connection test completed successfully');
                        resolve();
                    }, 2000);
                }
            });
        });
        
        port.on('error', (error) => {
            console.error(`❌ Connection error: ${error.message}`);
            reject(error);
        });
        
        port.on('data', (data) => {
            const response = data.toString().trim();
            console.log(`📡 Printer response: ${response}`);
        });
        
        // Timeout after 5 seconds
        setTimeout(() => {
            if (port.isOpen) {
                port.close();
            }
            reject(new Error('Connection timeout'));
        }, 5000);
    });
}

// Run the test
testPrinterConnection()
    .then(() => {
        console.log('🎉 Printer connection test PASSED!');
        process.exit(0);
    })
    .catch((error) => {
        console.error('💥 Printer connection test FAILED:', error.message);
        process.exit(1);
    }); 