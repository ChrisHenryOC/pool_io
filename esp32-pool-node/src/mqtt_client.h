#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

class PoolMQTTClient {
public:
    PoolMQTTClient();
    
    bool initialize();
    bool connect();
    bool isConnected();
    void disconnect();
    void loop(); // Call regularly to maintain connection
    
    // Publishing methods
    bool publishSensorData(const String& topic, const JsonDocument& data);
    bool publishStatus(const String& deviceId, const String& status);
    bool publishGatewayMessage(const JsonDocument& data);
    
    // Subscription methods  
    bool subscribe(const String& topic);
    void setCallback(void (*callback)(char*, uint8_t*, unsigned int));
    
    // Connection management
    bool reconnect();
    String getConnectionStatus();
    
private:
    WiFiClient wifiClient;
    PubSubClient mqttClient;
    
    String clientId;
    unsigned long lastConnectionAttempt;
    int connectionRetries;
    
    bool connectToWiFi();
    String createClientId();
    void onConnectionLost();
};

#endif