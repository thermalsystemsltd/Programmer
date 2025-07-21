#!/bin/bash

echo "📦 Installing all required libraries for sensor.ino..."

# Stop the service
echo "🛑 Stopping TS1 Programmer service..."
sudo systemctl stop ts1-programmer 2>/dev/null || true

# Clear Arduino cache
echo "🧹 Clearing Arduino cache..."
rm -rf ~/.cache/arduino/sketches/*

# Update core index first
echo "🔄 Updating core index..."
./arduino-cli core update-index

# Install all required libraries
echo "📦 Installing libraries..."

# Core ESP8266 libraries (usually included with ESP8266 core)
echo "  - ESP8266WiFi (included with ESP8266 core)"
echo "  - ESP8266HTTPClient (included with ESP8266 core)"
echo "  - ESP8266httpUpdate (included with ESP8266 core)"
echo "  - BearSSLHelpers (included with ESP8266 core)"
echo "  - CertStoreBearSSL (included with ESP8266 core)"
echo "  - LittleFS (included with ESP8266 core)"
echo "  - Wire (included with Arduino core)"
echo "  - SPI (included with Arduino core)"
echo "  - vector (C++ standard library)"
echo "  - algorithm (C++ standard library)"

# Libraries that need to be installed
echo "  - Installing ArduinoJson..."
./arduino-cli lib install "ArduinoJson"

echo "  - Installing base64..."
./arduino-cli lib install "base64"

echo "  - Installing WiFiManager..."
./arduino-cli lib install "WiFiManager"

echo "  - Installing CRC..."
./arduino-cli lib install "CRC"

echo "  - Installing RTClib..."
./arduino-cli lib install "RTClib"

echo "  - Installing LoRa..."
./arduino-cli lib install "LoRa"

echo "  - Installing Adafruit ADS1X15..."
./arduino-cli lib install "Adafruit ADS1X15 Library"

echo "  - Installing INA226..."
./arduino-cli lib install "INA226"

# Note: INA228 is commented out in your code, so we'll skip it
echo "  - Skipping INA228 (commented out in code)"

echo "✅ All libraries installed!"

# Test compilation
echo "🧪 Testing compilation..."
./arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 sensor.ino --verbose

if [ $? -eq 0 ]; then
    echo "✅ Compilation successful!"
    
    # Update config.json to use the correct path
    echo "🔧 Updating config.json..."
    CURRENT_DIR=$(pwd)
    SKETCH_PATH="$CURRENT_DIR/sensor.ino"
    
    # Create backup
    cp config.json config.json.backup
    
    # Update the path
    sed -i "s|\"ARDUINO_SKETCH_PATH\": \".*\"|\"ARDUINO_SKETCH_PATH\": \"$SKETCH_PATH\"|" config.json
    
    echo "✅ Updated ARDUINO_SKETCH_PATH to: $SKETCH_PATH"
    echo "📋 Updated config:"
    cat config.json | grep ARDUINO_SKETCH_PATH
    
    echo "🚀 You can now restart the service with: sudo systemctl start ts1-programmer"
else
    echo "❌ Compilation failed!"
    echo "📋 Checking installed libraries:"
    ./arduino-cli lib list
    exit 1
fi 