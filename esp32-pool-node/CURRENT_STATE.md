# ESP32-S3 Pool Node - Session Resume Guide

## Quick Status Check

**Last Known Working State**: Pool monitoring system successfully converted from CircuitPython to C++ and operational.

### What's Working ✅
- ESP32-S3 Feather hardware setup
- WiFi connection to configured network  
- MQTT publishing to YOUR_HUB_IP:1883
- Temperature sensor (DS18X20 on GPIO 10)
- Water level sensor with 10-second debouncing (GPIO 11,12)
- Gateway message publishing (307 bytes)
- Testing mode (20-second sensor cycles, no deep sleep)

### What Needs Attention ⚠️
- Battery sensor (MAX17048 not detected on I2C)
- Deep sleep disabled for testing

## Quick Commands to Verify System

```bash
# Check if code compiles and uploads
cd /Users/chrishenry/source/pool_io/esp32-pool-node
pio run --target upload --target monitor

# Expected output every 20 seconds:
# "Published to poolio/temperature: ..."
# "Published to poolio/water_level: ..."  
# "Published to poolio/gateway: ..."
```

## Key Configuration Files

### secrets.h (WiFi credentials)
```cpp
const char* WIFI_NETWORKS[][2] = {
    {"YOUR_NETWORK_1", "your_password_1"},
    {"YOUR_NETWORK_2", "your_password_2"},
    {nullptr, nullptr}
};
```

### config.h (Pin assignments)
```cpp
#define TEMP_SENSOR_PIN 10
#define FLOAT_SWITCH_PIN_1 11  
#define FLOAT_SWITCH_PIN_2 12
#define LED_PIN LED_BUILTIN     // GPIO 13
#define MQTT_BROKER_HOST "YOUR_HUB_IP"
```

## Recent Changes Made

1. **Fixed router WiFi isolation** - ESP32 can now reach Ubuntu server
2. **Increased MQTT buffer** from 256 to 512 bytes for gateway messages
3. **Implemented proper float switch debouncing** - 100 samples over 10+ seconds
4. **Added I2C initialization** for MAX17048 battery monitor (still not working)
5. **Disabled deep sleep** for easier testing
6. **Enhanced debugging** with detailed MQTT publish logging

## Outstanding Issues

### Battery Monitor Issue
```
Battery sensor battery_01: Could not find MAX17048! Check battery connection.
Trying I2C scan...
```

**Tried Solutions**:
- I2C pins SDA=3, SCL=4
- Added Adafruit_MAX1704X library
- I2C scanner implementation

**Next to Try**:
1. Default I2C pins (SDA=21, SCL=22)
2. ADC fallback reading
3. Verify chip exists on this board variant

### Return to Production Mode
When ready, change in main.cpp:
```cpp
// Testing mode (current):
if (systemInitialized && mqttClient.isConnected() && (now - lastSensorRead >= 20000)) {

// Production mode (change to):
if (systemInitialized && mqttClient.isConnected() && lastSensorRead == 0) {
    // ... then add enterDeepSleep() call
```

## MQTT Data Being Published

**Temperature**: Every 20 seconds, ~78°F  
**Water Level**: Every 20 seconds, false (no water detected)  
**Gateway**: Combined status, 304 bytes  
**Status**: Online/sleeping/offline states  

## Network Details

- **ESP32 IP**: 192.168.68.108
- **Ubuntu Server**: YOUR_HUB_IP  
- **MQTT Broker**: Docker container (poolio-mosquitto)
- **WiFi Signal**: -66 to -70 dBm (good)

## If Starting Fresh Session

1. Verify hardware is connected and powered
2. Run `pio run --target upload --target monitor`  
3. Look for "MQTT connected successfully" and regular sensor publishing
4. If battery sensor fails, it's expected (known issue)
5. Data should appear in InfluxDB on Ubuntu server

## Integration Status

**CircuitPython → C++ Migration**: ✅ **COMPLETE**

The ESP32-S3 implementation successfully replaced the CircuitPython version and is publishing sensor data to the local MQTT broker, which feeds into InfluxDB for monitoring and alerting.