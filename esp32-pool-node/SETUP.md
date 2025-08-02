# ESP32 Pool Node - Development Environment Setup

## 🛠️ Quick Start (Recommended: VS Code + PlatformIO)

### Prerequisites
- ESP32-S3 development board
- USB cable for programming
- VS Code installed

### 1. Install PlatformIO Extension
1. Open VS Code
2. Extensions tab (Cmd+Shift+X or Ctrl+Shift+X)
3. Search for "PlatformIO IDE"
4. Install the official PlatformIO extension by PlatformIO
5. **Restart VS Code** (important!)

### 2. Open Project
```bash
cd /path/to/pool_io/esp32-pool-node
code .
```

### 3. Configure Credentials
```bash
# Copy template file
cp include/secrets-example.h include/secrets.h

# Edit secrets.h with your actual WiFi credentials
# Update WIFI_NETWORKS array with your network names and passwords
```

### 4. Connect Hardware
- Connect ESP32-S3 via USB
- Note the port (usually `/dev/cu.usbmodem*` on macOS)

### 5. Build and Upload
**PlatformIO toolbar appears at bottom of VS Code:**
- **✓ Build** - Compile the project
- **→ Upload** - Flash to ESP32
- **🔌 Serial Monitor** - View debug output
- **🗑️ Clean** - Clean build files

### 6. First Upload Process
1. Click **✓ Build** to compile
2. Click **→ Upload** to flash firmware
3. Click **🔌 Serial Monitor** to watch boot sequence
4. Baud rate should auto-detect to 115200

---

## 🔧 PlatformIO Commands (Terminal)

If you prefer command line:
```bash
# Build project
pio run

# Upload to device
pio run --target upload

# Open serial monitor
pio device monitor

# Clean build
pio run --target clean

# List connected devices
pio device list
```

---

## 📋 Development Workflow

### Daily Development
1. **Open project**: `code .` in the esp32-pool-node directory
2. **Make changes** to source files
3. **Build** (✓) to check for errors
4. **Upload** (→) to test on hardware
5. **Monitor** (🔌) to view serial debug output

### Key Files to Edit
- `src/main.cpp` - Main application logic
- `src/sensors.cpp` - Sensor implementations
- `include/config.h` - Pin assignments and settings
- `include/secrets.h` - WiFi credentials (git-ignored)

### Adding New Sensors
1. Add new class to `src/sensors.h`
2. Implement in `src/sensors.cpp`
3. Include in `main.cpp` setup and loop functions
4. Build and test

---

## 🐛 Troubleshooting

### PlatformIO Not Working
- **Restart VS Code** after installing extension
- Check VS Code bottom toolbar for PlatformIO icons
- Try reopening the folder: File → Open Folder

### Upload Fails
- Check ESP32 is connected and recognized
- Try different USB cable
- Press and hold BOOT button during upload if needed
- Check port selection in PlatformIO

### Build Errors
- Verify all libraries installed (PlatformIO handles this automatically)
- Check `include/secrets.h` exists and is properly formatted
- Clean build: 🗑️ Clean, then ✓ Build

### Serial Monitor Issues
- Ensure baud rate is 115200
- Close other serial applications
- Try unplugging/replugging ESP32

### WiFi Connection Issues
- Verify credentials in `secrets.h`
- Check WiFi network is 2.4GHz (ESP32 doesn't support 5GHz)
- Ensure hub `poolio-hub.local` is accessible on network

---

## 🏗️ Alternative: Arduino IDE Setup

If you prefer Arduino IDE over VS Code:

### Install Arduino IDE 2.x
Download from: https://www.arduino.cc/en/software

### Add ESP32 Board Support
1. File → Preferences
2. Additional Board Manager URLs: 
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
3. Tools → Board → Board Manager
4. Search "ESP32" and install "esp32 by Espressif Systems"

### Install Libraries
Tools → Manage Libraries, install:
- **PubSubClient** by Nick O'Leary
- **ArduinoJson** by Benoit Blanchon  
- **OneWire** by Jim Studt
- **DallasTemperature** by Miles Burton

### Board Configuration
- **Board**: ESP32S3 Dev Module
- **USB CDC On Boot**: Enabled
- **CPU Frequency**: 240MHz
- **Flash Size**: 4MB (or your board's size)
- **Partition Scheme**: Default 4MB
- **Upload Speed**: 921600

### Arduino IDE Workflow
1. Copy `src/main.cpp` content to new sketch
2. Create tabs for `sensors.h`, `sensors.cpp`, etc.
3. Configure board settings above
4. Upload and monitor via Serial Monitor

---

## 📡 Testing Hub Connection

### Verify Hub is Running
```bash
# Check hub services are running
docker ps

# Test MQTT broker
mosquitto_pub -h poolio-hub.local -t test -m "hello"
```

### Monitor MQTT Traffic
```bash
# Subscribe to all pool topics
mosquitto_sub -h poolio-hub.local -t "poolio/#" -v
```

### Expected Boot Sequence
```
=== PoolIO ESP32 Node Starting ===
Device ID: pool-node-001
Firmware: 1.0.0
Initializing sensors...
Temperature sensor temp_01 initialized: OK
Water level sensor water_level_01 initialized: pins 11,12
Battery sensor battery_01 initialized: ADC pin 0
Setting up MQTT connection...
WiFi connected to YourNetwork
IP address: 192.168.1.XXX
MQTT connected successfully
=== System initialization complete ===
```

---

## 🚀 Next Steps After Setup

1. **Verify sensors** are reading correctly via serial monitor
2. **Check MQTT messages** are reaching the hub
3. **Test deep sleep cycle** (device sleeps after 60 seconds)
4. **Monitor hub dashboard** for incoming data
5. **Compare with legacy CircuitPython node** for consistency

## 📝 Development Notes

- **PlatformIO** handles library dependencies automatically
- **Serial output** at 115200 baud shows detailed boot and sensor info
- **MQTT topics** follow the pattern `poolio/{sensor_type}`
- **Deep sleep** activates after 60 seconds of operation
- **Watchdog timer** prevents hangs (45 second timeout)
- **Configuration** can be updated via MQTT `poolio/config` topic

---

## 🔄 Switching Between Legacy and New Node

Both nodes can run simultaneously for comparison:
- **Legacy node**: Continues using Adafruit IO
- **New node**: Uses local hub via MQTT
- **Monitor both** to verify data consistency
- **Gradually transition** when confident in new implementation