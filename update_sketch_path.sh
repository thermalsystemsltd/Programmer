#!/bin/bash

echo "🔧 Updating sketch path in config.json..."

# Get the current directory
CURRENT_DIR=$(pwd)
SKETCH_PATH="$CURRENT_DIR/sensor.ino"

echo "📁 Current directory: $CURRENT_DIR"
echo "📄 Sketch path: $SKETCH_PATH"

# Check if sensor.ino exists
if [ ! -f "$SKETCH_PATH" ]; then
    echo "❌ Error: sensor.ino not found at $SKETCH_PATH"
    echo "📋 Available .ino files:"
    ls -la *.ino 2>/dev/null || echo "No .ino files found in current directory"
    exit 1
fi

# Create a backup of the current config
cp config.json config.json.backup

# Update the config.json with the correct path
jq --arg path "$SKETCH_PATH" '.ARDUINO_SKETCH_PATH = $path' config.json > config.json.tmp && mv config.json.tmp config.json

echo "✅ Updated ARDUINO_SKETCH_PATH to: $SKETCH_PATH"
echo "💾 Backup saved as config.json.backup"

# Show the updated config
echo "📋 Updated config:"
cat config.json | grep ARDUINO_SKETCH_PATH 