#!/bin/bash

echo "🔧 Fixing sketch compilation issues..."

# Stop the service
echo "🛑 Stopping TS1 Programmer service..."
sudo systemctl stop ts1-programmer 2>/dev/null || true

# Check what .ino files exist
echo "📋 Checking for .ino files:"
ls -la *.ino 2>/dev/null || echo "No .ino files found"

# Remove any conflicting Programmer.ino file
if [ -f "Programmer.ino" ]; then
    echo "🗑️ Removing conflicting Programmer.ino file..."
    rm Programmer.ino
fi

# Check if sensor.ino exists
if [ ! -f "sensor.ino" ]; then
    echo "❌ Error: sensor.ino not found!"
    echo "📋 Available files:"
    ls -la
    exit 1
fi

echo "✅ Found sensor.ino file"

# Install required libraries
echo "📦 Installing ArduinoJson library..."
./arduino-cli lib install "ArduinoJson"

echo "📦 Installing other common libraries..."
./arduino-cli lib install "WiFiManager"
./arduino-cli lib install "ESP8266WiFi"
./arduino-cli lib install "ESP8266HTTPClient"

# Update core index
echo "🔄 Updating core index..."
./arduino-cli core update-index

# Test compilation
echo "🧪 Testing compilation..."
./arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 sensor.ino --verbose

if [ $? -eq 0 ]; then
    echo "✅ Compilation test successful!"
    
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
    echo "❌ Compilation test failed!"
    exit 1
fi 