#include "mqtt_client.h"
#include "config.h"
#include "secrets.h"

PoolMQTTClient::PoolMQTTClient() : mqttClient(wifiClient) {
    clientId = createClientId();
    lastConnectionAttempt = 0;
    connectionRetries = 0;
}

bool PoolMQTTClient::initialize() {
    // Set MQTT server and port
    mqttClient.setServer(MQTT_BROKER_HOST, MQTT_BROKER_PORT);
    mqttClient.setKeepAlive(MQTT_KEEPALIVE);
    mqttClient.setSocketTimeout(10);
    
    // Increase buffer size for larger messages (default is 256 bytes)
    mqttClient.setBufferSize(512);
    
    Serial.printf("MQTT client initialized: %s:%d (buffer: 512 bytes)\n", MQTT_BROKER_HOST, MQTT_BROKER_PORT);
    return true;
}

bool PoolMQTTClient::connect() {
    if (!connectToWiFi()) {
        Serial.println("WiFi connection failed");
        return false;
    }
    
    // Test basic connectivity to server first
    Serial.printf("Testing basic connectivity to %s...\n", MQTT_BROKER_HOST);
    WiFiClient pingClient;
    if (pingClient.connect(MQTT_BROKER_HOST, 80)) {
        Serial.println("Can reach server on port 80: SUCCESS");
        pingClient.stop();
    } else {
        Serial.println("Cannot reach server on port 80: FAILED");
    }
    
    // Test network connectivity to MQTT port
    Serial.printf("Testing network connectivity to %s:%d...\n", MQTT_BROKER_HOST, MQTT_BROKER_PORT);
    WiFiClient testClient;
    if (testClient.connect(MQTT_BROKER_HOST, MQTT_BROKER_PORT)) {
        Serial.println("Network connection to MQTT broker: SUCCESS");
        testClient.stop();
    } else {
        Serial.println("Network connection to MQTT broker: FAILED");
        return false;
    }
    
    // Attempt MQTT connection
    Serial.printf("Attempting MQTT connection to %s...\n", MQTT_BROKER_HOST);
    
    Serial.printf("Using client ID: %s\n", clientId.c_str());
    Serial.printf("Using credentials: %s\n", strlen(MQTT_USERNAME) > 0 ? "YES" : "NO (anonymous)");
    
    bool connected;
    if (strlen(MQTT_USERNAME) > 0) {
        Serial.printf("Connecting with username: %s\n", MQTT_USERNAME);
        connected = mqttClient.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD);
    } else {
        Serial.println("Connecting anonymously...");
        connected = mqttClient.connect(clientId.c_str());
    }
    
    if (connected) {
        Serial.println("MQTT connected successfully");
        connectionRetries = 0;
        
        // Publish connection status
        publishStatus(DEVICE_ID, "online");
        
        return true;
    } else {
        connectionRetries++;
        Serial.printf("MQTT connection failed, rc=%d, retries=%d\n", 
                     mqttClient.state(), connectionRetries);
        return false;
    }
}

bool PoolMQTTClient::isConnected() {
    return mqttClient.connected() && WiFi.status() == WL_CONNECTED;
}

void PoolMQTTClient::disconnect() {
    if (mqttClient.connected()) {
        publishStatus(DEVICE_ID, "offline");
        mqttClient.disconnect();
    }
    WiFi.disconnect();
}

void PoolMQTTClient::loop() {
    if (isConnected()) {
        mqttClient.loop();
    } else {
        // Attempt reconnection if enough time has passed
        unsigned long now = millis();
        if (now - lastConnectionAttempt > 30000) { // Try every 30 seconds
            lastConnectionAttempt = now;
            reconnect();
        }
    }
}

bool PoolMQTTClient::publishSensorData(const String& topic, const JsonDocument& data) {
    if (!isConnected()) {
        Serial.println("MQTT not connected, cannot publish sensor data");
        return false;
    }
    
    String payload;
    serializeJson(data, payload);
    
    Serial.printf("Attempting to publish %d bytes to %s\n", payload.length(), topic.c_str());
    
    bool success = mqttClient.publish(topic.c_str(), payload.c_str(), true); // Retained message
    
    if (success) {
        Serial.printf("Published to %s: %s\n", topic.c_str(), payload.c_str());
    } else {
        Serial.printf("Failed to publish to %s (MQTT state: %d, payload size: %d)\n", 
                     topic.c_str(), mqttClient.state(), payload.length());
    }
    
    return success;
}

bool PoolMQTTClient::publishStatus(const String& deviceId, const String& status) {
    JsonDocument doc;
    doc["device_id"] = deviceId;
    doc["status"] = status;
    doc["timestamp"] = millis();
    doc["firmware_version"] = FIRMWARE_VERSION;
    doc["wifi_rssi"] = WiFi.RSSI();
    doc["free_heap"] = ESP.getFreeHeap();
    
    return publishSensorData(TOPIC_STATUS, doc);
}

bool PoolMQTTClient::publishGatewayMessage(const JsonDocument& data) {
    return publishSensorData(TOPIC_GATEWAY, data);
}

bool PoolMQTTClient::subscribe(const String& topic) {
    if (!isConnected()) {
        return false;
    }
    
    bool success = mqttClient.subscribe(topic.c_str());
    if (success) {
        Serial.printf("Subscribed to topic: %s\n", topic.c_str());
    } else {
        Serial.printf("Failed to subscribe to topic: %s\n", topic.c_str());
    }
    
    return success;
}

void PoolMQTTClient::setCallback(void (*callback)(char*, uint8_t*, unsigned int)) {
    mqttClient.setCallback(callback);
}

bool PoolMQTTClient::reconnect() {
    Serial.println("Attempting MQTT reconnection...");
    return connect();
}

String PoolMQTTClient::getConnectionStatus() {
    if (!WiFi.isConnected()) {
        return "WiFi disconnected";
    }
    
    if (!mqttClient.connected()) {
        switch (mqttClient.state()) {
            case MQTT_CONNECTION_TIMEOUT:
                return "MQTT connection timeout";
            case MQTT_CONNECTION_LOST:
                return "MQTT connection lost";
            case MQTT_CONNECT_FAILED:
                return "MQTT connect failed";
            case MQTT_DISCONNECTED:
                return "MQTT disconnected";
            case MQTT_CONNECT_BAD_PROTOCOL:
                return "MQTT bad protocol";
            case MQTT_CONNECT_BAD_CLIENT_ID:
                return "MQTT bad client ID";
            case MQTT_CONNECT_UNAVAILABLE:
                return "MQTT server unavailable";
            case MQTT_CONNECT_BAD_CREDENTIALS:
                return "MQTT bad credentials";
            case MQTT_CONNECT_UNAUTHORIZED:
                return "MQTT unauthorized";
            default:
                return "MQTT unknown error";
        }
    }
    
    return "Connected";
}

bool PoolMQTTClient::connectToWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        return true;
    }
    
    WiFi.mode(WIFI_STA);
    WiFi.setHostname("pool-node-001");
    
    // Try each network in the list
    for (int i = 0; WIFI_NETWORKS[i][0] != nullptr; i++) {
        const char* ssid = WIFI_NETWORKS[i][0];
        const char* password = WIFI_NETWORKS[i][1];
        
        Serial.printf("Attempting WiFi connection to %s...\n", ssid);
        WiFi.begin(ssid, password);
        
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && 
               millis() - startTime < WIFI_TIMEOUT_MS) {
            delay(500);
            Serial.print(".");
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("\nWiFi connected to %s\n", ssid);
            Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("Signal strength: %d dBm\n", WiFi.RSSI());
            return true;
        }
        
        Serial.printf("\nFailed to connect to %s\n", ssid);
        WiFi.disconnect();
        delay(1000);
    }
    
    Serial.println("Failed to connect to any WiFi network");
    return false;
}

String PoolMQTTClient::createClientId() {
    String id = String(MQTT_CLIENT_ID) + "-" + String(random(0xffff), HEX);
    return id;
}