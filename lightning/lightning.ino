/*
 * ⛈️ Lightning Storm — Arduino Nano
 * ----------------------------------
 * White LEDs strobe like distant lightning: clusters of 1–6 flashes with
 * random brightness, some stuttering, a fast afterglow after each flash,
 * then nothing for a while. The storm drifts between active spells
 * (strikes every few seconds) and quiet spells (a strike every half minute
 * or so), so it never feels like a metronome.
 *
 * Wiring (pin → resistor → LED anode, cathode → GND):
 *   D9  → 150–220 Ω → white LED (main)
 *   D10 → 150–220 Ω → white LED (optional second, set USE_FLASH_B)
 *   D2  → optional "thunder" trigger: goes HIGH for THUNDER_PULSE_MS at the
 *         start of every strike. Feed it to a sound module later (DFPlayer
 *         via a small transistor, or an Arduino input) and you get thunder
 *         that follows the lightning.
 * Hide the LEDs behind the backdrop or above a cloud flat, pointing at the
 * sky, so you see the flash on the backdrop rather than the LED itself.
 */

// ---------------------------------------------------------------- Pins 📌
const uint8_t PIN_FLASH_A = 9;
const uint8_t PIN_FLASH_B = 10;
const uint8_t PIN_THUNDER = 2;
const bool    USE_FLASH_B = true;
const bool    USE_THUNDER = true;

// ------------------------------------------- External trigger (DEJA IO pin) 🎛️
// Off by default. Set USE_TRIGGER = true and run one wire from a DEJA IO pin
// (or a relay contact / toggle switch) to D12, plus GND to GND. HIGH = effect
// runs, matching DEJA's "on". Unplugged reads HIGH, so the board still runs
// on its own. Contact-to-GND or inverted logic: set TRIGGER_ACTIVE_LOW = true.
// When off: no strikes; a strike in progress is cut. First strike ~2 s after the trigger returns.
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
const unsigned TICK_MS = 5;

// Storm rhythm (ms)
const unsigned long ACTIVE_SPELL_MIN = 20000, ACTIVE_SPELL_MAX = 90000;
const unsigned long QUIET_SPELL_MIN  = 15000, QUIET_SPELL_MAX  = 60000;
const unsigned long GAP_ACTIVE_MIN   = 1500,  GAP_ACTIVE_MAX   = 9000;   // between strikes, active spell
const unsigned long GAP_QUIET_MIN    = 12000, GAP_QUIET_MAX    = 40000;  // between strikes, quiet spell

// A strike is a cluster of flashes
const uint8_t  FLASHES_MIN = 1,  FLASHES_MAX = 6;
const unsigned FLASH_ON_MIN = 15, FLASH_ON_MAX = 90;      // ms a flash is lit
const unsigned FLASH_GAP_MIN = 40, FLASH_GAP_MAX = 350;   // ms between flashes
const uint8_t  LVL_MIN = 70, LVL_MAX = 255;               // flash brightness
const uint8_t  STUTTER_PERCENT = 30;                      // % of flashes that stutter
const unsigned STUTTER_ON_MIN = 8,  STUTTER_ON_MAX = 25;
const unsigned STUTTER_OFF_MIN = 10, STUTTER_OFF_MAX = 40;
const uint8_t  AFTERGLOW_DIV = 3;      // after a flash the level drops by 1/3 per tick
const uint8_t  B_PERCENT_MIN = 30, B_PERCENT_MAX = 100;   // LED B shows this % of LED A
const unsigned THUNDER_PULSE_MS = 100;

// ------------------------------------------------------------- Engine ⚡
enum Phase : uint8_t { WAITING, LIT, DARK };
Phase phase = WAITING;

unsigned long lastTick = 0, phaseAt = 0, spellAt = 0, thunderOff = 0;
bool    activeSpell = true;
uint8_t flashesLeft = 0, pulsesLeft = 0;
int     lvl = 0, lvlA = 0, lvlB = 0;

void write() {
  analogWrite(PIN_FLASH_A, lvlA);
  if (USE_FLASH_B) analogWrite(PIN_FLASH_B, lvlB);
}

void lightOn(int level) {
  lvlA = level;
  lvlB = (int)((long)level * random(B_PERCENT_MIN, B_PERCENT_MAX + 1) / 100);
  write();
}

void scheduleStrike(unsigned long now) {
  phase = WAITING;
  phaseAt = now + (activeSpell ? random(GAP_ACTIVE_MIN, GAP_ACTIVE_MAX + 1)
                               : random(GAP_QUIET_MIN,  GAP_QUIET_MAX  + 1));
}

void startFlash(unsigned long now) {
  lvl = random(LVL_MIN, LVL_MAX + 1);
  bool stutter = random(100) < STUTTER_PERCENT;
  pulsesLeft = stutter ? random(3, 7) : 1;
  lightOn(lvl);
  phase = LIT;
  phaseAt = now + (stutter ? random(STUTTER_ON_MIN, STUTTER_ON_MAX + 1)
                           : random(FLASH_ON_MIN,   FLASH_ON_MAX   + 1));
}

void startStrike(unsigned long now) {
  flashesLeft = random(FLASHES_MIN, FLASHES_MAX + 1);
  if (USE_THUNDER) { digitalWrite(PIN_THUNDER, HIGH); thunderOff = now + THUNDER_PULSE_MS; }
  startFlash(now);
}

void setup() {
  pinMode(PIN_FLASH_A, OUTPUT);
  if (USE_FLASH_B) pinMode(PIN_FLASH_B, OUTPUT);
  if (USE_THUNDER) { pinMode(PIN_THUNDER, OUTPUT); digitalWrite(PIN_THUNDER, LOW); }
  pinMode(PIN_TRIGGER, INPUT_PULLUP);
  write();
  randomSeed(analogRead(A0));
  lastTick = millis();
  spellAt = lastTick + random(ACTIVE_SPELL_MIN, ACTIVE_SPELL_MAX + 1);
  phase = WAITING;
  phaseAt = lastTick + 3000;        // first strike a few seconds after power-up
}

void loop() {
  unsigned long now = millis();
  if (now - lastTick < TICK_MS) return;
  lastTick = now;
  bool run = triggered(now);

  // storm drifts between active and quiet spells
  if ((long)(now - spellAt) >= 0) {
    activeSpell = !activeSpell;
    spellAt = now + (activeSpell ? random(ACTIVE_SPELL_MIN, ACTIVE_SPELL_MAX + 1)
                                 : random(QUIET_SPELL_MIN,  QUIET_SPELL_MAX  + 1));
  }

  if (USE_THUNDER && thunderOff && (long)(now - thunderOff) >= 0) {
    digitalWrite(PIN_THUNDER, LOW);
    thunderOff = 0;
  }

  if (!run) {                            // trigger off: dark, and hold until it's back
    if (lvlA || lvlB) { lvlA = 0; lvlB = 0; write(); }
    if (USE_THUNDER && thunderOff) { digitalWrite(PIN_THUNDER, LOW); thunderOff = 0; }
    phase = WAITING;
    phaseAt = now + 2000;
    return;
  }

  if ((long)(now - phaseAt) >= 0) {
    switch (phase) {
      case WAITING:
        startStrike(now);
        break;

      case LIT:                              // flash (or stutter pulse) ends
        pulsesLeft--;
        phase = DARK;
        if (pulsesLeft > 0) {
          phaseAt = now + random(STUTTER_OFF_MIN, STUTTER_OFF_MAX + 1);
        } else {
          flashesLeft--;
          phaseAt = now + random(FLASH_GAP_MIN, FLASH_GAP_MAX + 1);
        }
        break;

      case DARK:
        if (pulsesLeft > 0) {                // next stutter pulse, a bit dimmer
          lightOn(lvl * random(60, 101) / 100);
          phase = LIT;
          phaseAt = now + random(STUTTER_ON_MIN, STUTTER_ON_MAX + 1);
        } else if (flashesLeft > 0) {
          startFlash(now);
        } else {
          scheduleStrike(now);
        }
        break;
    }
  }

  // afterglow: fast decay whenever we're not lit
  if (phase != LIT && (lvlA || lvlB)) {
    lvlA -= lvlA / AFTERGLOW_DIV + (lvlA ? 1 : 0);
    lvlB -= lvlB / AFTERGLOW_DIV + (lvlB ? 1 : 0);
    if (lvlA < 0) lvlA = 0;
    if (lvlB < 0) lvlB = 0;
    write();
  }
}
