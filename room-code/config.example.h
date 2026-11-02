// 📋 Copy to config.h and fill in. config.h is yours — keep it out of any repo.
#pragma once

// ---------------------------------------------------------------- WiFi 🛜
#define WIFI_SSID       "your-wifi"
#define WIFI_PASSWORD   "your-password"

// ------------------------------------------------- DEJA server (MQTT) 📡
// The machine running `deja start` for this layout — its LAN IP or hostname.
#define MQTT_HOST       "192.168.1.100"
#define MQTT_PORT       1883
// Only if the broker was given MQTT_USERNAME / MQTT_PASSWORD; leave empty otherwise.
#define MQTT_USERNAME   ""
#define MQTT_PASSWORD   ""

// ------------------------------------------------------------- Layout 🚂
#define LAYOUT_ID       "tam"
#define LAYOUT_NAME     "TAMARACK JUNCTION"   // the small title across the top

// ------------------------------------------ OLED build only (-DDISPLAY_SSD1306) 🔲
// ESP32 DevKitC + 0.96" SSD1306: SDA → P21, SCL → P22, VCC → 3V3, GND → GND.
#define I2C_SDA         21
#define I2C_SCL         22
#define OLED_ADDR       0x3C      // some modules answer at 0x3D
#define OLED_ROTATION   0         // 2 = upside down
#define STATUS_LED_PIN  2         // DevKitC clones' blue LED; -1 if the board has none
