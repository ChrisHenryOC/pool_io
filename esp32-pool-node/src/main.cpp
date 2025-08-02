#include <Arduino.h>
#include <esp_sleep.h>
#include <esp_task_wdt.h>
#include "config.h"
#include "sensors.h"
#include "mqtt_client.h"

// Global objects
PoolMQTTClient mqttClient;
TemperatureSensor* tempSensor;
WaterLevelSensor* waterLevelSensor;
BatterySensor* batterySensor;

// System state
unsigned long lastSensorRead = 0;
unsigned long sleepDuration = DEFAULT_SLEEP_DURATION_S;
bool systemInitialized = false;

// Function declarations
void setupSensors();
void setupMQTT();
void readAndPublishSensors();
void publishGatewayMessage();
void enterDeepSleep();
void onMQTTMessage(char* topic, uint8_t* payload, unsigned int length);
void setupWatchdog();
void blinkLED(int times, int delayMs = 200);

void setup() {
    // Initialize LED first for visual feedback
    pinMode(LED_PIN, OUTPUT);
    
    // Blink rapidly to show code is running
    for(int i = 0; i < 10; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);
    }
    
    Serial.begin(115200);
    delay(3000); // Give more time for serial to initialize
    
    Serial.println("\n=== PoolIO ESP32-S3 Node Starting ===");
    Serial.printf("Device ID: %s\n", DEVICE_ID);
    Serial.printf("Firmware: %s\n", FIRMWARE_VERSION);
    Serial.printf("LED Pin: %d\n", LED_PIN);
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    
    blinkLED(3, 500); // Slower startup indicator
    
    // Setup watchdog timer
    setupWatchdog();
    
    // Initialize sensors
    setupSensors();
    
    // Initialize MQTT
    setupMQTT();
    
    // Subscribe to configuration topic
    mqttClient.subscribe(TOPIC_CONFIG);
    
    systemInitialized = true;
    Serial.println("=== System initialization complete ===\n");
}

void loop() {
    // Feed watchdog
    esp_task_wdt_reset();
    
    // Heartbeat LED blink every 5 seconds
    static unsigned long lastHeartbeat = 0;
    unsigned long now = millis();
    if (now - lastHeartbeat >= 5000) {
        lastHeartbeat = now;
        Serial.printf("Heartbeat: %lu ms, Free heap: %d\n", now, ESP.getFreeHeap());
        blinkLED(1, 50); // Quick heartbeat blink
    }
    
    // Maintain MQTT connection
    mqttClient.loop();
    
    // Read and publish sensor data every 20 seconds for testing
    if (systemInitialized && mqttClient.isConnected() && (now - lastSensorRead >= 20000)) {
        lastSensorRead = now;
        readAndPublishSensors();
        
        Serial.println("Waiting 20 seconds before next sensor reading...");
    }
    
    delay(1000);
}

void setupSensors() {
    Serial.println("Initializing sensors...");
    
    // Initialize temperature sensor
    tempSensor = new TemperatureSensor("temp_01", TEMP_SENSOR_PIN);
    if (!tempSensor->initialize()) {
        Serial.println("WARNING: Temperature sensor initialization failed");
    }
    
    // Initialize water level sensor
    waterLevelSensor = new WaterLevelSensor("water_level_01", 
                                           FLOAT_SWITCH_PIN_1, 
                                           FLOAT_SWITCH_PIN_2);
    if (!waterLevelSensor->initialize()) {
        Serial.println("WARNING: Water level sensor initialization failed");
    }
    
    // Initialize battery sensor
    batterySensor = new BatterySensor("battery_01", BATTERY_ADC_PIN);
    if (!batterySensor->initialize()) {
        Serial.println("WARNING: Battery sensor initialization failed");
    }
    
    Serial.println("Sensor initialization complete");
}

void setupMQTT() {
    Serial.println("Setting up MQTT connection...");
    
    mqttClient.initialize();
    mqttClient.setCallback(onMQTTMessage);
    
    // Attempt connection
    int attempts = 0;
    while (!mqttClient.connect() && attempts < 5) {
        attempts++;
        Serial.printf("MQTT connection attempt %d failed, retrying...\n", attempts);
        delay(5000);
    }
    
    if (!mqttClient.isConnected()) {
        Serial.println("Failed to establish MQTT connection, continuing anyway...");
    } else {
        Serial.println("MQTT connection established");
    }
}

void readAndPublishSensors() {
    Serial.println("Reading sensors...");
    blinkLED(1, 100);
    
    // Read temperature
    if (tempSensor && tempSensor->isAvailable()) {
        JsonDocument tempData = tempSensor->readData();
        mqttClient.publishSensorData(TOPIC_TEMPERATURE, tempData);
    }
    
    // Read water level
    if (waterLevelSensor && waterLevelSensor->isAvailable()) {
        JsonDocument levelData = waterLevelSensor->readData();
        mqttClient.publishSensorData("poolio/water_level", levelData);
    }
    
    // Read battery
    if (batterySensor && batterySensor->isAvailable()) {
        JsonDocument batteryData = batterySensor->readData();
        mqttClient.publishSensorData(TOPIC_BATTERY, batteryData);
    }
    
    // Publish gateway message (combined data)
    publishGatewayMessage();
    
    Serial.println("Sensor reading complete");
}

void publishGatewayMessage() {
    JsonDocument gatewayMsg;
    
    // Device information
    gatewayMsg["device_id"] = DEVICE_ID;
    gatewayMsg["device_type"] = DEVICE_TYPE;
    gatewayMsg["timestamp"] = millis();
    gatewayMsg["firmware_version"] = FIRMWARE_VERSION;
    
    // System status  
    gatewayMsg["uptime_ms"] = millis();
    gatewayMsg["free_heap"] = ESP.getFreeHeap();
    
    // Safely get WiFi RSSI
    if (WiFi.status() == WL_CONNECTED) {
        gatewayMsg["wifi_rssi"] = WiFi.RSSI();
    } else {
        gatewayMsg["wifi_rssi"] = -99;
    }
    
    gatewayMsg["connection_status"] = mqttClient.getConnectionStatus();
    
    // Sensor availability
    JsonObject sensors = gatewayMsg["sensors"].to<JsonObject>();
    sensors["temperature_available"] = tempSensor ? tempSensor->isAvailable() : false;
    sensors["water_level_available"] = waterLevelSensor ? waterLevelSensor->isAvailable() : false;
    sensors["battery_available"] = batterySensor ? batterySensor->isAvailable() : false;
    
    // Quick sensor readings (for gateway summary)
    if (tempSensor && tempSensor->isAvailable()) {
        JsonDocument tempData = tempSensor->readData();
        if (tempData["value"].is<float>()) {
            gatewayMsg["temperature_f"] = tempData["value"];
        }
    }
    
    
    if (batterySensor && batterySensor->isAvailable()) {
        JsonDocument batteryData = batterySensor->readData();
        if (batteryData["value"].is<float>()) {
            gatewayMsg["battery_voltage"] = batteryData["value"];
        }
        if (batteryData["percentage"].is<float>()) {
            gatewayMsg["battery_percentage"] = batteryData["percentage"];
        }
    }
    
    // Debug gateway message before publishing
    String gatewayDebug;
    serializeJson(gatewayMsg, gatewayDebug);
    Serial.printf("Gateway message size: %d bytes\n", gatewayDebug.length());
    Serial.printf("Gateway JSON: %s\n", gatewayDebug.c_str());
    
    mqttClient.publishGatewayMessage(gatewayMsg);
}

void enterDeepSleep() {
    // Publish offline status
    mqttClient.publishStatus(DEVICE_ID, "sleeping");
    delay(1000);
    
    // Disconnect cleanly
    mqttClient.disconnect();
    
    // Configure wake-up timer
    esp_sleep_enable_timer_wakeup(sleepDuration * 1000000ULL); // Convert to microseconds
    
    Serial.printf("Entering deep sleep for %lu seconds\n", sleepDuration);
    Serial.flush();
    
    // Enter deep sleep
    esp_deep_sleep_start();
}

void onMQTTMessage(char* topic, uint8_t* payload, unsigned int length) {
    // Convert payload to string
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    Serial.printf("MQTT message received on %s: %s\n", topic, message.c_str());
    
    // Handle configuration updates
    if (String(topic) == TOPIC_CONFIG) {
        JsonDocument config;
        DeserializationError error = deserializeJson(config, message);
        
        if (!error) {
            if (config["sleep_duration"].is<unsigned long>()) {
                sleepDuration = config["sleep_duration"];
                Serial.printf("Updated sleep duration to %lu seconds\n", sleepDuration);
            }
        } else {
            Serial.println("Failed to parse configuration JSON");
        }
    }
}

void setupWatchdog() {
    esp_task_wdt_init(WATCHDOG_TIMEOUT_S, true);
    esp_task_wdt_add(NULL);
    Serial.printf("Watchdog timer configured: %d seconds\n", WATCHDOG_TIMEOUT_S);
}

void blinkLED(int times, int delayMs) {
    for (int i = 0; i < times; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(delayMs);
        digitalWrite(LED_PIN, LOW);
        if (i < times - 1) delay(delayMs);
    }
}