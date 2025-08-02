#include "sensors.h"
#include "config.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_MAX1704X.h>
#include <Wire.h>

// TemperatureSensor Implementation
TemperatureSensor::TemperatureSensor(const String& id, int pin) 
    : sensorPin(pin), lastReading(-999.0) {
    sensorId = id;
    oneWire = nullptr;
    tempSensor = nullptr;
}

bool TemperatureSensor::initialize() {
    oneWire = new OneWire(sensorPin);
    tempSensor = new DallasTemperature((OneWire*)oneWire);
    
    ((DallasTemperature*)tempSensor)->begin();
    ((DallasTemperature*)tempSensor)->setResolution(TEMPERATURE_PRECISION);
    
    // Test sensor availability
    ((DallasTemperature*)tempSensor)->requestTemperatures();
    delay(1000);
    
    float testTemp = ((DallasTemperature*)tempSensor)->getTempFByIndex(0);
    initialized = validateTemperature(testTemp);
    
    Serial.printf("Temperature sensor %s initialized: %s\n", 
                 sensorId.c_str(), initialized ? "OK" : "FAILED");
    
    return initialized;
}

JsonDocument TemperatureSensor::readData() {
    JsonDocument doc;
    
    if (!initialized) {
        doc["error"] = "Sensor not initialized";
        return doc;
    }
    
    float temperature = readTemperatureWithRetry();
    
    doc["sensor_id"] = sensorId;
    doc["sensor_type"] = getType();
    doc["timestamp"] = millis();
    doc["units"] = getUnits();
    
    if (validateTemperature(temperature)) {
        doc["value"] = temperature;
        doc["quality"] = "good";
        lastReading = temperature;
    } else {
        doc["value"] = lastReading;
        doc["quality"] = "questionable";
        doc["error"] = "Invalid reading, using last known value";
    }
    
    return doc;
}

bool TemperatureSensor::isAvailable() const {
    return initialized && tempSensor != nullptr;
}

float TemperatureSensor::readTemperatureWithRetry(int retries) {
    for (int i = 0; i < retries; i++) {
        ((DallasTemperature*)tempSensor)->requestTemperatures();
        delay(750); // Wait for conversion
        
        float temp = ((DallasTemperature*)tempSensor)->getTempFByIndex(0);
        
        if (validateTemperature(temp)) {
            return temp;
        }
        
        Serial.printf("Temperature read attempt %d failed: %.2f\n", i + 1, temp);
        delay(1000);
    }
    
    return -999.0; // Error value
}

bool TemperatureSensor::validateTemperature(float temp) {
    return (temp > -50.0 && temp < 150.0 && temp != -196.6 && temp != 185.0);
}

// WaterLevelSensor Implementation
WaterLevelSensor::WaterLevelSensor(const String& id, int pin1, int pin2) 
    : switchPin1(pin1), switchPin2(pin2), lastLevel(false) {
    sensorId = id;
}

bool WaterLevelSensor::initialize() {
    pinMode(switchPin1, INPUT_PULLUP);
    pinMode(switchPin2, INPUT_PULLUP);
    
    // Test pins are responsive
    int test1 = digitalRead(switchPin1);
    int test2 = digitalRead(switchPin2);
    
    initialized = true; // Float switches are simple digital inputs
    
    Serial.printf("Water level sensor %s initialized: pins %d,%d\n", 
                 sensorId.c_str(), switchPin1, switchPin2);
    
    return initialized;
}

JsonDocument WaterLevelSensor::readData() {
    JsonDocument doc;
    
    doc["sensor_id"] = sensorId;
    doc["sensor_type"] = getType();
    doc["timestamp"] = millis();
    doc["units"] = getUnits();
    
    if (!initialized) {
        doc["error"] = "Sensor not initialized";
        return doc;
    }
    
    bool level = readFloatSwitchAverage();
    
    doc["value"] = level;
    doc["quality"] = "good";
    doc["raw_pin1"] = digitalRead(switchPin1);
    doc["raw_pin2"] = digitalRead(switchPin2);
    
    lastLevel = level;
    
    return doc;
}

bool WaterLevelSensor::isAvailable() const {
    return initialized;
}

bool WaterLevelSensor::readFloatSwitchAverage(int samples) {
    // Implement cumulative debouncing like Python version
    int accumulator = 0;
    
    for (int i = 0; i < samples; i++) {
        // Logic: if either pin is LOW, water level is adequate  
        bool pin1State = digitalRead(switchPin1) == LOW;
        bool pin2State = digitalRead(switchPin2) == LOW;
        bool waterOK = pin1State || pin2State;
        
        accumulator += waterOK ? 1 : 0;
        
        // Blink LED for feedback (like Python version)
        if (i < 3) {
            digitalWrite(LED_PIN, HIGH);
            delay(25);
            digitalWrite(LED_PIN, LOW);
            delay(25);
        }
        
        // Feed watchdog every 10 readings (watchdog is managed in main loop)
        // esp_task_wdt_reset(); // Not available in this context
        
        delay(100); // 100ms delay between readings for 10+ second total
    }
    
    // Return true ONLY if ALL readings are consistent (like Python version)
    // If accumulator == samples, all readings were consistent
    return (accumulator == samples);
}

// BatterySensor Implementation
BatterySensor::BatterySensor(const String& id, int pin) 
    : adcPin(pin), lastVoltage(0.0), lastPercentage(0) {
    sensorId = id;
}

bool BatterySensor::initialize() {
    // Initialize I2C for ESP32-S3 Feather (SDA=3, SCL=4)
    Wire.begin(3, 4);
    
    // Initialize MAX17048 battery monitor
    if (!maxlipo.begin()) {
        Serial.printf("Battery sensor %s: Could not find MAX17048! Check battery connection.\n", sensorId.c_str());
        Serial.println("Trying I2C scan...");
        
        // I2C scanner to debug
        for (byte address = 1; address < 127; address++) {
            Wire.beginTransmission(address);
            if (Wire.endTransmission() == 0) {
                Serial.printf("I2C device found at address 0x%02X\n", address);
            }
        }
        
        initialized = false;
        return false;
    }
    
    Serial.printf("Battery sensor %s initialized: MAX17048 with Chip ID: 0x%X\n", 
                 sensorId.c_str(), maxlipo.getChipID());
    
    initialized = true;
    return true;
}

JsonDocument BatterySensor::readData() {
    JsonDocument doc;
    
    doc["sensor_id"] = sensorId;
    doc["sensor_type"] = getType();
    doc["timestamp"] = millis();
    doc["units"] = getUnits();
    
    if (!initialized) {
        doc["error"] = "Sensor not initialized";
        return doc;
    }
    
    float voltage = readBatteryVoltage();
    int percentage = calculatePercentage(voltage);
    
    doc["value"] = voltage;
    doc["percentage"] = percentage;
    doc["quality"] = "good";
    
    // Add battery status
    if (voltage < CRITICAL_BATTERY_THRESHOLD) {
        doc["status"] = "critical";
    } else if (voltage < LOW_BATTERY_THRESHOLD) {
        doc["status"] = "low";
    } else {
        doc["status"] = "good";
    }
    
    lastVoltage = voltage;
    lastPercentage = percentage;
    
    return doc;
}

bool BatterySensor::isAvailable() const {
    return initialized;
}

float BatterySensor::readBatteryVoltage() {
    // Read battery voltage using MAX17048
    float cellVoltage = maxlipo.cellVoltage();
    
    if (isnan(cellVoltage)) {
        Serial.println("Failed to read cell voltage, check battery is connected!");
        return 0.0;
    }
    
    return cellVoltage;
}

int BatterySensor::calculatePercentage(float voltage) {
    // Use MAX17048's built-in battery percentage calculation
    float cellPercent = maxlipo.cellPercent();
    
    if (isnan(cellPercent)) {
        Serial.println("Failed to read cell percentage!");
        return 0;
    }
    
    return (int)cellPercent;
}