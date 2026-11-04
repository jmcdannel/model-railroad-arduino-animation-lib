/*
 * 🎛️ Servo Tuner — Arduino Nano
 * ------------------------------
 * A bench tool for turnout and gate servos:
 *   - centers every servo at power-up, so you can fit the horn straight
 *   - nudges a servo a degree at a time to find its two end points
 *   - swings it between them to check the throw before you glue it down
 *   - saves the end points and prints them ready to paste into DEJA or the
 *     crossing-gates sketch
 *
 * Two ways to drive it, use either or both:
 *   💬 Serial Monitor at 115200 (type ? and Enter for the commands)
 *   🎚️ a knob and a button, no computer needed
 *
 * Servos on Nano pins, or on a PCA9685 16-channel board (what DEJA's own
 * firmware uses). Angles map to pulses exactly the way DEJA does it, so the
 * numbers you find here go straight into a DEJA turnout.
 *
 * Wiring:
 *   Servo signal → D9, D10 (or PCA9685 channels; SDA → A4, SCL → A5)
 *   Servo + → a 5 V supply, NOT the Nano's 5V pin (servos stall the Nano)
 *   Servo − → supply GND, and supply GND → Nano GND
 *   Knob (10 kΩ pot): ends → 5V and GND, wiper → A0      (optional)
 *   Button: D2 → button → GND                              (optional)
 *   Mark button: D3 → button → GND                         (optional)
 */
#include <EEPROM.h>

// ====================================================== 1. Servos 🦾
#define DRIVER_PINS    0            // servos straight on Nano pins
#define DRIVER_PCA9685 1            // a PCA9685 board over I2C
#define DRIVER DRIVER_PINS

const uint8_t SERVO_PINS[] = { 9, 10 };   // DRIVER_PINS: any pins
#define PCA9685_ADDR   0x40               // DRIVER_PCA9685: the board's address…
#define PCA9685_COUNT  16                 // …and how many channels to tune

// ===================================================== 2. Pulses 📐
// 0° and 180° in microseconds. 650–2350 matches DEJA's firmware.
// (Arduino's Servo.write() uses 544–2400, so the same angle sits a little
// further out there. The crossing-gates sketch lets you set either.)
#define PULSE_MIN_US   650
#define PULSE_MAX_US   2350
#define CENTER_DEG     90
const bool CENTER_ON_BOOT = true;   // false = go to each servo's end A instead
const float SLOW_DEG_PER_S = 60;    // speed for toggle and swing moves

// ============================================ 3. Knob + buttons 🎚️
// -1 = not fitted. The button cycles the selected servo through
// center → knob → swing; a long press (1 s) selects the next servo.
// The mark button saves the knob position as end A, then end B.
#define KNOB_PIN       -1           // e.g. A0
#define BUTTON_PIN     -1           // e.g. 2
#define MARK_PIN       -1           // e.g. 3

// ================================================================
// ============================ the sketch ========================
// ================================================================

#if DRIVER == DRIVER_PCA9685
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>   // library: Adafruit PWM Servo Driver
Adafruit_PWMServoDriver pca(PCA9685_ADDR);
const uint8_t COUNT = PCA9685_COUNT;
#else
#include <Servo.h>
const uint8_t COUNT = sizeof SERVO_PINS;
Servo servos[sizeof SERVO_PINS];
#endif

const uint8_t UNSET = 255;
struct Tuned { uint8_t endA = UNSET, endB = UNSET; };
Tuned ends[COUNT];

enum Mode : uint8_t { HOLD, KNOB, SWING, LIMP };
float   pos[COUNT];                 // where each servo is (degrees)
float   target[COUNT];              // where it's heading, for slow moves
Mode    mode[COUNT];
bool    swingToB[COUNT];
uint8_t sel = 0;                    // the servo the commands act on
bool    markB = false;              // next mark button press sets end B

uint16_t pulseUs(float deg) { return PULSE_MIN_US + (uint16_t)((PULSE_MAX_US - PULSE_MIN_US) * deg / 180 + 0.5f); }

// ---------------------------------------------------- driving 🦾
void drive(uint8_t i, float deg) {
  pos[i] = deg;
#if DRIVER == DRIVER_PCA9685
  // same maths as DEJA's getPulseWidth(): µs → 12-bit ticks at 50 Hz
  pca.setPWM(i, 0, (uint16_t)(pulseUs(deg) / 1e6 * 50 * 4096));
#else
  if (!servos[i].attached()) servos[i].attach(SERVO_PINS[i], PULSE_MIN_US, PULSE_MAX_US);
  servos[i].writeMicroseconds(pulseUs(deg));
#endif
}

void limp(uint8_t i) {
#if DRIVER == DRIVER_PCA9685
  pca.setPWM(i, 0, 0);
#else
  servos[i].detach();
#endif
  mode[i] = LIMP;
}

void goTo(uint8_t i, float deg) {   // straight there
  deg = constrain(deg, 0, 180);
  mode[i] = HOLD;
  target[i] = deg;
  drive(i, deg);
}

void glideTo(uint8_t i, float deg) { // at SLOW_DEG_PER_S
  if (mode[i] == LIMP) drive(i, pos[i]);
  mode[i] = HOLD;
  target[i] = constrain(deg, 0, 180);
}

// --------------------------------------------------------- saving 💾
const uint8_t MAGIC = 0x5E;         // EEPROM byte 0: "these are ours"

void load() {
  if (EEPROM.read(0) != MAGIC) return;
  for (uint8_t i = 0; i < COUNT; i++) {
    ends[i].endA = EEPROM.read(1 + i * 2);
    ends[i].endB = EEPROM.read(2 + i * 2);
  }
}

void save() {
  EEPROM.update(0, MAGIC);
  for (uint8_t i = 0; i < COUNT; i++) {
    EEPROM.update(1 + i * 2, ends[i].endA);
    EEPROM.update(2 + i * 2, ends[i].endB);
  }
  Serial.println(F("💾 Saved. The end points survive power-off."));
}

// -------------------------------------------------------- printing 💬
void printDeg(uint8_t d) {
  if (d == UNSET) Serial.print(F("--"));
  else Serial.print(d);
}

void printWhere() {
  Serial.print(F("Servo ")); Serial.print(sel);
  Serial.print(F(": ")); Serial.print((int)(pos[sel] + 0.5));
  Serial.print(F("° (")); Serial.print(pulseUs(pos[sel])); Serial.println(F(" µs)"));
}

void printAll() {
  Serial.println();
  for (uint8_t i = 0; i < COUNT; i++) {
    Tuned &t = ends[i];
    Serial.print(i == sel ? F("▶ Servo ") : F("  Servo ")); Serial.print(i);
#if DRIVER == DRIVER_PINS
    Serial.print(F(" (pin ")); Serial.print(SERVO_PINS[i]); Serial.print(')');
#endif
    Serial.print(F("  A ")); printDeg(t.endA);
    Serial.print(F("°  B ")); printDeg(t.endB); Serial.println(F("°"));
    if (t.endA == UNSET || t.endB == UNSET) continue;
    Serial.print(F("    DEJA turnout   → straight: ")); Serial.print(t.endA);
    Serial.print(F("  divergent: ")); Serial.println(t.endB);
    Serial.print(F("    crossing gates → c.upDeg = ")); Serial.print(t.endA);
    Serial.print(F("; c.downDeg = ")); Serial.print(t.endB);
    Serial.print(F("; c.minUs = " )); Serial.print(PULSE_MIN_US);
    Serial.print(F("; c.maxUs = ")); Serial.print(PULSE_MAX_US); Serial.println(';');
  }
  Serial.println();
}

void help() {
  Serial.println(F(
    "\n🎛️ Servo tuner. Type a command and press Enter:\n"
    "  s3       select servo 3            90    go to 90°\n"
    "  + / -    nudge 1° (+++ = 3°)       ] / [ nudge 10°\n"
    "  c        center                    l     limp (stop driving it)\n"
    "  a / b    this spot is end A / B    t     glide to the other end\n"
    "  w        swing A <-> B until the next command\n"
    "  p        print every servo's numbers    e  save to EEPROM\n"
    "End A = straight / gate up, end B = divergent / gate down."));
}

// -------------------------------------------------------- commands ⌨️
void mark(bool isB) {
  uint8_t d = (uint8_t)(pos[sel] + 0.5);
  (isB ? ends[sel].endB : ends[sel].endA) = d;
  Serial.print(F("📍 Servo ")); Serial.print(sel);
  Serial.print(isB ? F(" end B = ") : F(" end A = ")); Serial.print(d);
  Serial.println(F("°  (e to save)"));
}

void toggle() {
  Tuned &t = ends[sel];
  if (t.endA == UNSET || t.endB == UNSET) { Serial.println(F("Set end A and end B first (a, b).")); return; }
  bool toB = abs(target[sel] - t.endA) < abs(target[sel] - t.endB);
  glideTo(sel, toB ? t.endB : t.endA);
  Serial.println(toB ? F("→ B") : F("→ A"));
}

void swing() {
  Tuned &t = ends[sel];
  if (t.endA == UNSET || t.endB == UNSET) { Serial.println(F("Set end A and end B first (a, b).")); return; }
  glideTo(sel, t.endB);
  mode[sel] = SWING; swingToB[sel] = true;
  Serial.println(F("🔁 Swinging. Any command stops it."));
}

void command(char *line) {
  while (*line == ' ') line++;
  if (!*line) return;
  if (mode[sel] == SWING) { mode[sel] = HOLD; target[sel] = pos[sel]; }  // any command stops a swing
  if (isdigit(*line)) { goTo(sel, atoi(line)); printWhere(); return; }
  if (*line == 's') {
    int n = atoi(line + 1);
    if (n < 0 || n >= COUNT) { Serial.print(F("Servos are 0 to ")); Serial.println(COUNT - 1); return; }
    sel = n; printWhere(); return;
  }
  // everything else is one letter, repeatable: "+++" nudges 3°
  float nudge = 0;
  for (char *c = line; *c; c++) {
    switch (*c) {
      case '+': nudge += 1; break;
      case '-': nudge -= 1; break;
      case ']': nudge += 10; break;
      case '[': nudge -= 10; break;
      case 'c': goTo(sel, CENTER_DEG); printWhere(); break;
      case 'a': mark(false); break;
      case 'b': mark(true); break;
      case 't': toggle(); break;
      case 'w': swing(); break;
      case 'l': limp(sel); Serial.println(F("Limp. Any move drives it again.")); break;
      case 'p': printAll(); break;
      case 'e': save(); break;
      case '?': case 'h': help(); break;
      default: break;
    }
  }
  if (nudge) { goTo(sel, pos[sel] + nudge); printWhere(); }
}

void readSerial() {
  static char line[16];
  static uint8_t len = 0;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') { line[len] = 0; command(line); len = 0; }
    else if (len < sizeof line - 1) line[len++] = c;
  }
}

// ------------------------------------------------- knob + buttons 🎚️
#if BUTTON_PIN >= 0 || MARK_PIN >= 0
// Returns 1 for a short press, 2 for a long press (1 s), on release.
uint8_t pressed(int8_t pin, bool &wasDown, uint32_t &downAt, uint32_t now) {
  bool down = digitalRead(pin) == LOW;
  uint8_t result = 0;
  if (down && !wasDown && now - downAt > 30) downAt = now;      // debounce
  if (!down && wasDown && now - downAt > 30) result = now - downAt >= 1000 ? 2 : 1;
  wasDown = down;
  return result;
}
#endif

void controls(uint32_t now) {
#if BUTTON_PIN >= 0
  static bool bDown = false; static uint32_t bAt = 0;
  uint8_t b = pressed(BUTTON_PIN, bDown, bAt, now);
  if (b == 2) { sel = (sel + 1) % COUNT; goTo(sel, pos[sel]); printWhere(); }
  if (b == 1) {
    Mode next = mode[sel] == KNOB ? SWING : mode[sel] == SWING ? HOLD : KNOB;
    if (next == HOLD) { goTo(sel, CENTER_DEG); Serial.println(F("🎯 center")); }
    else if (next == KNOB) {
      if (KNOB_PIN < 0) { goTo(sel, CENTER_DEG); }
      else { mode[sel] = KNOB; Serial.println(F("🎚️ knob")); }
    } else swing();
  }
#endif
#if MARK_PIN >= 0
  static bool mDown = false; static uint32_t mAt = 0;
  if (pressed(MARK_PIN, mDown, mAt, now)) { mark(markB); markB = !markB; save(); }
#endif
#if KNOB_PIN >= 0
  if (mode[sel] == KNOB) {
    static int lastRaw = -100;
    int raw = analogRead(KNOB_PIN);
    if (abs(raw - lastRaw) > 4) {                 // ignore wiper jitter
      lastRaw = raw;
      drive(sel, target[sel] = (uint8_t)map(raw, 0, 1023, 0, 180));
      static uint32_t lastPrint = 0;
      if (now - lastPrint > 250) { lastPrint = now; printWhere(); }
    }
  }
#endif
  (void)now;
}

// ------------------------------------------------- slow moves 🐢
void glide(uint32_t now) {
  static uint32_t last = 0;
  float step = SLOW_DEG_PER_S * (now - last) / 1000.0;
  last = now;
  for (uint8_t i = 0; i < COUNT; i++) {
    if (mode[i] == LIMP || mode[i] == KNOB) continue;
    if (pos[i] != target[i]) {
      float d = target[i] - pos[i];
      drive(i, abs(d) <= step ? target[i] : pos[i] + (d > 0 ? step : -step));
    } else if (mode[i] == SWING) {
      static uint32_t arrived[COUNT];
      if (!arrived[i]) arrived[i] = now | 1;
      if (now - arrived[i] > 1000) {               // pause at each end
        arrived[i] = 0;
        swingToB[i] = !swingToB[i];
        target[i] = swingToB[i] ? ends[i].endB : ends[i].endA;
      }
    }
  }
}

// ---------------------------------------------------- setup + loop 🔁
void setup() {
  Serial.begin(115200);
#if DRIVER == DRIVER_PCA9685
  pca.begin();
  pca.setPWMFreq(50);
#endif
#if BUTTON_PIN >= 0
  pinMode(BUTTON_PIN, INPUT_PULLUP);
#endif
#if MARK_PIN >= 0
  pinMode(MARK_PIN, INPUT_PULLUP);
#endif
  load();
  for (uint8_t i = 0; i < COUNT; i++) {
    bool hasA = ends[i].endA != UNSET;
    goTo(i, CENTER_ON_BOOT || !hasA ? CENTER_DEG : ends[i].endA);
  }
  help();
  printAll();
}

void loop() {
  uint32_t now = millis();
  readSerial();
  controls(now);
  glide(now);
}
