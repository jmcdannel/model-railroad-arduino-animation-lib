/*
 * 🔀 Turnout Panel — Arduino Nano (servos on pins or a PCA9685) or ESP32
 * ---------------------------------------------------------------------
 * Slow-motion servo turnouts with everything a fascia panel needs:
 *   🔘 a push button per turnout (press to throw, press to close)
 *   💡 two LEDs per turnout showing the route (the new one blinks while moving)
 *   ⚡ a frog relay that flips polarity halfway through the throw
 *   💾 remembers every turnout across power-off, and doesn't twitch at boot
 *
 * And the same turnouts from anywhere else:
 *   🚂 DEJA over USB, or MQTT on an ESP32: DEJA's own `turnout` and `servo`
 *      commands, with DEJA's motion profiles (fast, medium, slow, bounce).
 *      Fascia throws go back to DEJA as sensors.
 *   💬 the Serial Monitor: t0 toggles turnout 0, c0 closes it, d0 diverges it
 *
 * Find each servo's two angles with the servo tuner first (same pulse
 * mapping as DEJA: 0–180° → 650–2350 µs).
 */

// ===================================================== 1. Servos 🦾
#define DRIVER_PINS    0            // servos straight on Nano pins (Nano only)
#define DRIVER_PCA9685 1            // a PCA9685 board over I2C (Nano or ESP32)
#define DRIVER DRIVER_PINS
#define PCA9685_ADDR   0x40
#define PULSE_MIN_US   650          // 0°   } the same as DEJA's firmware
#define PULSE_MAX_US   2350         // 180° } and the servo tuner

// ==================================================== 2. Turnouts 🔀
#define NO_PIN 255
struct Turnout {
  const char *name;
  uint8_t servo;                    // Nano pin, or PCA9685 channel
  uint8_t closedDeg, thrownDeg;     // from the servo tuner (straight, divergent)
  uint8_t buttonPin;                // push button to GND, or NO_PIN
  uint8_t ledClosed, ledThrown;     // fascia LEDs, or NO_PIN
  uint8_t frogPin;                  // relay: HIGH when thrown, or NO_PIN
};

// One row per turnout. The row number (0, 1, 2…) is its DEJA turnout index.
const Turnout TURNOUTS[] = {
  { "Yard lead",  9, 72, 108, 2, 4, 5, 6 },
  { "Siding",    10, 80, 100, 3, 7, 8, NO_PIN },
};

// ====================================================== 3. Motion 🐢
// INSTANT · FAST (0.5 s) · MEDIUM (1 s) · SLOW (2 s) · BOUNCE (1.5 s), the
// same profiles DEJA uses. This one is for buttons and the Serial Monitor;
// DEJA sends its own with each command.
#define BUTTON_PROFILE PROFILE_SLOW
const bool HOLD_SERVOS = true;      // false = stop driving a servo after it
                                    //   arrives (quieter, but the points can creep)

// ====================================================== 4. Serial 💬
// SERIAL_TEXT  commands and a line per throw, for the Serial Monitor
// SERIAL_DEJA  plug into the DEJA server's USB as device DEVICE_ID
#define SERIAL_TEXT 0
#define SERIAL_DEJA 2
#define SERIAL_MODE SERIAL_TEXT
#define DEVICE_ID   "turnouts"

// ============================================ 5. WiFi (ESP32 only) 📡
// A DEJA MQTT device on DEJA/<LAYOUT_ID>/<DEVICE_ID>. WiFi + broker go in
// secrets.h. (An ESP32 needs DRIVER_PCA9685.)
#define USE_MQTT  false
#define LAYOUT_ID "my-layout"

// ================================================================
// ============================ the sketch ========================
// ================================================================

#include <EEPROM.h>
#include <ArduinoJson.h>            // library: ArduinoJson by Benoît Blanchon

#if defined(ESP32) && DRIVER == DRIVER_PINS
#error "On an ESP32, put the servos on a PCA9685 board (DRIVER_PCA9685)"
#endif
#if USE_MQTT && !defined(ESP32)
#error "MQTT needs WiFi: build this for an ESP32, or set USE_MQTT false"
#endif

#if DRIVER == DRIVER_PCA9685
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>   // library: Adafruit PWM Servo Driver
Adafruit_PWMServoDriver pca(PCA9685_ADDR);
#else
#include <Servo.h>
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
char topicIn[96], topicOut[96];
#endif

const uint8_t COUNT = sizeof TURNOUTS / sizeof TURNOUTS[0];

// ------------------------------------------------ motion profiles 🐢
// Mirrors DEJA's firmware (deja-arduino.ino): steps of 50 ms, and the
// bounce curve in percent of travel.
enum Profile : uint8_t { PROFILE_INSTANT, PROFILE_FAST, PROFILE_MEDIUM, PROFILE_SLOW, PROFILE_BOUNCE };
const uint8_t PROFILE_STEPS[] = { 1, 10, 20, 40, 30 };
const uint8_t BOUNCE[30] = { 0, 2, 3, 7, 13, 33, 50, 83, 100, 83, 75, 70, 65, 60, 60,
                             45, 30, 10, 0, 0, 10, 30, 45, 60, 60, 65, 70, 75, 83, 100 };
uint32_t durationMs(uint8_t p) { return p == PROFILE_INSTANT ? 0 : PROFILE_STEPS[p] * 50UL; }

Profile profileNamed(const char *name) {
  if (!strcmp(name, "instant")) return PROFILE_INSTANT;
  if (!strcmp(name, "fast"))    return PROFILE_FAST;
  if (!strcmp(name, "slow"))    return PROFILE_SLOW;
  if (!strcmp(name, "bounce"))  return PROFILE_BOUNCE;
  return PROFILE_MEDIUM;
}

struct Move {
  float    from, to, pos;           // degrees
  uint32_t startMs;
  Profile  profile;
  bool     moving, thrown, frogDone;
};
Move m[COUNT];
#if DRIVER == DRIVER_PINS
Servo servos[COUNT];
#endif

// ------------------------------------------------------ the servo 🦾
uint16_t pulseUs(float deg) { return PULSE_MIN_US + (uint16_t)((PULSE_MAX_US - PULSE_MIN_US) * deg / 180 + 0.5f); }

void drive(uint8_t i, float deg) {
  m[i].pos = deg;
#if DRIVER == DRIVER_PCA9685
  pca.setPWM(TURNOUTS[i].servo, 0, (uint16_t)(pulseUs(deg) / 1e6 * 50 * 4096));
#else
  servos[i].writeMicroseconds(pulseUs(deg));            // set the pulse first…
  if (!servos[i].attached()) servos[i].attach(TURNOUTS[i].servo, PULSE_MIN_US, PULSE_MAX_US);  // …then attach: no jump
#endif
}

void release(uint8_t i) {
#if DRIVER == DRIVER_PCA9685
  pca.setPWM(TURNOUTS[i].servo, 0, 0);
#else
  servos[i].detach();
#endif
}

void setPin(uint8_t pin, bool on) { if (pin != NO_PIN) digitalWrite(pin, on ? HIGH : LOW); }

void showLeds(uint8_t i, uint32_t now) {
  const Turnout &t = TURNOUTS[i];
  bool blink = m[i].moving && (now / 125) % 2;         // the new route blinks while moving
  setPin(t.ledClosed, !m[i].thrown && (!m[i].moving || blink));
  setPin(t.ledThrown,  m[i].thrown && (!m[i].moving || blink));
}

// ------------------------------------------------------- throwing 🔀
void report(uint8_t i);

void moveTo(uint8_t i, float deg, bool thrown, Profile profile) {
  Move &mv = m[i];
  mv.from = mv.pos; mv.to = deg; mv.thrown = thrown;
  mv.profile = profile; mv.startMs = millis();
  mv.moving = true; mv.frogDone = false;
}

void setThrown(uint8_t i, bool thrown, Profile profile) {
  moveTo(i, thrown ? TURNOUTS[i].thrownDeg : TURNOUTS[i].closedDeg, thrown, profile);
}

void tick(uint8_t i, uint32_t now) {
  Move &mv = m[i];
  if (!mv.moving) return;
  uint32_t elapsed = now - mv.startMs, dur = durationMs(mv.profile);
  float pos;
  if (elapsed >= dur) pos = mv.to;
  else if (mv.profile == PROFILE_BOUNCE) {
    uint32_t step = elapsed / 50;
    pos = mv.from + (mv.to - mv.from) * BOUNCE[step > 29 ? 29 : step] / 100;
  }
  else pos = mv.from + (mv.to - mv.from) * elapsed / dur;
  drive(i, pos);

  // ⚡ flip the frog halfway, while the points are clear of both rails
  if (!mv.frogDone && elapsed * 2 >= dur) { setPin(TURNOUTS[i].frogPin, mv.thrown); mv.frogDone = true; }

  if (elapsed >= dur) {
    mv.moving = false;
    if (!HOLD_SERVOS) release(i);
    if (EEPROM.read(1 + i) != mv.thrown) {   // only write when it changed
      EEPROM.write(1 + i, mv.thrown);
#ifdef ESP32
      EEPROM.commit();
#endif
    }
    report(i);
  }
}

// ---------------------------------------------------- reporting 💬
// To DEJA: each turnout is also a sensor (1 = thrown), so a fascia throw can
// show up in DEJA. Text: a line per throw.
void report(uint8_t i) {
#if SERIAL_MODE == SERIAL_DEJA
  Serial.print(F("{ \"sensor\": ")); Serial.print(i);
  Serial.print(F(", \"state\": ")); Serial.print(m[i].thrown ? 1 : 0); Serial.println(F(" }"));
#else
  Serial.print(m[i].thrown ? F("↗️ ") : F("⬆️ ")); Serial.print(TURNOUTS[i].name);
  Serial.print(m[i].thrown ? F(" thrown (") : F(" closed (")); Serial.print((int)(m[i].pos + 0.5)); Serial.println(F("°)"));
#endif
#if USE_MQTT
  if (mqtt.connected()) {
    char msg[96];
    snprintf(msg, sizeof msg, "{\"device\":\"%s\",\"sensor\":%u,\"state\":%u}", DEVICE_ID, i, m[i].thrown ? 1 : 0);
    mqtt.publish(topicOut, msg);
  }
#endif
}

// ---------------------------------------------------- DEJA commands 🚂
// turnout {turnout, state}       state true = thrown, moved with our angles
// servo   {servo, value, profile} DEJA's own angle; thrown = nearer thrownDeg
// The index is the row in TURNOUTS.
void dejaAction(JsonObjectConst a) {
  if (a["device"].is<const char *>() && strcmp(a["device"], DEVICE_ID) != 0) return;
  const char *action = a["action"] | "";
  JsonObjectConst p = a["payload"];
  if (!strcmp(action, "turnout")) {
    int i = p["turnout"] | -1;
    if (i >= 0 && i < COUNT) setThrown(i, p["state"] | false, BUTTON_PROFILE);
  } else if (!strcmp(action, "servo")) {
    int i = p["servo"] | -1;
    if (i < 0 || i >= COUNT) return;
    int deg = constrain(p["value"] | 90, 0, 180);
    const Turnout &t = TURNOUTS[i];
    bool thrown = abs(deg - t.thrownDeg) < abs(deg - t.closedDeg);
    moveTo(i, deg, thrown, profileNamed(p["profile"] | "medium"));
  }
}

void dejaFrame(const char *text, size_t len) {
  JsonDocument doc;
  if (deserializeJson(doc, text, len)) return;
  if (doc.is<JsonArrayConst>()) for (JsonObjectConst a : doc.as<JsonArrayConst>()) dejaAction(a);
  else dejaAction(doc.as<JsonObjectConst>());
}

// --------------------------------------------------- serial commands ⌨️
// DEJA sends JSON arrays with no newline, and "ping" lines; a frame ends at
// a newline or when its outer bracket closes. People type t0 · c0 · d0 · p · ?
void command(char *line) {
  if (!strcmp(line, "ping")) { Serial.println(F("pong")); return; }
  int i = atoi(line + 1);
  bool valid = i >= 0 && i < COUNT && line[1];
  switch (line[0]) {
    case 't': if (valid) setThrown(i, !m[i].thrown, BUTTON_PROFILE); break;
    case 'c': if (valid) setThrown(i, false, BUTTON_PROFILE); break;
    case 'd': if (valid) setThrown(i, true, BUTTON_PROFILE); break;
    case 'p': for (uint8_t k = 0; k < COUNT; k++) report(k); break;
    case '?': Serial.println(F("t0 toggle turnout 0 · c0 close it · d0 diverge it · p list all")); break;
  }
}

void readSerial() {
  static char buf[192];
  static uint8_t len = 0, depth = 0;
  static bool inString = false;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      buf[len] = 0;
      if (len && (buf[0] == '[' || buf[0] == '{')) dejaFrame(buf, len);
      else if (len) command(buf);
      len = depth = 0; inString = false;
      continue;
    }
    if (len == sizeof buf - 1) { len = depth = 0; inString = false; }   // too long: drop it
    buf[len++] = c;
    if (buf[0] != '[' && buf[0] != '{') continue;
    if (c == '"' && (len < 2 || buf[len - 2] != '\\')) inString = !inString;
    else if (!inString && (c == '[' || c == '{')) depth++;
    else if (!inString && (c == ']' || c == '}') && depth && --depth == 0) {
      dejaFrame(buf, len);
      len = 0; inString = false;
    }
  }
}

// ---------------------------------------------------------- buttons 🔘
void scanButtons(uint32_t now) {
  static bool down[sizeof TURNOUTS / sizeof TURNOUTS[0]];
  static uint32_t changedAt[sizeof TURNOUTS / sizeof TURNOUTS[0]];
  for (uint8_t i = 0; i < COUNT; i++) {
    if (TURNOUTS[i].buttonPin == NO_PIN) continue;
    bool d = digitalRead(TURNOUTS[i].buttonPin) == LOW;
    if (d != down[i] && now - changedAt[i] > 30) {
      down[i] = d; changedAt[i] = now;
      if (d) setThrown(i, !m[i].thrown, BUTTON_PROFILE);   // press: the other way
    }
  }
}

// ----------------------------------------------------------- MQTT 📡
#if USE_MQTT
void onMqtt(char *, byte *payload, unsigned int len) { dejaFrame((const char *)payload, len); }

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
    mqtt.subscribe(topicIn);
    char hello[80];
    snprintf(hello, sizeof hello, "{\"status\":\"connected\",\"device\":\"%s\"}", DEVICE_ID);
    mqtt.publish(topicOut, hello);
    for (uint8_t i = 0; i < COUNT; i++) report(i);
  }
}
#endif

// ---------------------------------------------------- setup + loop 🔁
const uint8_t MAGIC = 0x7A;         // EEPROM byte 0: "the bytes after me are ours"

void setup() {
  Serial.begin(115200);
#ifdef ESP32
  EEPROM.begin(64);
#endif
#if DRIVER == DRIVER_PCA9685
  pca.begin();
  pca.setPWMFreq(50);
#endif
  bool saved = EEPROM.read(0) == MAGIC;
  if (!saved) EEPROM.write(0, MAGIC);
  for (uint8_t i = 0; i < COUNT; i++) {
    const Turnout &t = TURNOUTS[i];
    if (t.buttonPin != NO_PIN) pinMode(t.buttonPin, INPUT_PULLUP);
    if (t.ledClosed != NO_PIN) pinMode(t.ledClosed, OUTPUT);
    if (t.ledThrown != NO_PIN) pinMode(t.ledThrown, OUTPUT);
    if (t.frogPin != NO_PIN)   pinMode(t.frogPin, OUTPUT);
    // 💾 back where it was at power-off: the frog first, then the servo
    // straight to that spot (no sweep across the rails)
    m[i].thrown = saved && EEPROM.read(1 + i) == 1;
    setPin(t.frogPin, m[i].thrown);
    drive(i, m[i].thrown ? t.thrownDeg : t.closedDeg);
    if (!HOLD_SERVOS) { delay(300); release(i); }
  }
#ifdef ESP32
  EEPROM.commit();
#endif

#if SERIAL_MODE == SERIAL_DEJA
  Serial.println(F("{\"status\":\"connected\",\"device\":\"" DEVICE_ID "\"}"));
#else
  Serial.print(F("🔀 Turnout panel: ")); Serial.print(COUNT); Serial.println(F(" turnouts. ? for commands."));
#endif
  for (uint8_t i = 0; i < COUNT; i++) report(i);
#if USE_MQTT
  snprintf(topicIn, sizeof topicIn, "DEJA/%s/%s", LAYOUT_ID, DEVICE_ID);
  snprintf(topicOut, sizeof topicOut, "DEJA/%s/%s/messages", LAYOUT_ID, DEVICE_ID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMqtt);
#endif
}

void loop() {
  uint32_t now = millis();
  readSerial();
#if USE_MQTT
  mqttService();
#endif
  scanButtons(now);
  for (uint8_t i = 0; i < COUNT; i++) { tick(i, now); showLeds(i, now); }

#if SERIAL_MODE == SERIAL_DEJA
  static uint32_t lastAlive = 0;
  if (now - lastAlive >= 60000) {
    lastAlive = now;
    Serial.print(F("{\"status\":\"alive\",\"device\":\"" DEVICE_ID "\",\"up\":"));
    Serial.print(now / 1000); Serial.println('}');
  }
#endif
}
