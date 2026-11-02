/*
 * 🚦 Grade Crossing Flashers — Arduino Nano
 * ------------------------------------------
 * Two red lamps alternate at the prototype 50 flashes per minute, with a
 * soft incandescent-style fade on each lamp. Triggered by IR sensors in the
 * track, a pushbutton, or both.
 *
 * Sensors — three ways to wire it:
 *   • Two sensors (best): one on each approach. The first sensor a train hits
 *     starts the flashers; they keep going until the train has passed the
 *     far sensor and been clear of it for HOLD_MS. Works in both directions,
 *     and gaps between cars never stop the flashers early.
 *   • One sensor (set USE_SENSOR_B = false): put it right at the crossing.
 *     Flashers run while the sensor sees the train plus HOLD_MS after.
 *   • Button only: press → flashers run for BUTTON_RUN_MS. Press again to
 *     extend. Works alongside sensors too (good for visitors).
 *   • D12 trigger (DEJA IO pin, see below): flashers run while it's asserted
 *     and for HOLD_MS after. Lets a DEJA sensor or route fire the crossing.
 * If sensor activity stops altogether (train reversed out, or you only have
 * one sensor), the flashers release after RELEASE_TIMEOUT_MS. MAX_ACTIVE_MS
 * is a safety net so a stalled train can't leave them going forever.
 *
 * Wiring:
 *   D9  → 220 Ω → red LED (left lamp)    cathode → GND
 *   D10 → 220 Ω → red LED (right lamp)   cathode → GND
 *   D2  ← IR sensor A OUT   (FC-51 / TCRT5000 module: VCC→5V, GND→GND, OUT→D2)
 *   D3  ← IR sensor B OUT
 *   D4  ← pushbutton → GND  (internal pull-up, no resistor needed)
 *   D5  → optional "active" output: HIGH while the flashers run. Use it later
 *         for a bell module or a gate servo.
 * The cheap IR obstacle modules pull OUT LOW when they see something. Mount
 * one between the ties pointing up and turn its pot until it triggers on a
 * car's underside at 2–3 cm but not on the ballast. Strong room light or
 * sunlight on the sensor can cause false triggers — shade it if needed.
 */

// ---------------------------------------------------------------- Pins 📌
const uint8_t PIN_LAMP_L   = 9;
const uint8_t PIN_LAMP_R   = 10;
const uint8_t PIN_SENSOR_A = 2;
const uint8_t PIN_SENSOR_B = 3;
const uint8_t PIN_BUTTON   = 4;
const uint8_t PIN_ACTIVE   = 5;

// ------------------------------------------- External trigger (DEJA IO pin) 🎛️
// Off by default. Set USE_TRIGGER = true and run one wire from a DEJA IO pin
// (or a relay contact / toggle switch) to D12, plus GND to GND. HIGH = effect
// runs, matching DEJA's "on". Unplugged reads HIGH, so the board still runs
// on its own. Contact-to-GND or inverted logic: set TRIGGER_ACTIVE_LOW = true.
// When off: nothing — the IR sensors and button keep working as usual.
// (Note: with USE_TRIGGER = true an unplugged D12 reads HIGH = flashers run.)
const uint8_t  PIN_TRIGGER         = 12;
const bool     USE_TRIGGER         = false;
const bool     TRIGGER_ACTIVE_LOW  = false;

// ------------------------------------------------------------- Tuning 🎛️
const bool     USE_SENSOR_B      = true;   // false = single sensor at the crossing
const bool     SENSOR_ACTIVE_LOW = true;   // true for FC-51 / TCRT5000 modules
const unsigned FLASHES_PER_MIN   = 50;     // prototype: 35–65
const bool     INCANDESCENT_FADE = true;   // soft ramp like old bulbs; false = hard LED snap
const unsigned FADE_MS           = 70;
const unsigned long HOLD_MS            = 3000;   // keep flashing after the train clears
const unsigned long RELEASE_TIMEOUT_MS = 15000;  // no sensor activity at all → release
const unsigned long MAX_ACTIVE_MS      = 120000; // safety net
const unsigned long BUTTON_RUN_MS      = 20000;
const unsigned DEBOUNCE_MS = 30;
const unsigned TICK_MS     = 10;

// ------------------------------------------------------------- Engine 🚦
struct Input {
  uint8_t pin;
  bool    activeLow;
  bool    stable, lastRaw;
  unsigned long changedAt;
};

Input sensorA = { PIN_SENSOR_A, SENSOR_ACTIVE_LOW, false, false, 0 };
Input sensorB = { PIN_SENSOR_B, SENSOR_ACTIVE_LOW, false, false, 0 };
Input button  = { PIN_BUTTON,   true,              false, false, 0 };
Input trigger = { PIN_TRIGGER,  TRIGGER_ACTIVE_LOW, false, false, 0 };

bool readInput(Input &in, unsigned long now) {
  bool raw = (digitalRead(in.pin) == LOW) == in.activeLow;
  if (raw != in.lastRaw) { in.lastRaw = raw; in.changedAt = now; }
  else if (raw != in.stable && now - in.changedAt >= DEBOUNCE_MS) in.stable = raw;
  return in.stable;
}

bool          active = false;      // sensor-driven activation
bool          firstWasA = false;   // which sensor the train hit first
bool          farSeen = false;     // has the train reached the far sensor?
unsigned long activeSince = 0, lastSensorMs = 0, manualUntil = 0, trigLast = 0;
bool          buttonWas = false;
bool          flashing = false;
unsigned long flashStart = 0, lastTick = 0;
int           levelL = 0, levelR = 0;

void setup() {
  pinMode(PIN_LAMP_L, OUTPUT);
  pinMode(PIN_LAMP_R, OUTPUT);
  pinMode(PIN_ACTIVE, OUTPUT);
  pinMode(PIN_SENSOR_A, INPUT_PULLUP);
  pinMode(PIN_SENSOR_B, INPUT_PULLUP);
  pinMode(PIN_BUTTON,   INPUT_PULLUP);
  pinMode(PIN_TRIGGER,  INPUT_PULLUP);
  analogWrite(PIN_LAMP_L, 0);
  analogWrite(PIN_LAMP_R, 0);
  digitalWrite(PIN_ACTIVE, LOW);
  lastTick = millis();
}

void loop() {
  unsigned long now = millis();
  if (now - lastTick < TICK_MS) return;
  lastTick = now;

  // ---- inputs -----------------------------------------------------------
  bool a = readInput(sensorA, now);
  bool b = USE_SENSOR_B ? readInput(sensorB, now) : false;
  bool pressed = readInput(button, now);
  if (pressed && !buttonWas) manualUntil = now + BUTTON_RUN_MS;
  buttonWas = pressed;
  bool trig = USE_TRIGGER ? readInput(trigger, now) : false;
  if (trig) trigLast = now;
  bool external = trig || (trigLast && now - trigLast < HOLD_MS);

  bool anySensor = a || b;
  if (anySensor) lastSensorMs = now;

  // ---- sensor logic -----------------------------------------------------
  if (!active) {
    if (anySensor) {
      active = true;
      activeSince = now;
      firstWasA = a;
      farSeen = !USE_SENSOR_B;             // single sensor: nothing "far" to wait for
    }
  } else {
    if (USE_SENSOR_B && ((firstWasA && b) || (!firstWasA && a))) farSeen = true;
    unsigned long quiet = now - lastSensorMs;
    if (!anySensor && ((farSeen && quiet >= HOLD_MS) || quiet >= RELEASE_TIMEOUT_MS)) active = false;
    if (now - activeSince >= MAX_ACTIVE_MS) active = false;
  }

  bool manual = manualUntil && (long)(now - manualUntil) < 0;
  bool wantFlash = active || manual || external;

  if (wantFlash && !flashing) flashStart = now;   // always start on the left lamp
  flashing = wantFlash;
  digitalWrite(PIN_ACTIVE, flashing ? HIGH : LOW);

  // ---- lamps ------------------------------------------------------------
  int targetL = 0, targetR = 0;
  if (flashing) {
    unsigned long period = 60000UL / FLASHES_PER_MIN;
    bool leftOn = ((now - flashStart) % period) < period / 2;
    targetL = leftOn ? 255 : 0;
    targetR = leftOn ? 0 : 255;
  }

  if (INCANDESCENT_FADE) {
    int step = (int)(255UL * TICK_MS / FADE_MS);
    if (step < 1) step = 1;
    if (levelL < targetL) levelL = (levelL + step > targetL) ? targetL : levelL + step;
    else if (levelL > targetL) levelL = (levelL - step < targetL) ? targetL : levelL - step;
    if (levelR < targetR) levelR = (levelR + step > targetR) ? targetR : levelR + step;
    else if (levelR > targetR) levelR = (levelR - step < targetR) ? targetR : levelR - step;
  } else {
    levelL = targetL;
    levelR = targetR;
  }
  analogWrite(PIN_LAMP_L, levelL);
  analogWrite(PIN_LAMP_R, levelR);
}
