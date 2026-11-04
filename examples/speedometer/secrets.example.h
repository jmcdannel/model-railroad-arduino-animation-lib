// 📋 Copy to secrets.h and fill in. secrets.h is git-ignored: keep it out of any repo.
// Only used when USE_MQTT is true (ESP32).
#pragma once

#define WIFI_SSID     "your-wifi"
#define WIFI_PASSWORD "your-password"

// The DEJA server (it runs an MQTT broker on port 1883), or any other broker.
#define MQTT_HOST     "192.168.1.100"
#define MQTT_PORT     1883
#define MQTT_USERNAME ""   // only if the broker needs a login
#define MQTT_PASSWORD ""
