#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Adafruit_MAX1704X.h>

// Base class for all pool sensors
class PoolSensor {
public:
    virtual ~PoolSensor() = default;
    
    virtual bool initialize() = 0;
    virtual String getId() const = 0;
    virtual String getType() const = 0;
    virtual String getUnits() const = 0;
    virtual JsonDocument readData() = 0;
    virtual bool isAvailable() const = 0;
    
protected:
    String sensorId;
    bool initialized = false;
};

// Temperature sensor implementation  
class TemperatureSensor : public PoolSensor {
public:
    TemperatureSensor(const String& id, int pin);
    
    bool initialize() override;
    String getId() const override { return sensorId; }
    String getType() const override { return "temperature"; }
    String getUnits() const override { return "fahrenheit"; }
    JsonDocument readData() override;
    bool isAvailable() const override;
    
private:
    int sensorPin;
    void* oneWire;      // OneWire instance
    void* tempSensor;   // DallasTemperature instance
    float lastReading;
    
    float readTemperatureWithRetry(int retries = 3);
    bool validateTemperature(float temp);
};

// Water level (float switch) sensor implementation
class WaterLevelSensor : public PoolSensor {
public:
    WaterLevelSensor(const String& id, int pin1, int pin2);
    
    bool initialize() override;
    String getId() const override { return sensorId; }
    String getType() const override { return "water_level"; }
    String getUnits() const override { return "boolean"; }
    JsonDocument readData() override;
    bool isAvailable() const override;
    
private:
    int switchPin1;
    int switchPin2;
    bool lastLevel;
    
    bool readFloatSwitchAverage(int samples = 10);
};

// Battery monitoring sensor
class BatterySensor : public PoolSensor {
public:
    BatterySensor(const String& id, int adcPin);
    
    bool initialize() override;
    String getId() const override { return sensorId; }
    String getType() const override { return "battery"; }
    String getUnits() const override { return "volts"; }
    JsonDocument readData() override;
    bool isAvailable() const override;
    
private:
    int adcPin;
    float lastVoltage;
    int lastPercentage;
    Adafruit_MAX17048 maxlipo;
    
    float readBatteryVoltage();
    int calculatePercentage(float voltage);
};

#endif