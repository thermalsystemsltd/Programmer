# 🍓 Raspberry Pi Deployment Guide

## Perfect Production Setup for TS1 Sensor Programmer

The TS1 Sensor Programmer is **specifically designed** to run on Raspberry Pi for production use. This guide covers everything you need to deploy it successfully.

## 🎯 Why Raspberry Pi?

- **Cost-effective** - Much cheaper than a full PC
- **Low power** - 24/7 operation without high electricity costs
- **Compact** - Fits easily in production environments
- **Reliable** - Stable Linux-based operation
- **Touch-friendly** - Perfect for 7-inch touch screens

## 📋 Hardware Requirements

### Minimum Setup
- **Raspberry Pi 3B+** (2GB RAM)
- **8GB+ microSD card** (Class 10 recommended)
- **Power supply** (3A, 5V)
- **USB cable** for ESP8266 connection

### Recommended Setup
- **Raspberry Pi 4** (4GB RAM)
- **32GB+ microSD card** (Class 10)
- **7-inch touch screen** with case
- **3A power supply**
- **USB hub** for multiple devices
- **Cooling fan** (for Pi 4)

### Optional Additions
- **3D printer** for automated PCB positioning
- **Network cable** for stable connection
- **UPS** for power protection

## 🚀 Step-by-Step Installation

### 1. Prepare Raspberry Pi OS

**Option A: Desktop Version (with touch screen)**
```bash
# Download Raspberry Pi Imager
# Flash Raspberry Pi OS (32-bit) with desktop
# Enable SSH during setup - adad
```

**Option B: Lite Version (headless)**
```bash
# Flash Raspberry Pi OS Lite
# Create ssh file in boot partition
# Configure network in wpa_supplicant.conf
```

### 2. Initial Setup

```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Set hostname (optional)
sudo hostnamectl set-hostname ts1-programmer

# Enable SSH (if not already enabled)
sudo systemctl enable ssh
sudo systemctl start ssh
```

### 3. Install Node.js 18+

```bash
# Add NodeSource repository
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -

# Install Node.js
sudo apt-get install -y nodejs

# Verify installation
node --version
npm --version
```

### 4. Install Arduino CLI

```bash
# Download and install Arduino CLI
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

# Move to system path
sudo mv bin/arduino-cli /usr/local/bin/

# Update package index
arduino-cli core update-index

# Install ESP8266 core
arduino-cli core install esp8266:esp8266

# Verify installation
arduino-cli version
arduino-cli core list
```

### 5. Install Additional Dependencies

```bash
# Install build tools (needed for some npm packages)
sudo apt-get install -y build-essential python3

# Install git
sudo apt-get install -y git

# Install screen (for background operation)
sudo apt-get install -y screen
```

### 6. Clone and Setup TS1 Programmer

```bash
# Clone repository
git clone https://github.com/thermalsystemsltd/Programmer.git
cd Programmer

# Install dependencies
npm install

# Copy configuration
cp config.example.json config.json

# Edit configuration
nano config.json
```

### 7. Configure the System

Edit `config.json` with your settings:

```json
{
  "ARDUINO_SKETCH_PATH": "/home/pi/Arduino/TS1_Sensor/TS1_Sensor.ino",
  "WEBHOOK_URL": "https://n8n.ts1cloud.com/webhook/your-webhook-id",
  "SUCCESS_WEBHOOK_URL": "https://n8n.ts1cloud.com/webhook/success",
  "COM_PORT": "/dev/ttyUSB0",
  "BAUD_RATE": "115200",
  "PRINTER_COM_PORT": "/dev/ttyUSB1",
  "PRINTER_BAUD_RATE": "115200",
  "PCB_GRID_X": 30,
  "PCB_GRID_Y": 110,
  "PCB_ROWS": 2,
  "PCB_COLS": 5,
  "PCB_START_X": 0,
  "PCB_START_Y": 0,
  "PCB_Z_UP": 10,
  "PCB_Z_DOWN": 0
}
```

**Important Pi Notes:**
- COM ports are `/dev/ttyUSB0`, `/dev/ttyUSB1`, etc. on Linux
- Use absolute paths for Arduino sketch
- Ensure proper permissions for USB devices

### 8. Test the Installation

```bash
# Start the server
npm start

# Check if it's running
curl http://localhost:3000/api/status
```

### 9. Access the Interface

- **Local access**: `http://localhost:3000`
- **Network access**: `http://[PI_IP_ADDRESS]:3000`
- **Find IP address**: `hostname -I`

## 🔧 Auto-Start Setup

### Create Systemd Service

```bash
sudo nano /etc/systemd/system/ts1-programmer.service
```

Add this content:

```ini
[Unit]
Description=TS1 Sensor Programmer
After=network.target
Wants=network.target

[Service]
Type=simple
User=pi
Group=pi
WorkingDirectory=/home/pi/Programmer
ExecStart=/usr/bin/node server.js
Restart=always
RestartSec=10
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
```

### Enable and Start Service

```bash
# Reload systemd
sudo systemctl daemon-reload

# Enable service (starts on boot)
sudo systemctl enable ts1-programmer

# Start service
sudo systemctl start ts1-programmer

# Check status
sudo systemctl status ts1-programmer

# View logs
sudo journalctl -u ts1-programmer -f
```

## 🖥️ Touch Screen Setup (Optional)

### Install Touch Screen Drivers

```bash
# For official 7-inch touch screen
sudo apt-get install -y xserver-xorg-input-evdev

# For other touch screens, follow manufacturer instructions
```

### Auto-Start Browser (Desktop Mode)

Create autostart entry:

```bash
mkdir -p ~/.config/autostart
nano ~/.config/autostart/ts1-programmer.desktop
```

Add content:

```ini
[Desktop Entry]
Type=Application
Name=TS1 Programmer
Exec=chromium-browser --kiosk --disable-web-security --user-data-dir=/tmp/chrome --no-first-run http://localhost:3000
Terminal=false
X-GNOME-Autostart-enabled=true
```

## 🔌 USB Device Permissions

### Fix USB Device Access

```bash
# Add user to dialout group (for serial devices)
sudo usermod -a -G dialout pi

# Create udev rules for consistent device names
sudo nano /etc/udev/rules.d/99-usb-serial.rules
```

Add rules:

```
# ESP8266 device
SUBSYSTEM=="tty", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="ea60", SYMLINK+="esp8266"

# 3D Printer (adjust vendor/product IDs as needed)
SUBSYSTEM=="tty", ATTRS{idVendor}=="1d50", ATTRS{idProduct}=="6022", SYMLINK+="3dprinter"
```

Reload rules:

```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

## 📊 Monitoring and Maintenance

### Check System Status

```bash
# Service status
sudo systemctl status ts1-programmer

# View logs
sudo journalctl -u ts1-programmer -f

# Check disk space
df -h

# Check memory usage
free -h

# Check CPU temperature
vcgencmd measure_temp
```

### Update the Application

```bash
cd /home/pi/Programmer
git pull
npm install
sudo systemctl restart ts1-programmer
```

### Backup Configuration

```bash
# Backup config
cp config.json config.json.backup

# Restore config
cp config.json.backup config.json
```

## 🐛 Troubleshooting

### Common Issues

**Service won't start:**
```bash
# Check logs
sudo journalctl -u ts1-programmer -n 50

# Check permissions
ls -la /home/pi/Programmer/

# Test manually
cd /home/pi/Programmer
node server.js
```

**USB devices not found:**
```bash
# List USB devices
lsusb

# Check serial devices
ls -la /dev/tty*

# Check user groups
groups pi
```

**Network access issues:**
```bash
# Check firewall
sudo ufw status

# Allow port 3000
sudo ufw allow 3000

# Check network
ip addr show
```

**Performance issues:**
```bash
# Check system resources
htop

# Increase swap (if needed)
sudo dphys-swapfile swapoff
sudo nano /etc/dphys-swapfile
# Set CONF_SWAPSIZE=1024
sudo dphys-swapfile setup
sudo dphys-swapfile swapon
```

## 🎯 Production Tips

### Security
- Change default password
- Use firewall
- Keep system updated
- Use HTTPS in production

### Reliability
- Use quality power supply
- Monitor temperature
- Regular backups
- UPS for power protection

### Performance
- Use Class 10+ SD card
- Adequate cooling
- Monitor resource usage
- Regular maintenance

## 📞 Support

For Pi-specific issues:
1. Check system logs: `sudo journalctl -u ts1-programmer`
2. Verify hardware connections
3. Test with known good components
4. Check network connectivity

---

**Your TS1 Sensor Programmer is now ready for production use on Raspberry Pi! 🍓** 