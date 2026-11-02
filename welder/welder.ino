/*
 * 🔥 Arc Welder Effect — Arduino Nano
 * ------------------------------------
 * Drop it in a shop/industry building. Blue + white LEDs flicker like an arc,
 * then a red LED glows and slowly cools once the welder stops.
 *
 * Sequence (repeats forever once powered):
 *   IDLE   → dark, random pause (welder repositioning / grabbing a new piece)
 *   STRIKE → 1–4 short white pops as the rod tries to catch
 *   WELD   → several arc "bursts": white + blue flicker at independent random
 *            rates, with short pauses between bursts (moving along the seam)
 *   …then back to IDLE. The red LED is driven by a tiny thermal model: it heats
 *   up while the arc is running and cools down exponentially afterwards, so the
 *   red glow lingers and fades naturally after the last burst.
 *
 * Fully non-blocking (no delay()), so all three LEDs run independently.
 * Optional D12 trigger (DEJA IO pin) — see the External trigger block.
 *
 * Wiring (each LED: pin → resistor → anode, cathode → GND):
 *   D9  → 220 Ω → white LED
 *   D10 → 220 Ω → blue LED
 *   D11 → 150–220 Ω → red LED
 *   (D9/D10/D11 are all PWM pins on the Nano.)
 *
 * Based on the classic model-railroad arc-welder sketches by Steve Spence,
 * Paul & David Bradt (Model Railroader SN230) and the Cornhill & Atherton
 * finite-state-machine version on Model Railroad Hobbyist. Rewritten from
 * scratch with PWM intensity, burst structure and a thermal red glow.
 */

// ---------------------------------------------------------------- Pins 📌
const uint8_t PIN_WHITE = 9;
const uint8_t PIN_BLUE  = 10;
const uint8_t PIN_RED   = 11;

// ------------------------------------------- External trigger (DEJA IO pin) 🎛️
// Off by default. Set USE_TRIGGER = true and run one wire from a DEJA IO pin
// (or a relay contact / toggle switch) to D12, plus GND to GND. HIGH = effect
// runs, matching DEJA's "on". Unplugged reads HIGH, so the board still runs
// on its own. Contact-to-GND or inverted logic: set TRIGGER_ACTIVE_LOW = true.
// When off: no new jobs start; a job in progress stops and the red glow cools down.
const uint8_t  PIN_TRIGGER         = 12;
const bool     USE_TRIGGER         = false;
const bool     TRIGGER_ACTIVE_LOW  = false;
const unsigned TRIGGER_DEBOUNCE_MS = 30;

bool triggered(unsigned long now) {
  if (!USE_TRIGGER) return true;
  static bool seeded = false, stable = true, lastRaw = true;
  static unsigned long changedAt = 0;
  bool raw = (digitalRead(PIN_TRIGGER) == LOW) == TRIGGER_ACTIVE_LOW;
  if (!seeded) { seeded = true; stable = lastRaw = raw; changedAt = now; }   // start from the real pin state
  if (raw != lastRaw) { lastRaw = raw; changedAt = now; }
  else if (raw != stable && now - changedAt >= TRIGGER_DEBOUNCE_MS) stable = raw;
  return stable;
}

// ------------------------------------------------------------- Tuning 🎛️
// All times are milliseconds. Every range is picked randomly each time.

// Dark gap between weld jobs
const unsigned long IDLE_MIN = 3000;
const unsigned long IDLE_MAX = 12000;

// Strike attempts before the arc catches
const uint8_t  STRIKES_MIN    = 1;
const uint8_t  STRIKES_MAX    = 4;
const unsigned STRIKE_ON_MIN  = 30;
const unsigned STRIKE_ON_MAX  = 110;
const unsigned STRIKE_OFF_MIN = 200;
const unsigned STRIKE_OFF_MAX = 700;

// A weld job is a series of arc bursts separated by short pauses
const unsigned long JOB_MIN   = 6000;
const unsigned long JOB_MAX   = 16000;
const unsigned      BURST_MIN = 700;
const unsigned      BURST_MAX = 4000;
const unsigned      PAUSE_MIN = 150;
const unsigned      PAUSE_MAX = 900;

// White arc flicker (fast, bright, mostly on)
const unsigned WHITE_ON_MIN  = 5;
const unsigned WHITE_ON_MAX  = 60;
const unsigned WHITE_OFF_MIN = 5;
const unsigned WHITE_OFF_MAX = 110;
const uint8_t  WHITE_LVL_MIN = 110;   // PWM 0–255
const uint8_t  WHITE_LVL_MAX = 255;

// Blue arc flicker (sparser, snappier)
const unsigned BLUE_ON_MIN  = 3;
const unsigned BLUE_ON_MAX  = 25;
const unsigned BLUE_OFF_MIN = 10;
const unsigned BLUE_OFF_MAX = 220;
const uint8_t  BLUE_LVL_MIN = 140;
const uint8_t  BLUE_LVL_MAX = 255;

// Red glow thermal model
const uint8_t  RED_MAX        = 255;   // brightest the glow gets
const float    HEAT_UP_MS     = 3500;  // ms of arc to reach full heat
const float    COOL_TAU_MS    = 2800;  // bigger = slower, longer fade
const float    HEAT_FLOOR     = 0.01;  // below this the LED is just off

const bool DEBUG = false;              // true → state changes on Serial @ 9600

// ------------------------------------------------------- Flicker engine ⚡
// One independent random on/off flasher per LED. Call update() every loop;
// pass active=false to force it dark.
struct Flicker {
  uint8_t  pin;
  unsigned onMin, onMax, offMin, offMax;
  uint8_t  lvlMin, lvlMax;
  bool     on;
  unsigned long next;

  void update(unsigned long now, bool active) {
    if (!active) {
      if (on) { analogWrite(pin, 0); on = false; }
      next = now;                       // fire immediately on next activation
      return;
    }
    if ((long)(now - next) < 0) return; // not time yet (wrap-safe)
    on = !on;
    if (on) {
      analogWrite(pin, random(lvlMin, lvlMax + 1));
      next = now + random(onMin, onMax + 1);
    } else {
      analogWrite(pin, 0);
      next = now + random(offMin, offMax + 1);
    }
  }
};

Flicker white = { PIN_WHITE, WHITE_ON_MIN, WHITE_ON_MAX, WHITE_OFF_MIN, WHITE_OFF_MAX, WHITE_LVL_MIN, WHITE_LVL_MAX, false, 0 };
Flicker blue  = { PIN_BLUE,  BLUE_ON_MIN,  BLUE_ON_MAX,  BLUE_OFF_MIN,  BLUE_OFF_MAX,  BLUE_LVL_MIN,  BLUE_LVL_MAX,  false, 0 };

// --------------------------------------------------------- State machine 🔁
enum State : uint8_t { IDLE, STRIKE, WELD };
State state = IDLE;

unsigned long stateUntil   = 0;   // when the current state ends
unsigned long phaseUntil   = 0;   // sub-timer: strike pop / burst / pause
uint8_t       strikesLeft  = 0;
bool          strikeLit    = false;
bool          arcOn        = false;   // true during an arc burst

float         heat         = 0.0;     // 0.0 (cold) … 1.0 (white hot)
unsigned long lastTick     = 0;

void logState(const char *s) {
  if (DEBUG) { Serial.print(millis()); Serial.print("  "); Serial.println(s); }
}

void enterIdle(unsigned long now) {
  state = IDLE;
  arcOn = false;
  stateUntil = now + random(IDLE_MIN, IDLE_MAX + 1);
  logState("IDLE");
}

void enterStrike(unsigned long now) {
  state = STRIKE;
  strikesLeft = random(STRIKES_MIN, STRIKES_MAX + 1);
  strikeLit = false;
  phaseUntil = now;                  // first pop right away
  logState("STRIKE");
}

void enterWeld(unsigned long now) {
  state = WELD;
  stateUntil = now + random(JOB_MIN, JOB_MAX + 1);
  arcOn = true;
  phaseUntil = now + random(BURST_MIN, BURST_MAX + 1);
  logState("WELD");
}

void setup() {
  pinMode(PIN_WHITE, OUTPUT);
  pinMode(PIN_BLUE,  OUTPUT);
  pinMode(PIN_RED,   OUTPUT);
  pinMode(PIN_TRIGGER, INPUT_PULLUP);
  analogWrite(PIN_WHITE, 0);
  analogWrite(PIN_BLUE,  0);
  analogWrite(PIN_RED,   0);

  if (DEBUG) Serial.begin(9600);
  randomSeed(analogRead(A0));        // floating pin → different show every power-up

  lastTick = millis();
  enterIdle(lastTick);
  stateUntil = lastTick + 1500;      // short first wait so you see it start
}

void loop() {
  unsigned long now = millis();
  bool run = triggered(now);

  // ---- main sequence -----------------------------------------------------
  switch (state) {

    case IDLE:
      if (run && (long)(now - stateUntil) >= 0) enterStrike(now);
      break;

    case STRIKE:
      if (!run) { analogWrite(PIN_WHITE, 0); enterIdle(now); break; }
      if ((long)(now - phaseUntil) >= 0) {
        if (strikeLit) {                       // pop finished
          analogWrite(PIN_WHITE, 0);
          strikeLit = false;
          strikesLeft--;
          if (strikesLeft == 0) { enterWeld(now); break; }
          phaseUntil = now + random(STRIKE_OFF_MIN, STRIKE_OFF_MAX + 1);
        } else {                               // start a pop
          analogWrite(PIN_WHITE, random(WHITE_LVL_MIN, WHITE_LVL_MAX + 1));
          strikeLit = true;
          phaseUntil = now + random(STRIKE_ON_MIN, STRIKE_ON_MAX + 1);
        }
      }
      break;

    case WELD:
      if (!run || (long)(now - stateUntil) >= 0) { enterIdle(now); break; }
      if ((long)(now - phaseUntil) >= 0) {     // toggle burst ↔ pause
        arcOn = !arcOn;
        phaseUntil = now + (arcOn ? random(BURST_MIN, BURST_MAX + 1)
                                  : random(PAUSE_MIN, PAUSE_MAX + 1));
      }
      break;
  }

  // ---- arc flicker (white + blue run independently while a burst is on) --
  bool arcActive = (state == WELD) && arcOn;
  white.update(now, arcActive);
  blue.update(now, arcActive);

  // ---- red glow: heat while arcing, exponential cool-down otherwise ------
  unsigned long dt = now - lastTick;
  lastTick = now;
  if (dt) {
    if (arcActive) {
      heat += dt / HEAT_UP_MS;
      if (heat > 1.0) heat = 1.0;
    } else {
      heat -= heat * (dt / COOL_TAU_MS);   // first-order cooling
      if (heat < HEAT_FLOOR) heat = 0.0;
    }
    // gamma-ish curve so the fade looks even to the eye
    analogWrite(PIN_RED, (uint8_t)(heat * heat * RED_MAX + 0.5));
  }
}
