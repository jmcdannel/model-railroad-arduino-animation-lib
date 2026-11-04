// 📋 Copy to secrets.h and fill in. secrets.h is git-ignored: keep it out of any repo.
#pragma once

#define WIFI_SSID     "your-wifi"
#define WIFI_PASSWORD "your-password"

// The machine running the DEJA server (`deja start`): its LAN IP or hostname.
// The server runs an MQTT broker on port 1883.
#define MQTT_HOST     "192.168.1.100"
#define MQTT_PORT     1883
#define MQTT_USERNAME ""   // only if the broker was given MQTT_USERNAME / MQTT_PASSWORD
#define MQTT_PASSWORD ""
