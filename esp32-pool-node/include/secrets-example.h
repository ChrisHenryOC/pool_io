#ifndef SECRETS_H
#define SECRETS_H

// WiFi credentials - update with your networks
// Copy this file to secrets.h and update with your actual credentials
const char* WIFI_NETWORKS[][2] = {
    {"YourHomeWiFi", "your-wifi-password"},
    {"YourBackupWiFi", "backup-wifi-password"},
    {"YourMobileHotspot", "hotspot-password"},
    {nullptr, nullptr}  // Terminator - do not remove
};

// MQTT credentials (if authentication is enabled on your hub)
// Leave empty strings if your MQTT broker doesn't require authentication
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""

// OTA update password (for future secure firmware updates)
// Choose a strong password for over-the-air updates
#define OTA_PASSWORD "change-this-strong-password"

#endif