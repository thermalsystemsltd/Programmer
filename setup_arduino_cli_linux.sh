#!/bin/bash

echo "🔧 Setting up Arduino CLI for Linux/Raspberry Pi..."

# Stop the service if it's running
echo "🛑 Stopping TS1 Programmer service..."
sudo systemctl stop ts1-programmer 2>/dev/null || true

# Remove Windows executable if it exists
if [ -f "arduino-cli.exe" ]; then
    echo "🗑️ Removing Windows arduino-cli.exe..."
    rm arduino-cli.exe
fi

# Download Arduino CLI for Linux ARM
echo "📥 Downloading Arduino CLI for Linux ARM..."
curl -fsSL https://downloads.arduino.cc/arduino-cli/arduino-cli_latest_Linux_ARMv7.tar.gz | tar xz

# Make it executable
echo "🔐 Making arduino-cli executable..."
chmod +x arduino-cli

# Test the installation
echo "🧪 Testing Arduino CLI installation..."
./arduino-cli version

if [ $? -eq 0 ]; then
    echo "✅ Arduino CLI installed successfully!"
    
    # Install ESP8266 core
    echo "📦 Installing ESP8266 core..."
    ./arduino-cli core install esp8266:esp8266
    
    # Update core index
    echo "🔄 Updating core index..."
    ./arduino-cli core update-index
    
    echo "✅ Arduino CLI setup completed!"
    echo "🚀 You can now restart the service with: sudo systemctl start ts1-programmer"
else
    echo "❌ Arduino CLI installation failed!"
    exit 1
fi 