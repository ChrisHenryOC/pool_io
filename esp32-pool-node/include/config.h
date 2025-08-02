#ifndef CONFIG_H
#define CONFIG_H

// Pin definitions for Adafruit ESP32-S3 Feather
#define TEMP_SENSOR_PIN 10      // DS18X20 temperature sensor (D10)
#define FLOAT_SWITCH_PIN_1 11   // Float switch input 1 (D11)
#define FLOAT_SWITCH_PIN_2 12   // Float switch input 2 (D12)
#define LED_PIN LED_BUILTIN     // Built-in LED
#define BATTERY_ADC_PIN A13     // Battery voltage monitoring (GPIO2/A13)

// Network configuration
#define WIFI_TIMEOUT_MS 30000
#define MQTT_TIMEOUT_MS 10000
#define MQTT_KEEPALIVE 60

// Hub configuration (your Ubuntu server)
#define MQTT_BROKER_HOST "192.168.68.120"
#define MQTT_BROKER_PORT 1883
#define MQTT_CLIENT_ID "pool-node-esp32"

// MQTT Topics
#define TOPIC_GATEWAY "poolio/gateway"
#define TOPIC_TEMPERATURE "poolio/temperature" 
#define TOPIC_BATTERY "poolio/battery"
#define TOPIC_CONFIG "poolio/config"
#define TOPIC_STATUS "poolio/status"

// Sleep and timing configuration
#define DEFAULT_SLEEP_DURATION_S 300  // 5 minutes
#define SENSOR_READ_RETRIES 3
#define FLOAT_SWITCH_SAMPLES 100  // 100 samples * 100ms = 10 seconds
#define TEMPERATURE_PRECISION 12

// Power management
#define LOW_BATTERY_THRESHOLD 3.3
#define CRITICAL_BATTERY_THRESHOLD 3.0
#define WATCHDOG_TIMEOUT_S 45

// Device identification
#define DEVICE_ID "pool-node-001"
#define FIRMWARE_VERSION "1.0.0"
#define DEVICE_TYPE "pool-sensor"

#endif