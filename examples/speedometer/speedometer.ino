/*
 * 🚄 Scale Speedometer — Arduino Nano or ESP32
 * --------------------------------------------
 * Two IR sensors a known distance apart time a train between them and work
 * out its scale speed. Either direction works: whichever sensor the train
 * hits first starts the clock.
 *
 * Every setting is in this file, top to bottom:
 *   1. Sensors + distance   2. Scale + units   3. Display
 *   4. Serial output        5. MQTT (ESP32 only)
 * Change what you need, ignore the rest. Everything below "the sketch"
 * is plain code you're welcome to edit.
 *
 * Wiring (Nano):
 *   D2 ← IR sensor A OUT   (FC-51 / TCRT5000 module: VCC→5V, GND→GND)
 *   D3 ← IR sensor B OUT   D2 and D3 are the Nano's interrupt pins, so the
 *                          timing is exact even while the display updates.
 *   TM1637 display: CLK → D4, DIO → D5, VCC → 5V, GND → GND
 *   OLED (SSD1306 I2C): SDA → A4, SCL → A5, VCC → 5V, GND → GND
 * On an ESP32 use input pins with pull-ups (e.g. 32 and 33; not 34–39)
 * and SDA 21 / SCL 22. The ESP32 is 3.3 V: power the IR modules from 3V3.
 */

// ===================================================== 1. Sensors 📏
#define SENSOR_A_PIN      2
#define SENSOR_B_PIN      3
#define SENSOR_ACTIVE_LOW true      // the cheap IR modules pull OUT low on a train
// Measure centre to centre between the two sensors, as exactly as you can:
// 1 mm off on a 300 mm gap is 0.3% off on the speed.
const float SENSOR_GAP_MM = 300.0;

const uint32_t GIVE_UP_MS = 15000;  // second sensor never tripped (train stopped or backed out)
const uint32_t REARM_MS   = 2000;   // both sensors clear this long before the next train
                                    // (longer than the gap between two cars)

// ================================================ 2. Scale + units 📐
// Z 220 · N 160 (UK 148, Japan 150) · TT 120 · HO 87.1 · OO 76.2
// S 64 · O 48 (UK 43.5) · G 22.5 (1:29 and 1:32 are common too)
const float SCALE      = 87.1;
const char  SCALE_NAME[] = "HO";
const bool  USE_MPH    = true;      // false = km/h

// ====================================================== 3. Display 🔢
// DISPLAY_NONE · DISPLAY_TM1637 (4-digit LED) · DISPLAY_OLED (0.96" SSD1306 I2C)
#define DISPLAY_NONE   0
#define DISPLAY_TM1637 1
#define DISPLAY_OLED   2
#define DISPLAY_TYPE   DISPLAY_NONE

#define TM1637_CLK     4
#define TM1637_DIO     5
#define OLED_ADDR      0x3C         // some modules answer at 0x3D
const uint32_t HOLD_READING_MS = 30000;  // then the display goes back to "ready"

// ================================================ 4. Serial output 💬
// SERIAL_TEXT  "🚄 A->B 1.74 s, 33.6 mph (HO)" for the Serial Monitor
// SERIAL_JSON  {"speed":32.1,"units":"mph",…} one line per train, for scripts
// SERIAL_DEJA  plug into a DEJA server's USB port: both sensors report as
//              DEJA sensors 0 and 1 (blocks, triggers, effects can use them)
//              and each speed shows up in the DEJA server log
#define SERIAL_TEXT 0
#define SERIAL_JSON 1
#define SERIAL_DEJA 2
#define SERIAL_MODE SERIAL_TEXT
#define DEJA_DEVICE_ID "speedometer"   // add a device with this id in DEJA first

// ========================================= 5. MQTT (ESP32 only) 📡
// Publishes every reading as JSON to MQTT_TOPIC. Point it at the DEJA
// server's broker (port 1883) or any other (Home Assistant, Node-RED…).
// WiFi and broker details go in secrets.h: copy secrets.example.h.
#define USE_MQTT false
#define MQTT_TOPIC "DEJA/my-layout/speedometer"

// ================================================================
// ============================ the sketch ========================
// ================================================================

#if USE_MQTT && !defined(ESP32)
#error "MQTT needs WiFi: build this for an ESP32, or set USE_MQTT false"
#endif

#if DISPLAY_TYPE == DISPLAY_TM1637
#include <TM1637Display.h>          // library: TM1637 by Avishay Orpaz
TM1637Display led(TM1637_CLK, TM1637_DIO);
#elif DISPLAY_TYPE == DISPLAY_OLED
#include <Wire.h>
#include <SSD1306Ascii.h>           // library: SSD1306Ascii by Bill Greiman
#include <SSD1306AsciiWire.h>       // (text only, so it fits a Nano's RAM)
SSD1306AsciiWire oled;
#endif

#if USE_MQTT
#include <WiFi.h>
#include <PubSubClient.h>
#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.example.h"
#warning "No secrets.h yet: copy secrets.example.h to secrets.h and fill it in"
#endif
WiFiClient wifi;
PubSubClient mqtt(wifi);
#endif

// ⏱️ The interrupt handlers stamp the moment each sensor first sees the
// train. 0 = not yet. (micros() is never 0 once we OR in 1.)
volatile uint32_t hitA = 0, hitB = 0;
volatile bool armed = true;

#ifdef ESP32
#define FAST IRAM_ATTR              // ESP32 interrupt handlers live in RAM
#else
#define FAST
#endif
void FAST onA() { if (armed && !hitA) hitA = micros() | 1; }
void FAST onB() { if (armed && !hitB) hitB = micros() | 1; }

bool sensorActive(uint8_t pin) {
  return digitalRead(pin) == (SENSOR_ACTIVE_LOW ? LOW : HIGH);
}

// ------------------------------------------------------- the maths 🧮
// mm per second on the layout × scale = mm per second full size.
float scaleSpeed(uint32_t micros) {
  float mmPerSec = SENSOR_GAP_MM * 1e6 / micros;
  float kmh = mmPerSec * SCALE * 3.6 / 1000.0;
  return USE_MPH ? kmh / 1.609344 : kmh;
}
const char *units() { return USE_MPH ? "mph" : "km/h"; }

// -------------------------------------------------------- display 🔢
void showReady() {
#if DISPLAY_TYPE == DISPLAY_TM1637
  const uint8_t dashes[] = { SEG_G, SEG_G, SEG_G, SEG_G };
  led.setSegments(dashes);
#elif DISPLAY_TYPE == DISPLAY_OLED
  oled.clear();
  oled.setFont(System5x7);
  oled.print(F("Speedometer  ")); oled.println(SCALE_NAME);
  oled.println();
  oled.set2X(); oled.println(F("Ready")); oled.set1X();
#endif
}

void showTiming(bool aToB) {
#if DISPLAY_TYPE == DISPLAY_TM1637
  // a dash marching toward the far sensor
  const uint8_t ab[] = { SEG_G, 0, 0, 0 }, ba[] = { 0, 0, 0, SEG_G };
  led.setSegments(aToB ? ab : ba);
#elif DISPLAY_TYPE == DISPLAY_OLED
  oled.clear();
  oled.setFont(System5x7);
  oled.println(aToB ? F("A -> B") : F("B -> A"));
  oled.println();
  oled.set2X(); oled.println(F("Timing...")); oled.set1X();
#endif
}

void showSpeed(float speed, bool aToB) {
#if DISPLAY_TYPE == DISPLAY_TM1637
  // under 100: one decimal (32.1), otherwise whole numbers
  if (speed < 100) led.showNumberDecEx((int)(speed * 10 + 0.5), 0b00100000);
  else led.showNumberDec((int)(speed + 0.5));
#elif DISPLAY_TYPE == DISPLAY_OLED
  oled.clear();
  oled.setFont(System5x7);
  oled.print(aToB ? F("A -> B   ") : F("B -> A   "));
  oled.print(SCALE_NAME); oled.print(' '); oled.println(units());
  oled.setFont(lcdnums14x24);
  oled.set2X();
  oled.println(speed, speed < 100 ? 1 : 0);
  oled.set1X();
#endif
}

// --------------------------------------------------------- serial 💬
void reportSpeed(float speed, bool aToB, uint32_t us) {
#if SERIAL_MODE == SERIAL_JSON
  Serial.print(F("{\"speed\":")); Serial.print(speed, 1);
  Serial.print(F(",\"units\":\"")); Serial.print(units());
  Serial.print(F("\",\"dir\":\"")); Serial.print(aToB ? F("AB") : F("BA"));
  Serial.print(F("\",\"ms\":")); Serial.print(us / 1000.0, 1);
  Serial.print(F(",\"scale\":")); Serial.print(SCALE, 1);
  Serial.println('}');
#else
  // plain text; in DEJA mode this lands in the server log
  Serial.print(SERIAL_MODE == SERIAL_DEJA ? F("[speed] ") : F("🚄 "));
  Serial.print(aToB ? F("A->B ") : F("B->A "));
  Serial.print(us / 1e6, 2); Serial.print(F(" s, "));
  Serial.print(speed, 1); Serial.print(' '); Serial.print(units());
  Serial.print(F(" (")); Serial.print(SCALE_NAME); Serial.println(')');
#endif
#if USE_MQTT
  char msg[128];
  snprintf(msg, sizeof msg,
           "{\"speed\":%.1f,\"units\":\"%s\",\"dir\":\"%s\",\"ms\":%.1f,\"scale\":%.1f}",
           speed, units(), aToB ? "AB" : "BA", us / 1000.0, SCALE);
  mqtt.publish(MQTT_TOPIC, msg);
#endif
}

void note(const __FlashStringHelper *msg) {
  // In DEJA mode keep the serial line quiet: only sensor JSON and log lines.
#if SERIAL_MODE != SERIAL_JSON
  Serial.println(msg);
#endif
}

// ----------------------------------------------- DEJA sensors 🔌
// DEJA reads `{ "sensor": n, "state": 0|1 }` (with those exact spaces) from a
// USB device, answers nothing back, and pings every 30 s expecting "pong".
#if SERIAL_MODE == SERIAL_DEJA
bool lastA = false, lastB = false;
uint32_t lastAlive = 0;

void dejaSensor(uint8_t index, bool on) {
  Serial.print(F("{ \"sensor\": ")); Serial.print(index);
  Serial.print(F(", \"state\": ")); Serial.print(on ? 1 : 0);
  Serial.println(F(" }"));
}

void dejaService(uint32_t now) {
  bool a = sensorActive(SENSOR_A_PIN), b = sensorActive(SENSOR_B_PIN);
  if (a != lastA) dejaSensor(0, lastA = a);
  if (b != lastB) dejaSensor(1, lastB = b);

  static char line[8];
  static uint8_t len = 0;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      line[len] = 0;
      if (!strcmp(line, "ping")) Serial.println(F("pong"));
      len = 0;
    } else if (len < sizeof line - 1) line[len++] = c;
  }
  if (now - lastAlive >= 60000) {
    lastAlive = now;
    Serial.print(F("{\"status\":\"alive\",\"device\":\"" DEJA_DEVICE_ID "\",\"up\":"));
    Serial.print(now / 1000);
    Serial.println('}');
  }
}
#endif

// ----------------------------------------------------------- MQTT 📡
#if USE_MQTT
void mqttService() {
  static uint32_t lastTry = 0;
  if (WiFi.status() != WL_CONNECTED) return;    // WiFi reconnects by itself
  if (mqtt.connected()) { mqtt.loop(); return; }
  if (millis() - lastTry < 5000) return;
  lastTry = millis();
  String id = "speedometer-" + WiFi.macAddress();
  const char *user = *MQTT_USERNAME ? MQTT_USERNAME : nullptr;  // "" = no login
  if (mqtt.connect(id.c_str(), user, MQTT_PASSWORD))
    Serial.println(F("[mqtt] connected"));
}
#endif

// ---------------------------------------------------- setup + loop 🔁
enum State { READY, TIMING, CLEARING } state = READY;
uint32_t stateSince = 0, shownAt = 0, clearSince = 0;
bool showingSpeed = false;

void setup() {
  Serial.begin(115200);
  pinMode(SENSOR_A_PIN, INPUT_PULLUP);
  pinMode(SENSOR_B_PIN, INPUT_PULLUP);
  int edge = SENSOR_ACTIVE_LOW ? FALLING : RISING;
  attachInterrupt(digitalPinToInterrupt(SENSOR_A_PIN), onA, edge);
  attachInterrupt(digitalPinToInterrupt(SENSOR_B_PIN), onB, edge);

#if DISPLAY_TYPE == DISPLAY_TM1637
  led.setBrightness(5);
#elif DISPLAY_TYPE == DISPLAY_OLED
  Wire.begin();
  oled.begin(&Adafruit128x64, OLED_ADDR);
#endif
  showReady();

#if USE_MQTT
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
#endif

#if SERIAL_MODE == SERIAL_DEJA
  Serial.println(F("{\"status\":\"connected\",\"device\":\"" DEJA_DEVICE_ID "\"}"));
#elif SERIAL_MODE == SERIAL_TEXT
  Serial.print(F("🚄 Speedometer ready: ")); Serial.print(SENSOR_GAP_MM, 0);
  Serial.print(F(" mm gap, ")); Serial.print(SCALE_NAME);
  Serial.print(F(" (1:")); Serial.print(SCALE, 1); Serial.println(')');
#endif
}

void loop() {
  uint32_t now = millis();
#if SERIAL_MODE == SERIAL_DEJA
  dejaService(now);
#endif
#if USE_MQTT
  mqttService();
#endif

  noInterrupts();
  uint32_t a = hitA, b = hitB;
  interrupts();

  switch (state) {
    case READY:                       // waiting for the first sensor
      if (a || b) {
        showTiming(a && (!b || (int32_t)(b - a) > 0));
        state = TIMING; stateSince = now;
      }
      break;

    case TIMING:                      // first sensor hit, waiting for the other
      if (a && b) {
        armed = false;
        bool aToB = (int32_t)(b - a) > 0;
        uint32_t us = aToB ? b - a : a - b;
        if (us < 2000) {
          note(F("[speed] both sensors at once: check the wiring or the gap"));
          showReady();
        } else {
          float speed = scaleSpeed(us);
          showSpeed(speed, aToB);
          reportSpeed(speed, aToB, us);
          showingSpeed = true; shownAt = now;
        }
        state = CLEARING; clearSince = now;
      } else if (now - stateSince > GIVE_UP_MS) {
        armed = false;
        note(F("[speed] only one sensor tripped; gave up"));
        showReady();
        state = CLEARING; clearSince = now;
      }
      break;

    case CLEARING:                    // let the whole train go by
      if (sensorActive(SENSOR_A_PIN) || sensorActive(SENSOR_B_PIN)) clearSince = now;
      else if (now - clearSince > REARM_MS) {
        noInterrupts();
        hitA = hitB = 0; armed = true;
        interrupts();
        state = READY;
      }
      break;
  }

  if (showingSpeed && state == READY && now - shownAt > HOLD_READING_MS) {
    showingSpeed = false;
    showReady();
  }
}
