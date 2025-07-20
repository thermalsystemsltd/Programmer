# TS1 Sensor Programmer

An automated ESP8266 programming system with 3D printer integration for mass production of TS1 sensors.

## 🎯 Overview

The TS1 Sensor Programmer is a comprehensive solution for automating the programming of ESP8266-based sensors. It combines:

- **Automated Serial Number Management** via n8n webhooks
- **Arduino CLI Integration** for ESP8266 compilation and upload
- **3D Printer Control** for automated PCB positioning
- **Real-time Web Interface** for monitoring and control
- **Touch-friendly UI** optimized for Raspberry Pi deployment

## ✨ Features

### 🔧 Core Programming
- **Automated Serial Fetching**: Gets next serial number from n8n system
- **Arduino File Updates**: Automatically updates sketch with new serial number
- **ESP8266 Programming**: Compiles and uploads using Arduino CLI
- **Boot Mode Toggle**: Uses esptool to ensure reliable programming
- **Success Notifications**: Sends webhook notifications on successful programming

### 🎮 3D Printer Integration
- **Manual Controls**: Full manual control of X, Y, Z axes
- **Grid Programming**: Automated programming of multiple PCBs in a grid pattern
- **Z-Axis Safety**: Automatic raise/lower for JTAG contact
- **Configurable Positions**: Set exact PCB grid positions and spacing
- **Real-time Position Display**: Live position tracking

### 🖥️ Web Interface
- **Real-time Logging**: Live updates via Server-Sent Events (SSE)
- **Touch-friendly Design**: Optimized for 7-inch Raspberry Pi touch screens
- **Collapsible Configuration**: Clean, organized interface
- **Status Monitoring**: Connection status and position display
- **Responsive Design**: Works on desktop and mobile devices

## 🚀 Quick Start

### Prerequisites
- Node.js 18+ 
- Arduino CLI installed and configured
- ESP8266 board support installed
- 3D printer with G-code support (optional)

### Installation

1. **Clone the repository**
   ```bash
   git clone https://github.com/thermalsystemsltd/Programmer.git
   cd Programmer
   ```

2. **Install dependencies**
   ```bash
   npm install
   ```

3. **Configure the system**
   - Edit `config.json` with your settings
   - Set Arduino sketch path
   - Configure webhook URLs
   - Set COM ports for device and printer

4. **Start the server**
   ```bash
   npm start
   ```

5. **Access the web interface**
   - Open browser to `http://localhost:3000`
   - Configure your settings
   - Start programming!

## ⚙️ Configuration

### Basic Settings (`config.json`)
```json
{
  "ARDUINO_SKETCH_PATH": "path/to/your/sketch.ino",
  "WEBHOOK_URL": "https://n8n.ts1cloud.com/webhook/your-webhook",
  "SUCCESS_WEBHOOK_URL": "https://n8n.ts1cloud.com/webhook/success",
  "COM_PORT": "COM5",
  "BAUD_RATE": "115200"
}
```

### 3D Printer Settings
```json
{
  "PRINTER_COM_PORT": "COM6",
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

## 🎮 Usage

### Manual Programming
1. Click **"START FULL PROCESS"**
2. System will:
   - Fetch next serial number from n8n
   - Update Arduino sketch file
   - Program the connected ESP8266 device
   - Send success notification

### Grid Programming (3D Printer)
1. Configure PCB grid settings
2. Position PCBs in the configured grid
3. Click **"START GRID PROGRAMMING"**
4. System will automatically:
   - Move to each PCB position
   - Lower Z-axis for JTAG contact
   - Program each PCB
   - Raise Z-axis and move to next position

### Manual Printer Control
- Use the manual control buttons to move printer axes
- X/Y movement in 10mm increments
- Z movement in 1mm and 5mm increments
- Home button to return to origin

## 🔧 API Endpoints

### Core Programming
- `POST /api/proxy-webhook` - Fetch serial number from n8n
- `POST /api/update-serial` - Update Arduino sketch with new serial
- `POST /api/program-device` - Program ESP8266 device
- `GET /api/logs` - Server-Sent Events for real-time logs

### 3D Printer Control
- `POST /api/printer/connect` - Connect to 3D printer
- `POST /api/printer/disconnect` - Disconnect from printer
- `POST /api/printer/home` - Home all axes
- `POST /api/printer/move` - Move printer (relative or absolute)
- `POST /api/printer/grid-program` - Start automated grid programming
- `GET /api/printer/status` - Get printer status and position

### Configuration
- `GET /api/config` - Get current configuration
- `POST /api/config` - Update configuration
- `GET /api/connected-devices` - List available COM ports

## 🏗️ Architecture

### Backend (Node.js/Express)
- **server.js**: Main server with API endpoints
- **PrinterController**: 3D printer communication and control
- **Arduino Integration**: CLI commands for compilation and upload
- **Webhook Management**: n8n integration for serial numbers

### Frontend (HTML/CSS/JavaScript)
- **index.html**: Single-page web interface
- **Real-time Updates**: SSE for live logging
- **Touch Interface**: Optimized for small screens
- **Responsive Design**: Works on various devices

## 🔌 Hardware Setup

### ESP8266 Programming
- Connect ESP8266 device to specified COM port
- Ensure Arduino CLI can detect the board
- Verify esptool is available for boot mode control

### 3D Printer Integration
- Connect 3D printer to specified COM port
- Mount JTAG connector on printer head
- Configure PCB grid positions
- Set appropriate Z heights for contact

## 🐛 Troubleshooting

### Common Issues

**SerialPort not available**
```bash
npm install serialport
```

**Arduino CLI not found**
- Install Arduino CLI: https://arduino.github.io/arduino-cli/
- Add to system PATH
- Install ESP8266 board support

**Printer connection fails**
- Check COM port and baud rate
- Ensure printer is powered on
- Verify G-code compatibility

**Programming fails**
- Check device connection
- Verify Arduino sketch path
- Ensure ESP8266 board support is installed

## 📝 Development

### Project Structure
```
Programmer/
├── server.js              # Main server file
├── package.json           # Dependencies and scripts
├── config.json           # Configuration file
├── public/
│   └── index.html        # Web interface
├── .gitignore            # Git ignore rules
└── README.md             # This file
```

### Adding Features
- Backend: Add new endpoints in `server.js`
- Frontend: Update `public/index.html`
- Configuration: Add new fields to `config.json`

## 📄 License

MIT License - see LICENSE file for details

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## 📞 Support

For issues and questions:
- Create an issue on GitHub
- Check the troubleshooting section
- Review the configuration documentation

---

**Built for Thermal Systems Ltd - Automated Sensor Production** 