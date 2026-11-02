/*
 * 🔥 Campfire / Fireplace Flicker — Arduino Nano
 * ------------------------------------------------
 * Three LEDs flicker like a fire: two "flame" LEDs (yellow + orange) dance
 * quickly and independently while a red "ember" LED breathes slowly
 * underneath. Random gusts flare the whole fire up; lulls let it settle.
 *
 * Works for a campfire, hobo barrel, fireplace behind a window, caboose
 * stove, forge, trash fire — anything that burns.
 *
 * Wiring (pin → resistor → LED anode, cathode → GND):
 *   D9  → 220 Ω → yellow LED        (flame 1)
 *   D10 → 220 Ω → orange/amber LED  (flame 2)
 *   D11 → 150–220 Ω → red LED       (ember)
 * Tip: bunch the LEDs together and diffuse them (tissue paper, a dab of
 * hot glue) so the colors blend into one flame.
 */

// ---------------------------------------------------------------- Pins 📌
const uint8_t PIN_FLAME1 = 9;
const uint8_t PIN_FLAME2 = 10;
const uint8_t PIN_EMBER  = 11;

// ------------------------------------------- External trigger (DEJA IO pin) 🎛️
// Off by default. Set USE_TRIGGER = true and run one wire from a DEJA IO pin
// (or a relay contact / toggle switch) to D12, plus GND to GND. HIGH = effect
// runs, matching DEJA's "on". Unplugged reads HIGH, so the board still runs
// on its own. Contact-to-GND or inverted logic: set TRIGGER_ACTIVE_LOW = true.
// When off: the fire fades out; it relights the moment the trigger returns.
const uint8_t  PIN_TRIGGER         = 12;
const bool     USE_TRIGGER         = false;
const bool     TRIGGER_ACTIVE_LOW  = false;
const unsigned TRIGGER_DEBOUNCE_MS = 30;


// ------------------------------------------------------------- Tuning 🎛️
const unsigned TICK_MS = 10;          // update rate (ms)

// Flames: brightness range, time between new targets, easing (1 = snap, 8+ = lazy)
const uint8_t  FLAME_LVL_MIN  = 40;
const uint8_t  FLAME_LVL_MAX  = 255;
const unsigned FLAME_STEP_MIN = 30;
const unsigned FLAME_STEP_MAX = 140;
const uint8_t  FLAME_EASE     = 3;

// Ember: slow, warm, never goes out
const uint8_t  EMBER_LVL_MIN  = 90;
const uint8_t  EMBER_LVL_MAX  = 200;
const unsigned EMBER_STEP_MIN = 300;
const unsigned EMBER_STEP_MAX = 1500;
const uint8_t  EMBER_EASE     = 12;

// Gusts (flare-ups) and lulls (fire settles down)
const unsigned long EVENT_EVERY_MIN = 3000;
const unsigned long EVENT_EVERY_MAX = 12000;
const unsigned      EVENT_LEN_MIN   = 400;
const unsigned      EVENT_LEN_MAX   = 2000;
const int           GUST_BOOST      = 70;   // added to flame targets during a gust
const int           LULL_DROP       = -60;  // added during a lull
const uint8_t       LULL_PERCENT    = 35;   // % of events that are lulls

// ------------------------------------------------------------- Engine 🔥
struct Flame {
  uint8_t  pin;
  uint8_t  lvlMin, lvlMax;
  unsigned stepMin, stepMax;
  uint8_t  ease;
  int      cur, target;
  unsigned long next;
};

Flame flames[3] = {
  { PIN_FLAME1, FLAME_LVL_MIN, FLAME_LVL_MAX, FLAME_STEP_MIN, FLAME_STEP_MAX, FLAME_EASE, 0, 0, 0 },
  { PIN_FLAME2, FLAME_LVL_MIN, FLAME_LVL_MAX, FLAME_STEP_MIN, FLAME_STEP_MAX, FLAME_EASE, 0, 0, 0 },
  { PIN_EMBER,  EMBER_LVL_MIN, EMBER_LVL_MAX, EMBER_STEP_MIN, EMBER_STEP_MAX, EMBER_EASE, 0, 0, 0 },
};

unsigned long lastTick  = 0;
unsigned long eventAt   = 0;      // when the next gust/lull starts or the current one ends
bool          eventOn   = false;
int           boost     = 0;      // current gust/lull offset applied to flame targets

static int clamp255(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }

void pickTarget(Flame &f, unsigned long now, int offset) {
  f.target = clamp255(random(f.lvlMin, f.lvlMax + 1) + offset);
  f.next   = now + random(f.stepMin, f.stepMax + 1);
}

void easeFlame(Flame &f) {
  int diff = f.target - f.cur;
  int step = diff / f.ease;
  if (step == 0 && diff != 0) step = (diff > 0) ? 1 : -1;
  f.cur += step;
  analogWrite(f.pin, f.cur);
}

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

void setup() {
  for (uint8_t i = 0; i < 3; i++) {
    pinMode(flames[i].pin, OUTPUT);
    analogWrite(flames[i].pin, 0);
  }
  pinMode(PIN_TRIGGER, INPUT_PULLUP);
  randomSeed(analogRead(A0));
  unsigned long now = millis();
  for (uint8_t i = 0; i < 3; i++) pickTarget(flames[i], now, 0);
  eventAt = now + random(EVENT_EVERY_MIN, EVENT_EVERY_MAX + 1);
  lastTick = now;
}

void loop() {
  unsigned long now = millis();
  if (now - lastTick < TICK_MS) return;
  lastTick = now;
  bool run = triggered(now);

  // gusts and lulls
  if ((long)(now - eventAt) >= 0) {
    if (!eventOn) {
      eventOn = true;
      boost   = (random(100) < LULL_PERCENT) ? LULL_DROP : GUST_BOOST;
      eventAt = now + random(EVENT_LEN_MIN, EVENT_LEN_MAX + 1);
    } else {
      eventOn = false;
      boost   = 0;
      eventAt = now + random(EVENT_EVERY_MIN, EVENT_EVERY_MAX + 1);
    }
  }

  // flames: new random target when due, then ease toward it
  for (uint8_t i = 0; i < 3; i++) {
    Flame &f = flames[i];
    int offset = (i == 2) ? boost / 3 : boost;   // ember reacts gently
    if (!run)                            { f.target = 0; f.next = now; }   // fade out; relight instantly when back
    else if ((long)(now - f.next) >= 0)  pickTarget(f, now, offset);
    easeFlame(f);
  }
}
