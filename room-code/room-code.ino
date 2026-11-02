/*
 * 🪧 Room Code Sign — two boards, one sketch
 * ------------------------------------------
 * A small always-on sign for the wall by the layout. Its home page shows the
 * DEJA room code visitors type to join as a guest, next to a QR of the guest
 * join link, with a bar that drains as the code counts down to its next
 * rotation. The BOOT button pages through three more screens:
 *
 *   1  JOIN          QR of the guest link + the 3-digit code and its countdown
 *   2  NOW RUNNING   the locos that are moving, fastest first, with roster names
 *   3  TRACK POWER   one row per track output on each EX-CommandStation
 *   4  SERVER        version, uptime, serial / cloud, standby, drivers, clients, WiFi
 *
 * Two minutes after the last press it drifts back to JOIN, so the wall
 * default is always the code. A red dot in the header on every page means
 * every known track output is off.
 *
 * It is NOT a DEJA device. The DEJA server (started with ROOM_SIGN_MQTT=true)
 * publishes two topics on its MQTT broker:
 *
 *   DEJA/<layoutId>/roomCode   {"code":"123","secondsRemaining":287,"windowSeconds":300}   every 30 s
 *   DEJA/<layoutId>/roomSign   {"join":"https://…/guest/join?layoutId=tam",
 *                               "running":[{"name":"UP 4014","address":4014,"pct":45,"fwd":true}],
 *                               "tracks":[{"device":"EX-CSB1","output":"A","mode":"MAIN","power":true}],
 *                               "server":{"version":"1.22.0","uptimeS":3700,"serial":true,"cloud":true,
 *                                         "standby":"awake","drivers":2,"clients":3}}   on change + every 30 s
 *
 * This sketch joins WiFi, subscribes to both and draws whatever it hears.
 * The code is only ever *derived* in the cloud — no secret, no clock,
 * nothing worth stealing lives on this board. If the server goes quiet
 * (90 s with no message) the digits come down and the sign says so, so a
 * stale code — or a stale train list — is never on the wall.
 *
 * Displays (pick one at build time, see README):
 *   default          Waveshare / SpotPear ESP32-C6-LCD-1.47 — 172×320 ST7789 on SPI
 *                    (MOSI 6, SCLK 7, CS 14, DC 15, RST 21, BL 22) + WS2812 status LED on GPIO 8
 *                    + the BOOT button on GPIO 9 for paging
 *   -DDISPLAY_SSD1306  any ESP32 + 0.96" 128×64 SSD1306 OLED on I2C (SDA 21, SCL 22 by default)
 *                    + a plain status LED (DevKitC clones: GPIO 2; set -1 for none).
 *                    Too small for a QR — it shows the code page only.
 *
 * States → LED:  WiFi… blue/fast blink · server… amber/slow blink · code live dim green/off
 *                · rotating amber/on · offline red/double blink
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "config.h"

#if defined(DISPLAY_SSD1306)
  #include <Wire.h>
  #include <Adafruit_SSD1306.h>
  #define HAS_PAGES 0
#else
  #include <SPI.h>
  #include <Adafruit_ST7789.h>
  #include "qrcode_gen.h"   // ricmoo/QRCode (MIT), vendored: the ESP32 core's own qrcode.h shadows the library
  #include "logo.h"         // the DEJA.js mark, 24x24 RGB565 (generated from the website's icon)
  #define HAS_PAGES 1
#endif

// ------------------------------------------------------------- Tuning 🎛️
const uint32_t OFFLINE_AFTER_MS   = 90000;   // no message for this long → "server offline"
const uint32_t MQTT_RETRY_MS      = 5000;    // between MQTT connect attempts
const uint32_t WIFI_RETRY_MS      = 15000;   // re-kick WiFi.begin if still down
const uint32_t PAGE_HOME_AFTER_MS = 120000;  // no button press for this long → back to JOIN
const uint32_t BUTTON_DEBOUNCE_MS = 30;

// Declared before any function so the Arduino builder's auto-prototypes can see them.
enum State { WIFI_CONNECTING, SERVER_CONNECTING, WAITING_FOR_CODE, CODE_LIVE, CODE_ROTATING, SERVER_OFFLINE };
enum Page  { PAGE_JOIN, PAGE_RUNNING, PAGE_TRACKS, PAGE_SERVER, PAGE_COUNT };

// What the server's sign feed carries (room-sign-mqtt.ts). Fixed-size so a bad message can't grow anything.
const uint8_t MAX_RUNNING = 4, MAX_TRACKS = 16;
struct Running { char name[24]; int pct; bool fwd; };
struct Track   { char device[20]; char output[3]; char mode[12]; int8_t power; };   // power: 1 on, 0 off, -1 unknown
struct ServerInfo { char version[16]; uint32_t uptimeS; bool serial, cloud; char standby[10]; uint8_t drivers, clients; };

#if defined(DISPLAY_SSD1306)
// ======================================================== OLED 128×64 🔲
#ifndef I2C_SDA
#define I2C_SDA 21
#endif
#ifndef I2C_SCL
#define I2C_SCL 22
#endif
#ifndef OLED_ADDR
#define OLED_ADDR 0x3C
#endif
#ifndef OLED_ROTATION
#define OLED_ROTATION 0            // 2 = upside down
#endif
#ifndef STATUS_LED_PIN
#define STATUS_LED_PIN 2           // -1 = no LED
#endif
const int16_t W = 128, H = 64;
const int16_t TITLE_BASE = 8;
const int16_t BODY_TOP = 11;                       // everything below the title is "body"
const int16_t DIGITS_BOTTOM = 56;                  // logisoso42 baseline
const int16_t BAR_Y = 60, BAR_H = 4, BAR_X = 2, BAR_W = W - 4;
const int16_t MSG1_BASE = 36, MSG2_BASE = 52;
#define FONT_TITLE  u8g2_font_6x10_tf
#define FONT_DIGITS u8g2_font_logisoso42_tn
#define FONT_MSG1   u8g2_font_helvB10_tf
#define FONT_MSG2   u8g2_font_6x10_tf
#define C_BG      SSD1306_BLACK
#define C_TITLE   SSD1306_WHITE
#define C_DIGITS  SSD1306_WHITE
#define C_TRACK   SSD1306_BLACK
#define C_BAR     SSD1306_WHITE
#define C_BAR_LOW SSD1306_WHITE
#define C_MUTED   SSD1306_WHITE
#define C_ALERT   SSD1306_WHITE
Adafruit_SSD1306 display(W, H, &Wire, -1);
Adafruit_GFX&    gfx = display;
bool displayOk = false;
// ⚠️ Until display.begin() has run there is no frame buffer — drawing would write to NULL.
inline bool canDraw() { return displayOk; }
uint32_t lastOledProbe = 0;
inline void flush() { if (displayOk) display.display(); }
// Adafruit_SSD1306::begin() returns true even with nothing on the bus — ask the bus ourselves.
bool oledPresent() { Wire.beginTransmission(OLED_ADDR); return Wire.endTransmission() == 0; }
#else
// ================================================= ESP32-C6-LCD-1.47 🖥️
const int8_t PIN_MOSI = 6, PIN_SCLK = 7, PIN_CS = 14, PIN_DC = 15, PIN_RST = 21, PIN_BL = 22;
const int8_t PIN_RGB  = 8;
const int8_t PIN_BUTTON = 9;          // BOOT, active low
const uint8_t ROTATION      = 1;      // 1 or 3 = landscape (USB-C left/right); 0/2 = portrait
const bool    INVERT_COLORS = true;   // IPS panel wants inversion on; flip if colours look negative
const uint8_t BACKLIGHT     = 220;    // 0–255
const uint8_t LED_BRIGHTNESS = 24;    // status LED cap (it is bright); 0 = LED off
const int16_t W = 320, H = 172;
// 🎨 Livery (dejajs.com): the mark + "DEJA.js" in the header, the cyan / magenta / lime stripes under it
const int16_t TITLE_BASE = 23;
const int16_t STRIPES_Y = 30;                      // cyan 4 · gap · magenta 2 · gap · lime 1
const int16_t BODY_TOP = 40;
const int16_t DIGITS_BOTTOM = 142;                 // logisoso92 baseline (full-width code, before the feed arrives)
const int16_t BAR_Y = 154, BAR_H = 8, BAR_X = 16, BAR_W = W - 32;
const int16_t MSG1_BASE = 98, MSG2_BASE = 126;
// JOIN page, two columns: QR box on the left, the code on the right
const int16_t QR_X = 10, QR_Y = BODY_TOP + 4, QR_BOX = H - BODY_TOP - 8;   // 124 px square
const int16_t COL_X = QR_X + QR_BOX + 10, COL_W = W - COL_X - 12;         // right column
const int16_t COL_LABEL_BASE = 60, COL_DIGITS_BOTTOM = 134, COL_BAR_Y = 150;
const int16_t COL_MSG1_BASE = 98, COL_MSG2_BASE = 122;
// list pages
const int16_t HEADING_BASE = 58, ROW_X = 16, ROW_R = W - 16;
#define FONT_TITLE   u8g2_font_helvB10_tf     // the layout name, after the wordmark
#define FONT_BRAND   u8g2_font_helvB14_tf     // DEJA
#define FONT_BRAND_JS u8g2_font_helvB10_tf    // .js
#define FONT_DIGITS  u8g2_font_logisoso92_tn
#define FONT_DIGITS_COL u8g2_font_logisoso62_tn
#define FONT_MSG1    u8g2_font_helvB18_tf
#define FONT_MSG2    u8g2_font_helvB12_tf
#define FONT_HEADING u8g2_font_helvB10_tf
#define FONT_ROW     u8g2_font_helvB14_tf
#define FONT_ROW_SM  u8g2_font_helvB12_tf
#define FONT_SMALL   u8g2_font_helvR10_tf
#define FONT_PILL    u8g2_font_helvB08_tf
// RGB565 — the website's livery: ground #0E1014, cyan #00C3FF, magenta #D500F9, lime #69FF00
#define C_BG      0x0882   // graphite ground
#define C_CYAN    0x061F
#define C_MAGENTA 0xD01F
#define C_LIME    0x6FE0
#define C_TITLE   C_CYAN   // accents: page dots, countdown bar
#define C_DIGITS  0xFFFF
#define C_TRACK   0x2124
#define C_BAR     C_CYAN
#define C_BAR_LOW 0xF800   // < 30 s left
#define C_MUTED   0x8C71
#define C_ALERT   0xF9A6
#define C_ON      C_LIME
#define C_OFF     0xF800   // red
#define C_WHITE   0xFFFF
#define C_BLACK   0x0000
SPIClass        spi(FSPI);
Adafruit_ST7789 display(&spi, PIN_CS, PIN_DC, PIN_RST);
Adafruit_GFX&   gfx = display;
inline void flush() {}
inline bool canDraw() { return true; }
#endif

// --------------------------------------------------------------- State 🧠

U8G2_FOR_ADAFRUIT_GFX u8g2;
WiFiClient    wifiClient;
PubSubClient  mqtt(wifiClient);

// room code
String   code;                 // "" until the first message
uint32_t expiresAt = 0;        // millis() when the current code runs out
uint32_t windowMs  = 300000;
uint32_t lastMsgAt = 0;
// sign feed
char       joinUrl[128] = "";
Running    running[MAX_RUNNING]; uint8_t runningN = 0;
Track      tracks[MAX_TRACKS];   uint8_t tracksN = 0;
ServerInfo server = {};
bool       allOff = false;     // every known track output is off
uint32_t   lastFeedAt = 0, feedRev = 0;
// connection + rendering
uint32_t lastWifiKick = 0, lastMqttTry = 0;
State    state = WIFI_CONNECTING, drawnState = (State)-1, reportedState = (State)-1;
Page     page = PAGE_JOIN, drawnPage = (Page)-1;
String   drawnCode, drawnJoin;
int16_t  drawnBarW = -1;
uint32_t drawnFeedRev = 0, drawnAt = 0, lastPressAt = 0;
bool     drawnFeedFresh = false, drawnAllOff = false;
uint32_t lastReport = 0;
char     topicCode[64], topicSign[64];

const char* stateName(State s) {
  switch (s) {
    case WIFI_CONNECTING:   return "wifi-connecting";
    case SERVER_CONNECTING: return "server-connecting";
    case WAITING_FOR_CODE:  return "waiting-for-code";
    case CODE_LIVE:         return "code-live";
    case CODE_ROTATING:     return "code-rotating";
    default:                return "server-offline";
  }
}
const char* pageName(Page p) {
  switch (p) {
    case PAGE_JOIN:    return "join";
    case PAGE_RUNNING: return "running";
    case PAGE_TRACKS:  return "tracks";
    default:           return "server";
  }
}
bool feedFresh(uint32_t now) { return lastFeedAt && (now - lastFeedAt < OFFLINE_AFTER_MS); }

// ----------------------------------------------------------------- LED 💡
#if defined(DISPLAY_SSD1306)
void ledFor(State s) {
  if (STATUS_LED_PIN < 0) return;
  uint32_t t = millis();
  bool on;
  switch (s) {
    case WIFI_CONNECTING:   on = (t / 200) & 1; break;            // fast blink
    case SERVER_CONNECTING: on = (t / 1000) & 1; break;           // slow blink
    case WAITING_FOR_CODE:
    case CODE_ROTATING:     on = true; break;                     // steady
    case CODE_LIVE:         on = false; break;                    // quiet while the code is up
    default:                on = ((t % 1000) < 100) || ((t % 1000) > 200 && (t % 1000) < 300); // double blink
  }
  digitalWrite(STATUS_LED_PIN, on ? HIGH : LOW);
}
#else
void led(uint8_t r, uint8_t g, uint8_t b) {
  rgbLedWrite(PIN_RGB, r * LED_BRIGHTNESS / 255, g * LED_BRIGHTNESS / 255, b * LED_BRIGHTNESS / 255);
}
void ledFor(State s) {
  bool blink = (millis() / 500) & 1;
  switch (s) {
    case WIFI_CONNECTING:   led(0, 0, blink ? 255 : 0); break;
    case SERVER_CONNECTING: led(blink ? 255 : 0, blink ? 120 : 0, 0); break;
    case WAITING_FOR_CODE:  led(255, 120, 0); break;
    case CODE_LIVE:         led(0, 255, 40); break;
    case CODE_ROTATING:     led(255, 120, 0); break;
    case SERVER_OFFLINE:    led(255, 0, 0); break;
  }
}
#endif

// ------------------------------------------------------------- Drawing 🎨
int16_t textWidth(const char* s) { return u8g2.getUTF8Width(s); }
void centred(const char* s, int16_t baseline, uint16_t colour) {
  u8g2.setForegroundColor(colour);
  u8g2.setCursor((W - textWidth(s)) / 2, baseline);
  u8g2.print(s);
}
void at(int16_t x, int16_t baseline, const char* s, uint16_t colour) {
  u8g2.setForegroundColor(colour);
  u8g2.setCursor(x, baseline);
  u8g2.print(s);
}
void rightAt(int16_t right, int16_t baseline, const char* s, uint16_t colour) { at(right - textWidth(s), baseline, s, colour); }
/** Print s at x, cut (with …) so it never runs past `right`. */
void fitted(int16_t x, int16_t right, int16_t baseline, const char* s, uint16_t colour) {
  char buf[32];
  strlcpy(buf, s, sizeof(buf));
  int16_t maxW = right - x;
  if (textWidth(buf) <= maxW) { at(x, baseline, buf, colour); return; }
  size_t n = strlen(buf);
  while (n > 1) {
    buf[--n] = '\0';
    buf[n] = '\0';
    char cut[36];
    snprintf(cut, sizeof(cut), "%s…", buf);
    if (textWidth(cut) <= maxW) { at(x, baseline, cut, colour); return; }
  }
  at(x, baseline, buf, colour);
}

void clearBody() { if (!canDraw()) return; gfx.fillRect(0, BODY_TOP, W, H - BODY_TOP, C_BG); }

void drawHeader() {
  if (!canDraw()) return;
  gfx.fillRect(0, 0, W, BODY_TOP, C_BG);
#if HAS_PAGES
  // 🚂 the mark, "DEJA.js", then the layout name — like the site's header
  gfx.drawRGBBitmap(8, 3, LOGO, LOGO_W, LOGO_H);
  int16_t x = 8 + LOGO_W + 7;
  u8g2.setFont(FONT_BRAND);    at(x, TITLE_BASE, "DEJA", C_WHITE);   x += textWidth("DEJA");
  u8g2.setFont(FONT_BRAND_JS); at(x, TITLE_BASE, ".js", C_MAGENTA);  x += textWidth(".js") + 12;
  // 📄 page dots, current one filled — and a red dot before them when every track output is off
  const int16_t r = 3, pitch = 11, y = TITLE_BASE - 5;
  int16_t dots = W - BAR_X - r - (PAGE_COUNT - 1) * pitch;
  u8g2.setFont(FONT_TITLE);
  fitted(x, dots - r - 26, TITLE_BASE, LAYOUT_NAME, C_MUTED);
  for (uint8_t p = 0, dx = 0; p < PAGE_COUNT; p++, dx += pitch) {
    if (p == page) gfx.fillCircle(dots + dx, y, r, C_TITLE);
    else           gfx.drawCircle(dots + dx, y, r, C_MUTED);
  }
  if (allOff) gfx.fillCircle(dots - r - 13, y, 4, C_OFF);
  // 🟦🟪🟩 the livery stripes
  gfx.fillRect(0, STRIPES_Y,     W, 4, C_CYAN);
  gfx.fillRect(0, STRIPES_Y + 5, W, 2, C_MAGENTA);
  gfx.fillRect(0, STRIPES_Y + 8, W, 1, C_LIME);
#else
  u8g2.setFont(FONT_TITLE);
  at(BAR_X, TITLE_BASE, LAYOUT_NAME, C_TITLE);
  const char* label = "ROOM CODE";
  if (textWidth(LAYOUT_NAME) + textWidth(label) + 3 * BAR_X < W) {   // only if both fit on the title row
    rightAt(W - BAR_X, TITLE_BASE, label, C_MUTED);
  }
#endif
  flush();
}

void drawMessage(const char* line1, const char* line2, uint16_t colour) {
  if (!canDraw()) return;
  clearBody();
  u8g2.setFont(FONT_MSG1);
  centred(line1, MSG1_BASE, colour);
  if (line2 && *line2) {
    u8g2.setFont(FONT_MSG2);
    centred(line2, MSG2_BASE, C_MUTED);
  }
  flush();
}

// Full-width code (the OLED, and the C6 before the sign feed has told it the join link)
void drawDigits() {
  if (!canDraw()) return;
  clearBody();
  u8g2.setFont(FONT_DIGITS);
  centred(code.c_str(), DIGITS_BOTTOM, C_DIGITS);
  gfx.fillRect(BAR_X, BAR_Y, BAR_W, BAR_H, C_TRACK);
  drawnBarW = -1;
}

void drawBarAt(int16_t x, int16_t y, int16_t w, int16_t h) {
  uint32_t now = millis();
  int32_t  left = (int32_t)(expiresAt - now);
  if (left < 0) left = 0;
  int16_t fill = (int16_t)((int64_t)w * left / (int64_t)windowMs);
  if (fill == drawnBarW) return;
  uint16_t colour = left < 30000 ? C_BAR_LOW : C_BAR;
  gfx.fillRect(x, y, w, h, C_TRACK);
  if (fill > 0) gfx.fillRect(x, y, fill, h, colour);
  drawnBarW = fill;
  flush();
}

#if HAS_PAGES
// ---------------------------------------------------------- QR + pages 📄
QRCode  qr;
uint8_t qrData[256];   // room for version 6 (41×41)

void drawQr() {
  // smallest version that holds the link; the module size then fills the box
  int8_t ok = -1;
  for (uint8_t v = 3; v <= 6 && ok != 0; v++) ok = qrcode_initText(&qr, qrData, v, ECC_MEDIUM, joinUrl);
  gfx.fillRect(QR_X, QR_Y, QR_BOX, QR_BOX, C_WHITE);
  if (ok != 0) { Serial.println("[qr] join link too long for a QR"); return; }
  int16_t scale = (QR_BOX - 8) / qr.size; if (scale < 1) scale = 1;
  int16_t side = scale * qr.size, x0 = QR_X + (QR_BOX - side) / 2, y0 = QR_Y + (QR_BOX - side) / 2;
  for (uint8_t y = 0; y < qr.size; y++)
    for (uint8_t x = 0; x < qr.size; x++)
      if (qrcode_getModule(&qr, x, y)) gfx.fillRect(x0 + x * scale, y0 + y * scale, scale, scale, C_BLACK);
  Serial.printf("[qr] version %d (%dx%d) at %d px/module\n", qr.version, qr.size, qr.size, scale);
}

void clearColumn() { gfx.fillRect(COL_X, BODY_TOP, W - COL_X, H - BODY_TOP, C_BG); }
void colCentred(const char* s, int16_t baseline, uint16_t colour) { at(COL_X + (COL_W - textWidth(s)) / 2, baseline, s, colour); }

/** JOIN page: QR on the left, the code's state in the right column. */
void drawJoin(bool withQr) {
  if (withQr) { clearBody(); drawQr(); }
  else clearColumn();
  switch (state) {
    case CODE_LIVE:
      u8g2.setFont(FONT_HEADING); colCentred("ROOM CODE", COL_LABEL_BASE, C_MUTED);
      u8g2.setFont(FONT_DIGITS_COL); colCentred(code.c_str(), COL_DIGITS_BOTTOM, C_DIGITS);
      gfx.fillRect(COL_X, COL_BAR_Y, COL_W, BAR_H, C_TRACK);
      drawnBarW = -1;
      break;
    case CODE_ROTATING:
      u8g2.setFont(FONT_ROW_SM); colCentred("New code in", COL_MSG1_BASE, C_MUTED); colCentred("a moment", COL_MSG2_BASE, C_MUTED); break;
    case WAITING_FOR_CODE:
      u8g2.setFont(FONT_ROW_SM); colCentred("Waiting for", COL_MSG1_BASE, C_MUTED); colCentred("the code", COL_MSG2_BASE, C_MUTED); break;
    case SERVER_OFFLINE:
      u8g2.setFont(FONT_ROW_SM); colCentred("Server offline", COL_MSG1_BASE, C_ALERT); colCentred("no code right now", COL_MSG2_BASE, C_MUTED); break;
    case SERVER_CONNECTING:
      u8g2.setFont(FONT_ROW_SM); colCentred("Looking for", COL_MSG1_BASE, C_MUTED); colCentred("the server", COL_MSG2_BASE, C_MUTED); break;
    default:
      u8g2.setFont(FONT_ROW_SM); colCentred("Connecting", COL_MSG1_BASE, C_MUTED); colCentred("to WiFi", COL_MSG2_BASE, C_MUTED); break;
  }
}

void heading(const char* s) { u8g2.setFont(FONT_HEADING); at(ROW_X, HEADING_BASE, s, C_MUTED); }
void offlineBody() { drawMessage("Server offline", "No news from the server for 90 s", C_ALERT); }

void triangle(int16_t x, int16_t cy, bool right, uint16_t colour) {
  if (right) gfx.fillTriangle(x, cy - 6, x, cy + 6, x + 10, cy, colour);
  else       gfx.fillTriangle(x + 10, cy - 6, x + 10, cy + 6, x, cy, colour);
}

void drawRunning() {
  clearBody();
  heading("NOW RUNNING");
  if (runningN == 0) {
    u8g2.setFont(FONT_MSG2);
    centred("Nothing moving right now", 112, C_MUTED);
    return;
  }
  const int16_t pitch = 26, base0 = 84;
  for (uint8_t i = 0; i < runningN; i++) {
    int16_t base = base0 + i * pitch;
    char pct[8]; snprintf(pct, sizeof(pct), "%d%%", running[i].pct);
    u8g2.setFont(FONT_ROW);
    rightAt(ROW_R, base, pct, C_DIGITS);
    int16_t pctW = textWidth("100%");
    triangle(ROW_R - pctW - 18, base - 6, running[i].fwd, running[i].fwd ? C_LIME : C_MAGENTA);
    fitted(ROW_X, ROW_R - pctW - 28, base, running[i].name, C_DIGITS);
  }
}

const char* modeLabel(const char* m) {
  if (!strcmp(m, "MAIN")) return "Main";
  if (!strcmp(m, "MAIN_AUTO")) return "Main auto-rev";
  if (!strcmp(m, "MAIN_INV")) return "Main inv";
  if (!strcmp(m, "PROG")) return "Prog";
  if (!strcmp(m, "DC")) return "DC";
  if (!strcmp(m, "DCX")) return "DC inv";
  if (!strncmp(m, "BOOST", 5)) return "Booster";
  return m;
}

void pill(int16_t right, int16_t base, int8_t power) {
  const int16_t w = 44, h = 18, x = right - w, y = base - 14;
  const char* txt = power == 1 ? "ON" : power == 0 ? "OFF" : "?";
  if (power == 1)      gfx.fillRoundRect(x, y, w, h, 9, C_ON);
  else if (power == 0) gfx.fillRoundRect(x, y, w, h, 9, C_OFF);
  else                 gfx.drawRoundRect(x, y, w, h, 9, C_MUTED);
  u8g2.setFont(FONT_PILL);
  at(x + (w - textWidth(txt)) / 2, base - 1, txt, power < 0 ? C_MUTED : C_BLACK);
}

void drawTracks() {
  clearBody();
  heading("TRACK POWER");
  if (tracksN == 0) {
    u8g2.setFont(FONT_MSG2);
    centred("No track outputs reported", 112, C_MUTED);
    return;
  }
  const uint8_t maxRows = 5;
  const int16_t pitch = 21, base0 = 80;
  uint8_t rows = tracksN < maxRows ? tracksN : maxRows;
  for (uint8_t i = 0; i < rows; i++) {
    int16_t base = base0 + i * pitch;
    char left[48];
    if (i == maxRows - 1 && tracksN > maxRows) snprintf(left, sizeof(left), "+%d more", tracksN - maxRows + 1);
    else snprintf(left, sizeof(left), "%s %s · %s", tracks[i].device, tracks[i].output, modeLabel(tracks[i].mode));
    u8g2.setFont(FONT_ROW_SM);
    fitted(ROW_X, ROW_R - 56, base, left, C_DIGITS);
    if (!(i == maxRows - 1 && tracksN > maxRows)) pill(ROW_R, base, tracks[i].power);
  }
}

void uptimeText(uint32_t s, char* out, size_t n) {
  uint32_t d = s / 86400, h = (s / 3600) % 24, m = (s / 60) % 60;
  if (d)      snprintf(out, n, "%lud %luh", (unsigned long)d, (unsigned long)h);
  else if (h) snprintf(out, n, "%luh %lum", (unsigned long)h, (unsigned long)m);
  else        snprintf(out, n, "%lum", (unsigned long)m);
}

void kv(int16_t x, int16_t base, const char* k, const char* v, uint16_t colour) {
  u8g2.setFont(FONT_SMALL); at(x, base, k, C_MUTED);
  u8g2.setFont(FONT_ROW_SM); at(x + 62, base, v, colour);
}

void drawServer() {
  clearBody();
  heading("SERVER");
  char buf[32];
  const int16_t L = ROW_X, R = W / 2 + 8, pitch = 22, b0 = 80;
  snprintf(buf, sizeof(buf), "v%s", server.version);          kv(L, b0, "Version", buf, C_DIGITS);
  uptimeText(server.uptimeS, buf, sizeof(buf));                kv(L, b0 + pitch, "Up", buf, C_DIGITS);
  kv(L, b0 + 2 * pitch, "Serial", server.serial ? "connected" : "no port", server.serial ? C_ON : C_OFF);
  kv(L, b0 + 3 * pitch, "Cloud",  server.cloud  ? "connected" : "offline", server.cloud  ? C_ON : C_OFF);
  const char* sb = !strcmp(server.standby, "standby") ? "Standby" : !strcmp(server.standby, "warning") ? "Standby soon" : "Awake";
  kv(R, b0, "State", sb, !strcmp(server.standby, "awake") ? C_ON : C_MAGENTA);
  snprintf(buf, sizeof(buf), "%u", server.drivers);            kv(R, b0 + pitch, "Drivers", buf, C_DIGITS);
  snprintf(buf, sizeof(buf), "%u", server.clients);            kv(R, b0 + 2 * pitch, "Clients", buf, C_DIGITS);
  snprintf(buf, sizeof(buf), "%d dBm", WiFi.RSSI());           kv(R, b0 + 3 * pitch, "WiFi", buf, C_DIGITS);
  u8g2.setFont(FONT_SMALL);
  snprintf(buf, sizeof(buf), "sign %s · server %s", WiFi.localIP().toString().c_str(), MQTT_HOST);
  at(ROW_X, H - 6, buf, C_MUTED);
}
#endif

void render() {
  if (!canDraw()) return;   // keep drawnState stale so the screen fills in the moment it attaches
  uint32_t now = millis();
  bool fresh = feedFresh(now);
#if HAS_PAGES
  bool pageChanged = page != drawnPage;
  if (pageChanged || allOff != drawnAllOff) { drawHeader(); drawnAllOff = allOff; }
  if (page == PAGE_JOIN) {
    bool haveQr = joinUrl[0] != '\0';
    bool joinChanged = drawnJoin != String(joinUrl);
    if (pageChanged || joinChanged || state != drawnState || (state == CODE_LIVE && code != drawnCode)) {
      if (haveQr) drawJoin(pageChanged || joinChanged);
      else switch (state) {   // no join link yet — the full-width code page
        case WIFI_CONNECTING:   drawMessage("Connecting to WiFi", WIFI_SSID, C_MUTED); break;
        case SERVER_CONNECTING: drawMessage("Looking for the server", MQTT_HOST, C_MUTED); break;
        case WAITING_FOR_CODE:  drawMessage("Waiting for the code", "", C_MUTED); break;
        case CODE_LIVE:         drawDigits(); break;
        case CODE_ROTATING:     drawMessage("New code in a moment", "", C_MUTED); break;
        case SERVER_OFFLINE:    drawMessage("Server offline", "No room code right now", C_ALERT); break;
      }
      drawnJoin = joinUrl;
    }
    if (state == CODE_LIVE) {
      if (haveQr) drawBarAt(COL_X, COL_BAR_Y, COL_W, BAR_H);
      else        drawBarAt(BAR_X, BAR_Y, BAR_W, BAR_H);
    }
  } else {
    // ⏱️ the server page carries uptime and WiFi strength — refresh it every 10 s on its own
    bool stale = page == PAGE_SERVER && now - drawnAt > 10000;
    if (pageChanged || feedRev != drawnFeedRev || fresh != drawnFeedFresh || stale) {
      if (!fresh)                    offlineBody();
      else if (page == PAGE_RUNNING) drawRunning();
      else if (page == PAGE_TRACKS)  drawTracks();
      else                           drawServer();
      drawnAt = now;
    }
  }
  drawnPage = page; drawnFeedRev = feedRev; drawnFeedFresh = fresh;
  drawnState = state; drawnCode = code;
#else
  (void)fresh;
  if (state != drawnState || (state == CODE_LIVE && code != drawnCode)) {
    switch (state) {
      case WIFI_CONNECTING:   drawMessage("Connecting to WiFi", WIFI_SSID, C_MUTED); break;
      case SERVER_CONNECTING: drawMessage("Looking for the server", MQTT_HOST, C_MUTED); break;
      case WAITING_FOR_CODE:  drawMessage("Waiting for the code", "", C_MUTED); break;
      case CODE_LIVE:         drawDigits(); break;
      case CODE_ROTATING:     drawMessage("New code in a moment", "", C_MUTED); break;
      case SERVER_OFFLINE:    drawMessage("Server offline", "No room code right now", C_ALERT); break;
    }
    drawnState = state;
    drawnCode  = code;
  }
  if (state == CODE_LIVE) drawBarAt(BAR_X, BAR_Y, BAR_W, BAR_H);
#endif
}

// ---------------------------------------------------------------- MQTT 📡
void onRoomCode(JsonDocument& doc) {
  const char* c = doc["code"];
  long secs = doc["secondsRemaining"] | 0L;
  long win  = doc["windowSeconds"]    | 300L;
  if (!c || secs <= 0) { Serial.println("[code] message without a live code"); return; }
  code      = c;
  expiresAt = millis() + (uint32_t)secs * 1000UL;
  windowMs  = (uint32_t)win * 1000UL;
  lastMsgAt = millis();
  Serial.printf("[code] %s (%ld s left)\n", c, secs);
}

void onSignFeed(JsonDocument& doc) {
  strlcpy(joinUrl, doc["join"] | "", sizeof(joinUrl));
  runningN = 0;
  for (JsonObject r : doc["running"].as<JsonArray>()) {
    if (runningN >= MAX_RUNNING) break;
    Running& it = running[runningN++];
    strlcpy(it.name, r["name"] | "", sizeof(it.name));
    it.pct = r["pct"] | 0;
    it.fwd = r["fwd"] | true;
  }
  tracksN = 0;
  uint8_t known = 0, off = 0;
  for (JsonObject t : doc["tracks"].as<JsonArray>()) {
    if (tracksN >= MAX_TRACKS) break;
    Track& it = tracks[tracksN++];
    strlcpy(it.device, t["device"] | "", sizeof(it.device));
    strlcpy(it.output, t["output"] | "", sizeof(it.output));
    strlcpy(it.mode,   t["mode"]   | "", sizeof(it.mode));
    JsonVariant p = t["power"];
    it.power = p.isNull() ? -1 : (p.as<bool>() ? 1 : 0);
    if (it.power >= 0) { known++; if (it.power == 0) off++; }
  }
  allOff = known > 0 && off == known;
  JsonObject s = doc["server"];
  strlcpy(server.version, s["version"] | "?", sizeof(server.version));
  server.uptimeS = s["uptimeS"] | 0UL;
  server.serial  = s["serial"]  | false;
  server.cloud   = s["cloud"]   | false;
  strlcpy(server.standby, s["standby"] | "awake", sizeof(server.standby));
  server.drivers = s["drivers"] | 0;
  server.clients = s["clients"] | 0;
  lastFeedAt = millis();
  feedRev++;
  Serial.printf("[feed] running=%u tracks=%u allOff=%d standby=%s drivers=%u clients=%u serial=%d cloud=%d v%s\n",
                runningN, tracksN, (int)allOff, server.standby, server.drivers, server.clients, (int)server.serial, (int)server.cloud, server.version);
}

void onMessage(char* t, byte* payload, unsigned int len) {
  JsonDocument doc;
  if (deserializeJson(doc, payload, len)) { Serial.printf("[mqtt] bad JSON on %s\n", t); return; }
  if (!strcmp(t, topicCode))      onRoomCode(doc);
  else if (!strcmp(t, topicSign)) onSignFeed(doc);
}

bool mqttConnect() {
  String id = "room-sign-" + WiFi.macAddress();
  id.replace(":", "");
  bool ok = strlen(MQTT_USERNAME) ? mqtt.connect(id.c_str(), MQTT_USERNAME, MQTT_PASSWORD) : mqtt.connect(id.c_str());
  if (ok) {
    mqtt.subscribe(topicCode);
    mqtt.subscribe(topicSign);
    Serial.printf("[mqtt] connected to %s:%d, subscribed %s + %s\n", MQTT_HOST, MQTT_PORT, topicCode, topicSign);
  } else {
    Serial.printf("[mqtt] connect failed rc=%d\n", mqtt.state());
  }
  return ok;
}

// -------------------------------------------------------------- Button 🔘
#if HAS_PAGES
void nextPage(uint32_t now) {
  page = (Page)((page + 1) % PAGE_COUNT);
  lastPressAt = now;
  Serial.printf("[page] %s\n", pageName(page));
}

void pollButton(uint32_t now) {
  static bool last = true, stable = true;
  static uint32_t changedAt = 0;
  bool raw = digitalRead(PIN_BUTTON);
  if (raw != last) { last = raw; changedAt = now; }
  else if (raw != stable && now - changedAt >= BUTTON_DEBOUNCE_MS) {
    stable = raw;
    if (stable == LOW) nextPage(now);   // pressed
  }
  // 🧪 `n` on the serial monitor pages too — handy when the board is on the bench, not the wall
  while (Serial.available()) if (Serial.read() == 'n') nextPage(now);
  if (page != PAGE_JOIN && now - lastPressAt > PAGE_HOME_AFTER_MS) {
    page = PAGE_JOIN;
    Serial.println("[page] join (idle)");
  }
}
#endif

// --------------------------------------------------------------- Setup 🚀
#if defined(DISPLAY_SSD1306)
// 🔌 Looks for the OLED now and, if it is not there, again every 5 s from loop() — so the
// screen can be wired up live (or swapped) without resetting the board.
void attachOled() {
  lastOledProbe = millis();
  if (!oledPresent()) {
    if (displayOk) Serial.println("[oled] lost the SSD1306 — check the wires");
    displayOk = false;
    return;
  }
  if (displayOk) return;
  displayOk = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR, true, false);   // Wire is already begun with our pins
  if (!displayOk) { Serial.println("[oled] SSD1306 answered but init failed"); return; }
  display.setRotation(OLED_ROTATION);
  Serial.printf("[oled] SSD1306 128x64 at 0x%02X on SDA %d / SCL %d\n", OLED_ADDR, I2C_SDA, I2C_SCL);
  u8g2.begin(gfx);
  u8g2.setFontMode(1);
  u8g2.setFontDirection(0);
  drawHeader();
  drawnState = (State)-1;   // force the body to redraw on the fresh screen
}
#endif

void setupDisplay() {
#if defined(DISPLAY_SSD1306)
  if (STATUS_LED_PIN >= 0) pinMode(STATUS_LED_PIN, OUTPUT);
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);
  attachOled();
#else
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_BL, OUTPUT);
  analogWrite(PIN_BL, BACKLIGHT);
  spi.begin(PIN_SCLK, -1, PIN_MOSI, PIN_CS);
  display.init(172, 320);
  display.setSPISpeed(40000000);
  display.setRotation(ROTATION);
  display.invertDisplay(INVERT_COLORS);
#endif
  u8g2.begin(gfx);
  u8g2.setFontMode(1);          // transparent text, we clear regions ourselves
  u8g2.setFontDirection(0);
  drawHeader();
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("[boot] room-code sign");
  setupDisplay();

  snprintf(topicCode, sizeof(topicCode), "DEJA/%s/roomCode", LAYOUT_ID);
  snprintf(topicSign, sizeof(topicSign), "DEJA/%s/roomSign", LAYOUT_ID);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastWifiKick = millis();
  Serial.printf("[wifi] connecting to %s\n", WIFI_SSID);

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMessage);
  mqtt.setBufferSize(2048);     // the sign feed is a few hundred bytes; room to grow
  mqtt.setSocketTimeout(5);
  mqtt.setKeepAlive(30);
}

// ---------------------------------------------------------------- Loop 🔁
void loop() {
  uint32_t now = millis();

  if (WiFi.status() != WL_CONNECTED) {
    state = WIFI_CONNECTING;
    if (now - lastWifiKick > WIFI_RETRY_MS) { WiFi.disconnect(); WiFi.begin(WIFI_SSID, WIFI_PASSWORD); lastWifiKick = now; }
  } else {
    if (state == WIFI_CONNECTING) Serial.printf("[wifi] connected, ip=%s\n", WiFi.localIP().toString().c_str());
    if (!mqtt.connected()) {
      state = SERVER_CONNECTING;
      if (now - lastMqttTry > MQTT_RETRY_MS) { lastMqttTry = now; mqttConnect(); }
    } else {
      mqtt.loop();
      now = millis();   // the callback above may have just stamped lastMsgAt — never compare against an older `now`
      bool fresh = lastMsgAt && (now - lastMsgAt < OFFLINE_AFTER_MS);
      bool live  = code.length() && (int32_t)(expiresAt - now) > 0;
      if (fresh && live)        state = CODE_LIVE;
      else if (fresh)           state = CODE_ROTATING;    // ran out; the server sends the next one a couple of seconds after
      else if (!lastMsgAt)      state = WAITING_FOR_CODE;
      else                      state = SERVER_OFFLINE;
    }
  }

  // 🪵 One line per state change, plus a heartbeat every 10 s so a serial monitor always shows signs of life
  if (state != reportedState) { Serial.printf("[state] %s\n", stateName(state)); reportedState = state; }
  if (now - lastReport > 10000) {
    lastReport = now;
    Serial.printf("[status] %s page=%s wifi=%d rssi=%d mqtt=%d code=%s left=%lds feed=%lds-ago up=%lus\n", stateName(state), pageName(page), (int)WiFi.status(), WiFi.RSSI(), mqtt.state(),
                  code.length() ? code.c_str() : "-", code.length() ? (long)((int32_t)(expiresAt - now) / 1000) : 0L,
                  lastFeedAt ? (long)((now - lastFeedAt) / 1000) : -1L, (unsigned long)(now / 1000));
  }

#if defined(DISPLAY_SSD1306)
  if (!displayOk && now - lastOledProbe > 5000) {
    attachOled();
    static uint8_t probes = 0;
    if (!displayOk && probes++ % 6 == 0) Serial.printf("[oled] no SSD1306 at 0x%02X (SDA %d / SCL %d) — still looking every 5 s\n", OLED_ADDR, I2C_SDA, I2C_SCL);
  }
#endif
#if HAS_PAGES
  pollButton(now);
#endif

  render();
  ledFor(state);
  delay(20);
}
