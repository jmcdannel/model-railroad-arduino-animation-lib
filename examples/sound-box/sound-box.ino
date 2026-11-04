/*
 * 🔊 Sound Trigger Box — Arduino Nano or ESP32 + DFPlayer Mini
 * ------------------------------------------------------------
 * One MP3 player, many ways to set it off:
 *   🔘 fascia buttons: horn, station announcement, yard sounds on/off
 *   🔌 trigger inputs: the crossing gates' bell output, a sensor, a relay
 *   🚂 DEJA: a `pin` effect on this device acts exactly like that pin being
 *      wired, so DEJA effects, routes and macros can play sounds
 *   💬 the Serial Monitor: p3 plays track 3, l3 loops it, s stops
 *
 * Each trigger plays once, loops while it's on, or toggles a loop. The
 * player plays one sound at a time: the newest trigger wins, and (with the
 * BUSY pin wired) a loop that was interrupted by a one-shot comes back.
 *
 * SD card: a folder named "mp3" holding 0001.mp3, 0002.mp3, … (the number
 * is the track; anything after it is ignored: 0001_horn.mp3).
 *
 * Wiring (Nano):
 *   D11 → 1 kΩ → DFPlayer RX     (the resistor stops 5 V hiss)
 *   D10 ← DFPlayer BUSY          (optional; LOW while playing)
 *   DFPlayer VCC → 5V, GND → GND, SPK_1 / SPK_2 → an 8 Ω speaker (≤ 3 W)
 *   Buttons: pin → button → GND (no resistor needed)
 * ESP32: TX 17 → DFPlayer RX (no resistor), BUSY → 16, VCC → 5V (VIN).
 */

// ====================================================== 1. Player 🎵
#define PLAYER_TX_PIN   11          // → 1 kΩ → DFPlayer RX (ESP32: 17)
#define PLAYER_RX_PIN   9           // Nano only: SoftwareSerial needs a receive pin,
                                    //   though nothing is read. Leave it unwired.
#define BUSY_PIN        10          // DFPlayer BUSY, or -1 if not wired (ESP32: 16)
#define VOLUME          20          // 0–30
#define VOLUME_KNOB_PIN -1          // a 10 kΩ pot's wiper (e.g. A0), or -1

// ===================================================== 2. Triggers 🔘
#define PLAY_ONCE     0   // each press plays the clip once (again = restart)
#define PLAY_WHILE_ON 1   // loops while the input is on, stops when it goes off
#define PLAY_TOGGLE   2   // press to start looping, press again to stop

#define TO_GND        0   // button, relay contact or IR module pulling the pin LOW
#define HIGH_IS_ON    1   // a logic output: DEJA IO pin, gates' bell out, PIR
#define DEJA_ONLY     2   // nothing wired: only DEJA (or serial) sets it

struct Trigger { uint8_t pin; uint8_t wiring; uint16_t track; uint8_t mode; };

// The pin number is also how DEJA reaches a trigger: a DEJA `pin` effect
// with pin 12 on this device works the bell below.
const Trigger TRIGGERS[] = {
  { 2,  TO_GND,    1, PLAY_ONCE },      // fascia button: horn
  { 3,  TO_GND,    2, PLAY_ONCE },      // fascia button: station announcement
  { 4,  TO_GND,    3, PLAY_TOGGLE },    // fascia button: yard sounds on / off
  { 12, DEJA_ONLY, 4, PLAY_WHILE_ON },  // crossing bell. Wire the gates' bell out
                                        //   to D12 and change to HIGH_IS_ON.
};
// A DEJA `pin` effect for a pin that isn't in the table plays track
// <pin> once (on) and stops it (off).

// ====================================================== 3. Serial 💬
// SERIAL_TEXT  commands and a line per sound, for the Serial Monitor
// SERIAL_DEJA  plug into the DEJA server's USB as device DEVICE_ID
#define SERIAL_TEXT 0
#define SERIAL_DEJA 2
#define SERIAL_MODE SERIAL_TEXT
#define DEVICE_ID   "sound-box"

// ============================================ 4. WiFi (ESP32 only) 📡
// A DEJA MQTT device: listens on DEJA/<LAYOUT_ID>/<DEVICE_ID> for the same
// `pin` commands. WiFi + broker go in secrets.h.
#define USE_MQTT  false
#define LAYOUT_ID "my-layout"

// ================================================================
// ============================ the sketch ========================
// ================================================================

#include <ArduinoJson.h>            // library: ArduinoJson by Benoît Blanchon

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
char topicIn[96], topicOut[96];
#endif

#ifdef ESP32
HardwareSerial player(2);
#else
#include <SoftwareSerial.h>
SoftwareSerial player(PLAYER_RX_PIN, PLAYER_TX_PIN);
#endif

const uint8_t COUNT = sizeof TRIGGERS / sizeof TRIGGERS[0];
bool wiredOn[COUNT];               // what the pin says (debounced)
bool dejaOn[COUNT];                // what DEJA (or serial) says
bool wasOn[COUNT];                 // last combined state, for edges
bool latched[COUNT];               // TOGGLE: loop switched on
uint32_t changedAt[COUNT];

int16_t  nowPlaying = -1;          // track, or -1
bool     nowLooping = false;
uint32_t startedAt = 0;

// ------------------------------------------------ DFPlayer 🎵
// Frame: 7E FF 06 cmd 00 paramH paramL checksumH checksumL EF. The player
// needs ~100 ms between commands, so they wait in a little queue.
enum : uint8_t { CMD_VOLUME = 0x06, CMD_STOP = 0x16, CMD_PLAY_MP3 = 0x12, CMD_LOOP = 0x19 };
struct Cmd { uint8_t cmd; uint16_t param; };
Cmd queue[6]; uint8_t qHead = 0, qCount = 0;
uint32_t nextSendAt = 0;

void send(uint8_t cmd, uint16_t param) {
  uint8_t f[10] = { 0x7E, 0xFF, 0x06, cmd, 0x00, (uint8_t)(param >> 8), (uint8_t)param, 0, 0, 0xEF };
  uint16_t sum = 0;
  for (uint8_t i = 1; i < 7; i++) sum += f[i];
  sum = -sum;
  f[7] = sum >> 8; f[8] = sum;
  player.write(f, sizeof f);
}
void enqueue(uint8_t cmd, uint16_t param) {
  if (qCount == sizeof queue / sizeof queue[0]) return;
  queue[(qHead + qCount++) % 6] = { cmd, param };
}
void pump(uint32_t now) {
  if (!qCount || (int32_t)(now - nextSendAt) < 0) return;
  send(queue[qHead].cmd, queue[qHead].param);
  qHead = (qHead + 1) % 6; qCount--;
  nextSendAt = now + 100;
}

void say(const __FlashStringHelper *what, int track) {
#if SERIAL_MODE == SERIAL_TEXT
  Serial.print(what); if (track >= 0) Serial.print(track); Serial.println();
#else
  (void)what; (void)track;          // DEJA mode: keep the line quiet
#endif
}

void play(uint16_t track, bool loop) {
  qCount = 0;                       // anything still queued is stale now
  enqueue(CMD_PLAY_MP3, track);
  if (loop) enqueue(CMD_LOOP, 0);   // 0 = repeat this track
  nowPlaying = track; nowLooping = loop; startedAt = millis();
  say(loop ? F("🔁 looping track ") : F("▶️ playing track "), track);
}

void stop() {
  qCount = 0;
  enqueue(CMD_STOP, 0);
  if (nowPlaying >= 0) say(F("⏹️ stopped"), -1);
  nowPlaying = -1; nowLooping = false;
}

#if BUSY_PIN >= 0
// true while the player is busy. BUSY takes a moment to go LOW after a play.
bool busy() { return millis() - startedAt < 500 || digitalRead(BUSY_PIN) == LOW; }
#endif

// ------------------------------------------------ triggers 🔘
bool wantsLoop(uint8_t i) {
  if (TRIGGERS[i].mode == PLAY_WHILE_ON) return wasOn[i];
  if (TRIGGERS[i].mode == PLAY_TOGGLE) return latched[i];
  return false;
}

void onEdge(uint8_t i, bool on) {
  const Trigger &t = TRIGGERS[i];
  switch (t.mode) {
    case PLAY_ONCE:
      if (on) play(t.track, false);
      break;
    case PLAY_WHILE_ON:
      if (on) play(t.track, true);
      else if (nowPlaying == (int16_t)t.track) stop();
      break;
    case PLAY_TOGGLE:
      if (!on) break;
      latched[i] = !latched[i];
      if (latched[i]) play(t.track, true);
      else if (nowPlaying == (int16_t)t.track) stop();
      break;
  }
}

void scanTriggers(uint32_t now) {
  for (uint8_t i = 0; i < COUNT; i++) {
    const Trigger &t = TRIGGERS[i];
    if (t.wiring != DEJA_ONLY) {
      bool raw = digitalRead(t.pin) == (t.wiring == TO_GND ? LOW : HIGH);
      if (raw != wiredOn[i] && now - changedAt[i] > 30) { wiredOn[i] = raw; changedAt[i] = now; }
    }
    bool on = wiredOn[i] || dejaOn[i];
    if (on != wasOn[i]) { wasOn[i] = on; onEdge(i, on); }
  }
#if BUSY_PIN >= 0
  // 🔁 a one-shot finished: bring back a loop that's still wanted. (Without
  // the BUSY pin we can't tell when a clip ends, so loops don't come back.)
  if (!busy()) {
    if (nowPlaying >= 0 && !nowLooping) nowPlaying = -1;
    for (int8_t i = COUNT - 1; i >= 0 && nowPlaying < 0; i--)
      if (wantsLoop(i)) play(TRIGGERS[i].track, true);
  }
#endif
}

// ---------------------------------------------- DEJA commands 🚂
// `pin` {pin, state}: the server sends the pin as a string ("12") and the
// state as true/false. Same handler for USB and MQTT.
void dejaPin(int pin, bool state) {
  for (uint8_t i = 0; i < COUNT; i++)
    if (TRIGGERS[i].pin == pin) { dejaOn[i] = state; return; }
  if (state) play(pin, false);                     // not in the table: play track <pin>
  else if (nowPlaying == pin) stop();
}

void dejaAction(JsonObjectConst a) {
  const char *action = a["action"] | "";
  if (strcmp(action, "pin") != 0) return;          // servos, turnouts: not ours
  if (a["device"].is<const char *>() && strcmp(a["device"], DEVICE_ID) != 0) return;
  JsonVariantConst p = a["payload"]["pin"];
  int pin = p.is<const char *>() ? atoi(p.as<const char *>()) : p.as<int>();
  dejaPin(pin, a["payload"]["state"] | false);
}

void dejaFrame(const char *text, size_t len) {
  JsonDocument doc;
  if (deserializeJson(doc, text, len)) return;
  if (doc.is<JsonArrayConst>()) for (JsonObjectConst a : doc.as<JsonArrayConst>()) dejaAction(a);
  else dejaAction(doc.as<JsonObjectConst>());
}

// -------------------------------------------- serial commands ⌨️
// The DEJA server sends JSON arrays with no newline, and "ping" lines. A
// frame ends at a newline or when its outer bracket closes. People type
// p3 · l3 · s · v20 · ? and Enter.
void command(char *line) {
  if (!strcmp(line, "ping")) { Serial.println(F("pong")); return; }
  int n = atoi(line + 1);
  switch (line[0]) {
    case 'p': if (n > 0) play(n, false); break;
    case 'l': if (n > 0) play(n, true); break;
    case 's': for (uint8_t i = 0; i < COUNT; i++) { latched[i] = false; dejaOn[i] = false; } stop(); break;
    case 'v': enqueue(CMD_VOLUME, constrain(n, 0, 30)); say(F("🔉 volume "), constrain(n, 0, 30)); break;
    case '?': Serial.println(F("p3 play track 3 · l3 loop track 3 · s stop · v20 volume (0-30)")); break;
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
  }
}
#endif

// ---------------------------------------------------- setup + loop 🔁
void setup() {
  Serial.begin(115200);
#ifdef ESP32
  player.begin(9600, SERIAL_8N1, -1, PLAYER_TX_PIN);
#else
  player.begin(9600);
#endif
#if BUSY_PIN >= 0
  pinMode(BUSY_PIN, INPUT_PULLUP);
#endif
  for (uint8_t i = 0; i < COUNT; i++)
    if (TRIGGERS[i].wiring != DEJA_ONLY) pinMode(TRIGGERS[i].pin, INPUT_PULLUP);

  delay(1500);                      // the DFPlayer reads its SD card at power-up
  enqueue(CMD_VOLUME, VOLUME);

#if SERIAL_MODE == SERIAL_DEJA
  Serial.println(F("{\"status\":\"connected\",\"device\":\"" DEVICE_ID "\"}"));
#else
  Serial.println(F("🔊 Sound box ready. ? for commands."));
#endif
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
  scanTriggers(now);

#if VOLUME_KNOB_PIN >= 0
  static int lastRaw = -1000;
  static uint32_t lastKnob = 0;
  if (now - lastKnob > 100) {
    lastKnob = now;
    int raw = analogRead(VOLUME_KNOB_PIN);
#ifdef ESP32
    raw /= 4;                       // 12-bit ADC → 0–1023
#endif
    if (abs(raw - lastRaw) > 20) {  // ignore wiper jitter
      lastRaw = raw;
      enqueue(CMD_VOLUME, raw * 31L / 1024);
    }
  }
#endif

#if SERIAL_MODE == SERIAL_DEJA
  static uint32_t lastAlive = 0;
  if (now - lastAlive >= 60000) {
    lastAlive = now;
    Serial.print(F("{\"status\":\"alive\",\"device\":\"" DEVICE_ID "\",\"up\":"));
    Serial.print(now / 1000); Serial.println('}');
  }
#endif
  pump(now);
}
