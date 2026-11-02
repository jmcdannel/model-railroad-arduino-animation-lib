/*
 * 📺 TV Glow — Arduino Nano
 * --------------------------
 * The flickering blue-white light of a television seen through a window.
 * Scenes cut every few seconds to a new brightness and color mix, with
 * gentle flicker inside each scene, occasional bright flashes (explosions,
 * commercials), near-black moments, and the odd bout of channel surfing.
 *
 * Wiring (pin → resistor → LED anode, cathode → GND):
 *   D9  → 220 Ω → cool-white LED
 *   D10 → 220 Ω → blue LED
 *   D11 → 220 Ω → red LED   (optional — adds warm scenes; leave unwired if you don't have one)
 * Point the LEDs at the inside wall of the room, not straight out the window,
 * so the glow washes the room like a real screen would.
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
// When off: the screen fades to black; a fresh scene starts when the trigger returns.
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
const unsigned TICK_MS = 20;

// Scenes: how long each lasts and the brightness range of each color
const unsigned SCENE_MIN = 600;
const unsigned SCENE_MAX = 6000;
const uint8_t  WHITE_MIN = 8,  WHITE_MAX = 200;
const uint8_t  BLUE_MIN  = 20, BLUE_MAX  = 255;
const uint8_t  RED_MIN   = 0,  RED_MAX   = 70;

// Flicker inside a scene
const uint8_t  FLICKER_MAX = 35;     // max ± wobble around the scene level
const unsigned MICRO_MIN   = 40;     // ms between wobbles
const unsigned MICRO_MAX   = 160;

// Scene transitions
const uint8_t  CUT_EASE     = 1;     // 1 = hard cut
const uint8_t  FADE_EASE    = 12;    // bigger = slower fade
const uint8_t  FADE_PERCENT = 25;    // % of scene changes that fade instead of cut

// Special moments (% chance per scene)
const uint8_t  FLASH_PERCENT = 15;   // bright flash somewhere in the scene
const unsigned FLASH_MIN = 30, FLASH_MAX = 120;
const uint8_t  DARK_PERCENT  = 10;   // near-black scene
const uint8_t  SURF_PERCENT  = 6;    // channel surfing: 3–8 rapid cuts
const unsigned SURF_MIN = 150, SURF_MAX = 400;

// ------------------------------------------------------------- Engine 📺
struct Channel {
  uint8_t pin;
  uint8_t lvlMin, lvlMax;
  int     cur, base, target;
};

Channel ch[3] = {
  { PIN_WHITE, WHITE_MIN, WHITE_MAX, 0, 0, 0 },
  { PIN_BLUE,  BLUE_MIN,  BLUE_MAX,  0, 0, 0 },
  { PIN_RED,   RED_MIN,   RED_MAX,   0, 0, 0 },
};

unsigned long lastTick = 0, sceneAt = 0, microAt = 0, flashAt = 0, flashUntil = 0;
uint8_t ease = CUT_EASE, flicker = 0, surfLeft = 0;

static int clamp255(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }

void newScene(unsigned long now) {
  unsigned len;
  bool dark = false;

  if (surfLeft > 0) {                       // mid-surf: quick cut, short scene
    surfLeft--;
    len = random(SURF_MIN, SURF_MAX + 1);
    ease = CUT_EASE;
  } else {
    if (random(100) < SURF_PERCENT) {       // start surfing
      surfLeft = random(3, 9);
      len = random(SURF_MIN, SURF_MAX + 1);
      ease = CUT_EASE;
    } else {
      len  = random(SCENE_MIN, SCENE_MAX + 1);
      ease = (random(100) < FADE_PERCENT) ? FADE_EASE : CUT_EASE;
      dark = (random(100) < DARK_PERCENT);
    }
  }

  for (uint8_t i = 0; i < 3; i++) {
    ch[i].base = dark ? random(0, 6) : random(ch[i].lvlMin, ch[i].lvlMax + 1);
    ch[i].target = ch[i].base;
  }
  flicker = dark ? 2 : random(3, FLICKER_MAX + 1);
  sceneAt = now + len;
  flashAt = (!dark && random(100) < FLASH_PERCENT) ? now + random(100, len) : 0;
}

void setup() {
  for (uint8_t i = 0; i < 3; i++) { pinMode(ch[i].pin, OUTPUT); analogWrite(ch[i].pin, 0); }
  pinMode(PIN_TRIGGER, INPUT_PULLUP);
  randomSeed(analogRead(A0));
  lastTick = millis();
  newScene(lastTick);
}

void loop() {
  unsigned long now = millis();
  if (now - lastTick < TICK_MS) return;
  lastTick = now;
  bool run = triggered(now);

  if (!run) {                                   // trigger off: fade to black, fresh scene when back
    flashAt = 0; flashUntil = 0; sceneAt = now;
    for (uint8_t i = 0; i < 3; i++) ch[i].target = 0;
    ease = 4;
  } else {
    if ((long)(now - sceneAt) >= 0) newScene(now);

    // flash: slam everything bright, then let it ease back down
    if (flashAt && (long)(now - flashAt) >= 0) {
      flashAt = 0;
      flashUntil = now + random(FLASH_MIN, FLASH_MAX + 1);
      ch[0].cur = 255; ch[1].cur = 255; ch[2].cur = 120;
    }
    if (flashUntil) {
      if ((long)(now - flashUntil) < 0) {
        for (uint8_t i = 0; i < 3; i++) analogWrite(ch[i].pin, ch[i].cur);
        return;
      }
      flashUntil = 0;
      ease = 3;                                 // settle back fairly quickly
    }

    // micro-flicker around the scene level
    if ((long)(now - microAt) >= 0) {
      microAt = now + random(MICRO_MIN, MICRO_MAX + 1);
      for (uint8_t i = 0; i < 3; i++)
        ch[i].target = clamp255(ch[i].base + random(-flicker, flicker + 1));
    }
  }

  // ease each channel toward its target
  for (uint8_t i = 0; i < 3; i++) {
    int diff = ch[i].target - ch[i].cur;
    int step = diff / ease;
    if (step == 0 && diff != 0) step = (diff > 0) ? 1 : -1;
    ch[i].cur += step;
    analogWrite(ch[i].pin, ch[i].cur);
  }
}
