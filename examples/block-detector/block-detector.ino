/*
 * 🚦 Block Occupancy Detector — Arduino Nano or ESP32
 * ---------------------------------------------------
 * Watches up to a dozen blocks and reports which ones have a train in them.
 * Each block can use whatever sensor suits it:
 *
 *   DETECT_DIGITAL  a pin that goes LOW when occupied: IR obstacle module,
 *                   reed switch, a commercial DCC detector's output
 *   DETECT_CURRENT  an ACS712 module or current transformer on an analog pin,
 *                   in the block's feeder (sees any loco drawing power)
 *   DETECT_LIGHT    an LDR between the ties: a train's shadow darkens it
 *
 * …and tell the world however suits you:
 *   💡 an LED per block on the fascia
 *   💬 Serial Monitor text, or one JSON line per change
 *   🔌 DEJA over USB: each block is a DEJA sensor (index = its row below)
 *   📡 DEJA over WiFi (ESP32): a DEJA MQTT device, same sensors
 *
 * Settings: the numbered blocks below. Everything after "the sketch" is
 * plain code you're welcome to change.
 */

// ====================================================== 1. Blocks 🧱
#define DETECT_DIGITAL 0
#define DETECT_CURRENT 1
#define DETECT_LIGHT   2
#define NO_PIN         255

struct Block {
  const char *name;
  uint8_t pin;          // sensor pin (analog pins for CURRENT and LIGHT)
  uint8_t type;         // DETECT_…
  uint8_t ledPin;       // fascia LED, lit while occupied (NO_PIN = none)
};

// One row per block. The row number (0, 1, 2…) is its DEJA sensor index.
const Block BLOCKS[] = {
  { "Main east",  2,  DETECT_DIGITAL, 8 },
  { "Main west",  3,  DETECT_DIGITAL, 9 },
  { "Siding",     A0, DETECT_CURRENT, 10 },
  { "Yard lead",  A1, DETECT_LIGHT,   11 },
};

// ================================================ 2. Sensitivity 🎚️
#define DIGITAL_ACTIVE_LOW true     // IR modules, reed switches to GND, most detectors
// CURRENT: how big a swing (ADC counts) counts as a train. Type c and Enter
// in the Serial Monitor to see live readings, then set it between the empty
// reading and the reading with a loco sitting in the block.
const uint16_t CURRENT_THRESHOLD = 10;
// LIGHT: how much darker than the room (ADC counts) counts as a shadow.
// The sketch follows the room light slowly, so lamps going on and off are fine.
const uint16_t LIGHT_MARGIN = 80;

// A block shows occupied once the sensor has seen a train this long…
const uint32_t OCCUPIED_AFTER_MS = 100;
// …and clear once it hasn't for this long. Bridges dirty track, gaps between
// cars, and spot sensors that only see the train between its wheels.
const uint32_t CLEAR_AFTER_MS = 3000;

// ===================================================== 3. Output 💬
// SERIAL_TEXT  "🟥 Main east occupied" (type ? and Enter for commands)
// SERIAL_JSON  {"block":0,"name":"Main east","occupied":true}
// SERIAL_DEJA  plug into the DEJA server's USB: each block reports as a
//              DEJA sensor. Add a device with DEVICE_ID in DEJA first.
#define SERIAL_TEXT 0
#define SERIAL_JSON 1
#define SERIAL_DEJA 2
#define SERIAL_MODE SERIAL_TEXT
#define DEVICE_ID   "blocks"

// ============================================ 4. WiFi (ESP32 only) 📡
// A DEJA WiFi device over MQTT: publishes each block as a DEJA sensor on
// DEJA/<LAYOUT_ID>/<DEVICE_ID>/messages. Add the device in DEJA as an MQTT
// (DIY) device with the same id. WiFi + broker go in secrets.h.
#define USE_MQTT  false
#define LAYOUT_ID "my-layout"

// ================================================================
// ============================ the sketch ========================
// ================================================================

#if USE_MQTT && !defined(ESP32)
#error "MQTT needs WiFi: build this for an ESP32, or set USE_MQTT false"
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
char topicOut[96];
#endif

#ifdef ESP32
const int ADC_MID = 2048;           // 12-bit ADC
#else
const int ADC_MID = 512;            // 10-bit ADC
#endif

const uint8_t COUNT = sizeof BLOCKS / sizeof BLOCKS[0];
bool     occupied[COUNT];           // what we've reported
bool     seen[COUNT];               // what the sensor says right now
uint32_t seenSince[COUNT];          // when `seen` last changed
uint16_t level[COUNT];              // last analog reading, for calibration
float    roomLight[COUNT];          // LIGHT: the slowly-followed empty reading
bool     calibrating = false;

// -------------------------------------------------------- sensing 👀
// CURRENT: DCC is a fast square wave, so look at the swing over a few
// milliseconds (peak to peak). Plain DC shows up as a shift from the
// sensor's zero (mid-scale on an ACS712), so count that too.
uint16_t currentLevel(uint8_t pin) {
  int lo = 32767, hi = -1;
  long sum = 0;
  for (uint8_t i = 0; i < 40; i++) {
    int v = analogRead(pin);
    lo = min(lo, v); hi = max(hi, v); sum += v;
  }
  int swing = hi - lo;
  int shift = abs((int)(sum / 40) - ADC_MID) * 2;
  return max(swing, shift);
}

// LIGHT: LDR from the pin to GND, 10 kΩ from the pin to 5V. A shadow
// raises the reading. Compare against the room, which we follow slowly
// while the block is clear.
bool lightSees(uint8_t i) {
  uint16_t v = analogRead(BLOCKS[i].pin);
  level[i] = v;
  if (roomLight[i] < 0) roomLight[i] = v;
  bool dark = v > roomLight[i] + LIGHT_MARGIN;
  if (!dark && !occupied[i]) roomLight[i] += (v - roomLight[i]) * 0.01;
  return dark;
}

bool sense(uint8_t i) {
  const Block &b = BLOCKS[i];
  switch (b.type) {
    case DETECT_CURRENT: level[i] = currentLevel(b.pin); return level[i] > CURRENT_THRESHOLD;
    case DETECT_LIGHT:   return lightSees(i);
    default:             return digitalRead(b.pin) == (DIGITAL_ACTIVE_LOW ? LOW : HIGH);
  }
}

// -------------------------------------------------------- reporting 💬
void report(uint8_t i) {
  bool on = occupied[i];
  if (BLOCKS[i].ledPin != NO_PIN) digitalWrite(BLOCKS[i].ledPin, on ? HIGH : LOW);
#if SERIAL_MODE == SERIAL_DEJA
  // DEJA matches this exact shape, spaces included
  Serial.print(F("{ \"sensor\": ")); Serial.print(i);
  Serial.print(F(", \"state\": ")); Serial.print(on ? 1 : 0); Serial.println(F(" }"));
#elif SERIAL_MODE == SERIAL_JSON
  Serial.print(F("{\"block\":")); Serial.print(i);
  Serial.print(F(",\"name\":\"")); Serial.print(BLOCKS[i].name);
  Serial.print(F("\",\"occupied\":")); Serial.print(on ? F("true") : F("false")); Serial.println('}');
#else
  Serial.print(on ? F("🟥 ") : F("🟩 ")); Serial.print(BLOCKS[i].name);
  Serial.println(on ? F(" occupied") : F(" clear"));
#endif
#if USE_MQTT
  if (mqtt.connected()) {
    char msg[96];
    snprintf(msg, sizeof msg, "{\"device\":\"%s\",\"sensor\":%u,\"state\":%u}", DEVICE_ID, i, on ? 1 : 0);
    mqtt.publish(topicOut, msg);
  }
#endif
}

void printStatus() {
  for (uint8_t i = 0; i < COUNT; i++) {
    Serial.print(i); Serial.print(F("  ")); Serial.print(BLOCKS[i].name);
    Serial.print(occupied[i] ? F("  occupied") : F("  clear"));
    if (BLOCKS[i].type != DETECT_DIGITAL) { Serial.print(F("  level ")); Serial.print(level[i]); }
    if (BLOCKS[i].type == DETECT_LIGHT) { Serial.print(F("  room ")); Serial.print((int)roomLight[i]); }
    Serial.println();
  }
}

// ------------------------------------------------------- commands ⌨️
// Text/JSON: ? help · s status · c live readings on/off. DEJA: ping → pong.
void command(const char *line) {
  if (!strcmp(line, "ping")) { Serial.println(F("pong")); return; }
#if SERIAL_MODE != SERIAL_DEJA
  if (!strcmp(line, "s")) printStatus();
  else if (!strcmp(line, "c")) {
    calibrating = !calibrating;
    Serial.println(calibrating ? F("📈 Live readings every 0.5 s. c again to stop.") : F("📈 Stopped."));
  } else if (!strcmp(line, "?")) {
    Serial.println(F("s = status of every block · c = live sensor readings (to set thresholds)"));
  }
#endif
}

void readSerial() {
  static char line[12];
  static uint8_t len = 0;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') { line[len] = 0; if (len) command(line); len = 0; }
    else if (len < sizeof line - 1) line[len++] = c;
  }
}

// ----------------------------------------------------------- MQTT 📡
#if USE_MQTT
void mqttService() {
  static uint32_t lastTry = 0;
  if (WiFi.status() != WL_CONNECTED) return;
  if (mqtt.connected()) { mqtt.loop(); return; }
  if (millis() - lastTry < 5000) return;
  lastTry = millis();
  char will[80];
  snprintf(will, sizeof will, "{\"status\":\"offline\",\"device\":\"%s\"}", DEVICE_ID);
  const char *user = *MQTT_USERNAME ? MQTT_USERNAME : nullptr;
  if (mqtt.connect(DEVICE_ID, user, MQTT_PASSWORD, topicOut, 0, false, will)) {
    char hello[80];
    snprintf(hello, sizeof hello, "{\"status\":\"connected\",\"device\":\"%s\"}", DEVICE_ID);
    mqtt.publish(topicOut, hello);
    for (uint8_t i = 0; i < COUNT; i++) report(i);   // let DEJA catch up
  }
}
#endif

// ---------------------------------------------------- setup + loop 🔁
void setup() {
  Serial.begin(115200);
  for (uint8_t i = 0; i < COUNT; i++) {
    if (BLOCKS[i].type == DETECT_DIGITAL) pinMode(BLOCKS[i].pin, INPUT_PULLUP);
    if (BLOCKS[i].ledPin != NO_PIN) pinMode(BLOCKS[i].ledPin, OUTPUT);
    roomLight[i] = -1;
  }
#if SERIAL_MODE == SERIAL_DEJA
  Serial.println(F("{\"status\":\"connected\",\"device\":\"" DEVICE_ID "\"}"));
#elif SERIAL_MODE == SERIAL_TEXT
  Serial.print(F("🚦 Watching ")); Serial.print(COUNT); Serial.println(F(" blocks. ? for commands."));
#endif
#if USE_MQTT
  snprintf(topicOut, sizeof topicOut, "DEJA/%s/%s/messages", LAYOUT_ID, DEVICE_ID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
#endif
}

void loop() {
  uint32_t now = millis();
  readSerial();
#if USE_MQTT
  mqttService();
#endif

  for (uint8_t i = 0; i < COUNT; i++) {
    bool s = sense(i);
    if (s != seen[i]) { seen[i] = s; seenSince[i] = now; }
    uint32_t wait = seen[i] ? OCCUPIED_AFTER_MS : CLEAR_AFTER_MS;
    if (seen[i] != occupied[i] && now - seenSince[i] >= wait) {
      occupied[i] = seen[i];
      report(i);
    }
  }

  static uint32_t lastCal = 0, lastAlive = 0;
  if (calibrating && now - lastCal >= 500) { lastCal = now; printStatus(); Serial.println(); }
#if SERIAL_MODE == SERIAL_DEJA
  if (now - lastAlive >= 60000) {
    lastAlive = now;
    Serial.print(F("{\"status\":\"alive\",\"device\":\"" DEVICE_ID "\",\"up\":"));
    Serial.print(now / 1000); Serial.println('}');
  }
#else
  (void)lastAlive;
#endif
}
