/*
 * 🚥 Traffic Light — Arduino Nano
 * --------------------------------
 * One intersection, two phase groups:
 *   Group A = north/south heads     Group B = east/west heads
 * Wire ONE head on group A for a single traffic light. Wire TWO heads on
 * each group (LEDs in parallel, each with its own resistor) for a full
 * four-way intersection — all four heads stay in sync automatically.
 *
 * Cycle: A green → A yellow → all red → B green → B yellow → all red → …
 * Set NIGHT_FLASH = true for late-night mode: A flashes yellow, B flashes red.
 * With the D12 trigger enabled, trigger off = night-flash (or dark, see
 * OFF_NIGHT_FLASH) and trigger on = normal cycle, entered through all-red.
 *
 * Wiring (pin → resistor → LED anode, cathode → GND):
 *   Group A:  D2 → red   D3 → yellow   D4 → green
 *   Group B:  D5 → red   D6 → yellow   D7 → green
 * With two heads per group use 470 Ω resistors so each pin sources well
 * under its 40 mA limit. These are plain on/off pins — no PWM needed.
 */

// ---------------------------------------------------------------- Pins 📌
const uint8_t A_RED = 2, A_YEL = 3, A_GRN = 4;
const uint8_t B_RED = 5, B_YEL = 6, B_GRN = 7;

// ------------------------------------------- External trigger (DEJA IO pin) 🎛️
// Off by default. Set USE_TRIGGER = true and run one wire from a DEJA IO pin
// (or a relay contact / toggle switch) to D12, plus GND to GND. HIGH = effect
// runs, matching DEJA's "on". Unplugged reads HIGH, so the board still runs
// on its own. Contact-to-GND or inverted logic: set TRIGGER_ACTIVE_LOW = true.
// When off: night-flash (yellow/red) by default, or dark if OFF_NIGHT_FLASH = false.
const uint8_t  PIN_TRIGGER         = 12;
const bool     USE_TRIGGER         = false;
const bool     TRIGGER_ACTIVE_LOW  = false;
const unsigned TRIGGER_DEBOUNCE_MS = 30;


// ------------------------------------------------------------- Tuning 🎛️
const unsigned long GREEN_A_MS = 14000;   // main road gets the longer green
const unsigned long GREEN_B_MS = 9000;
const unsigned long YELLOW_MS  = 3000;
const unsigned long ALL_RED_MS = 1500;    // both red between phases (prototype practice)
const bool          NIGHT_FLASH = false;
const unsigned      FLASH_MS    = 500;
const bool          OFF_NIGHT_FLASH = true;  // trigger off → true: flash like late night, false: dark

// ------------------------------------------------------------- Engine 🚥
// lamp bitmask: bit 0..5 = A_RED, A_YEL, A_GRN, B_RED, B_YEL, B_GRN
const uint8_t aR = 1, aY = 2, aG = 4, bR = 8, bY = 16, bG = 32;
const uint8_t PINS[6] = { A_RED, A_YEL, A_GRN, B_RED, B_YEL, B_GRN };

struct Step { unsigned long ms; uint8_t lamps; };
const Step CYCLE[] = {
  { GREEN_A_MS, aG | bR },
  { YELLOW_MS,  aY | bR },
  { ALL_RED_MS, aR | bR },
  { GREEN_B_MS, aR | bG },
  { YELLOW_MS,  aR | bY },
  { ALL_RED_MS, aR | bR },
};
const uint8_t STEPS = sizeof(CYCLE) / sizeof(CYCLE[0]);

enum Mode : uint8_t { MODE_CYCLE, MODE_FLASH, MODE_DARK };
Mode          mode = MODE_DARK;
uint8_t       step = 2;          // cycle position; always re-enters through all-red
unsigned long stepAt = 0;
bool          flashOn = false;

void setLamps(uint8_t mask) {
  for (uint8_t i = 0; i < 6; i++) digitalWrite(PINS[i], (mask >> i) & 1 ? HIGH : LOW);
}

void enterMode(Mode m, unsigned long now) {
  mode = m;
  if (m == MODE_CYCLE) {
    step = 2;
    setLamps(CYCLE[step].lamps);
    stepAt = now + CYCLE[step].ms;
  } else {
    flashOn = false;
    setLamps(0);
    stepAt = now;
  }
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
  for (uint8_t i = 0; i < 6; i++) pinMode(PINS[i], OUTPUT);
  pinMode(PIN_TRIGGER, INPUT_PULLUP);
  setLamps(0);                     // loop() picks the right mode on its first pass
}

void loop() {
  unsigned long now = millis();
  bool run = triggered(now);
  Mode want = NIGHT_FLASH ? MODE_FLASH
            : run         ? MODE_CYCLE
            : OFF_NIGHT_FLASH ? MODE_FLASH : MODE_DARK;
  if (want != mode) enterMode(want, now);

  if (mode == MODE_FLASH) {
    if ((long)(now - stepAt) >= 0) {
      flashOn = !flashOn;
      setLamps(flashOn ? (aY | bR) : 0);
      stepAt = now + FLASH_MS;
    }
  } else if (mode == MODE_CYCLE) {
    if ((long)(now - stepAt) >= 0) {
      step = (step + 1) % STEPS;
      setLamps(CYCLE[step].lamps);
      stepAt = now + CYCLE[step].ms;
    }
  }
}
