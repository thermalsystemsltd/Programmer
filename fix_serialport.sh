#!/bin/bash

echo "🔧 Fixing SerialPort dependency issue..."

# Stop the service if it's running
echo "🛑 Stopping TS1 Programmer service..."
sudo systemctl stop ts1-programmer 2>/dev/null || true

# Remove the problematic serialport version
echo "🗑️ Removing current serialport installation..."
npm uninstall serialport

# Install the stable version
echo "📦 Installing stable serialport version..."
npm install serialport@^11.0.0

# Clear npm cache
echo "🧹 Clearing npm cache..."
npm cache clean --force

# Reinstall all dependencies
echo "🔄 Reinstalling all dependencies..."
rm -rf node_modules package-lock.json
npm install

echo "✅ SerialPort fix completed!"
echo "🚀 You can now restart the service with: sudo systemctl start ts1-programmer" 