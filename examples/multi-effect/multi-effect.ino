/*
 * 🧩 Multi-Effect — several animations on one Arduino Nano
 * --------------------------------------------------------
 * Pick the effects you want in config.h, give each its own pins and its own
 * trigger input (a DEJA.js IO pin, a button, a sensor) or let it run from
 * power-up. Effects can also drive each other on the same board: thunder
 * after lightning, a bell with the crossing gates, a neon sign that lights
 * at dusk.
 *
 * After uploading, open Serial Monitor at 115200: the sketch checks the
 * pin plan (pins used twice, fades on non-PWM pins, servo conflicts) and
 * prints what it found.
 *
 * Each effect is a module in src/ (MRA_*.h); this sketch only wires them up.
 */
#include <MRA_Trigger.h>
using namespace mra;

// effect-to-effect links (used in config.h)
#define SOURCE_TRIGGER    0
#define SOURCE_NIGHT      1
#define SOURCE_LIGHTNING  2
#define SOURCE_GATES_BELL 3
#define SOURCE_CROSSING   4

#include "config.h"

// ------------------------------------------------- the chosen effects 🎬
#if USE_WELDER
  #include <MRA_Welder.h>
  Welder  welder(WELDER_PINS);
  Trigger welderTrigger(WELDER_START, WELDER_TRIGGER);
#endif
#if USE_CAMPFIRE
  #include <MRA_Campfire.h>
  Campfire campfire(CAMPFIRE_PINS);
  Trigger  campfireTrigger(CAMPFIRE_START, CAMPFIRE_TRIGGER);
#endif
#if USE_TV_GLOW
  #include <MRA_TvGlow.h>
  TvGlow  tvGlow(TV_GLOW_PINS);
  Trigger tvGlowTrigger(TV_GLOW_START, TV_GLOW_TRIGGER);
#endif
#if USE_LIGHTNING
  #include <MRA_Lightning.h>
  Lightning lightning(LIGHTNING_PINS);
  Trigger   lightningTrigger(LIGHTNING_START, LIGHTNING_TRIGGER);
#endif
#if USE_CROSSING
  #include <MRA_Crossing.h>
  Crossing crossing(CROSSING_PINS);
  Trigger  crossingTrigger(WHILE_TRIGGERED, CROSSING_TRIGGER);
#endif
#if USE_GATES
  #if !USE_CROSSING
    #error "USE_GATES needs USE_CROSSING: the gates follow the crossing's sensors"
  #endif
  #include <MRA_CrossingGates.h>
  CrossingGates gates(crossing, GATES_PINS);
#endif
#if USE_TRAFFIC_LIGHT
  #include <MRA_TrafficLight.h>
  TrafficLight trafficLight(TRAFFIC_LIGHT_PINS);
  Trigger      trafficLightTrigger(TRAFFIC_LIGHT_START, TRAFFIC_LIGHT_TRIGGER);
#endif
#if USE_NEON
  #include <MRA_Neon.h>
  Neon    neon(NEON_PINS);
  Trigger neonTrigger(NEON_START, NEON_TRIGGER);
#endif
#if USE_FLUORESCENT
  #include <MRA_Fluorescent.h>
  Fluorescent fluorescent(FLUORESCENT_PINS);
  Trigger     fluorescentTrigger(FLUORESCENT_START, FLUORESCENT_TRIGGER);
#endif
#if USE_BEACON
  #include <MRA_Beacon.h>
  Beacon  beacon(BEACON_PINS);
  Trigger beaconTrigger(BEACON_START, BEACON_TRIGGER);
#endif
#if USE_SOUND
  #include <MRA_Sound.h>
  Sound   sound(SOUND_PINS);
  Trigger soundTrigger(SOUND_START, SOUND_TRIGGER);
#endif
#if USE_DAY_NIGHT
  #include <MRA_DayNight.h>
  DayNight dayNight(DAY_NIGHT_PINS);
  Trigger  dayNightTrigger(DAY_NIGHT_START, DAY_NIGHT_TRIGGER);
#endif

// --------------------------------------------------- pin plan check 🔍
uint32_t usedPins = 0;
bool     pinPlanOk = true;

void warn(const __FlashStringHelper *what, uint8_t pin, const __FlashStringHelper *why) {
  pinPlanOk = false;
  Serial.print(F("⚠️  "));
  Serial.print(what);
  Serial.print(F(" pin "));
  Serial.print(pin);
  Serial.print(F(": "));
  Serial.println(why);
}

void claim(uint8_t pin, bool fades, const __FlashStringHelper *what) {
  if (pin == NO_PIN) return;
  if (pin >= 20) { warn(what, pin, F("A6/A7 can only read analog values")); return; }
  if (pin <= 1) warn(what, pin, F("D0/D1 are the USB serial port"));
  if (usedPins & (1UL << pin)) warn(what, pin, F("is already used by another effect"));
  usedPins |= 1UL << pin;
  if (fades && !digitalPinHasPWM(pin)) warn(what, pin, F("needs a PWM pin (3, 5, 6, 9, 10, 11) to fade"));
  if (fades && USE_GATES && (pin == 9 || pin == 10)) warn(what, pin, F("can't fade: the gate servos use Timer1"));
}

template <size_t N>
void claimAll(const uint8_t (&pins)[N], bool fades, const __FlashStringHelper *what) {
  for (size_t i = 0; i < N; i++) claim(pins[i], fades, what);
}

void claimTrigger(StartMode mode, uint8_t pin, const __FlashStringHelper *what) {
  if (mode != POWER_UP) claim(pin, false, what);
}

void checkPins() {
#if USE_WELDER
  { const uint8_t p[] = { WELDER_PINS }; claimAll(p, true, F("welder"));
    const uint8_t t[] = { WELDER_TRIGGER }; claimTrigger(WELDER_START, t[0], F("welder trigger")); }
#endif
#if USE_CAMPFIRE
  { const uint8_t p[] = { CAMPFIRE_PINS }; claimAll(p, true, F("campfire"));
    const uint8_t t[] = { CAMPFIRE_TRIGGER }; claimTrigger(CAMPFIRE_START, t[0], F("campfire trigger")); }
#endif
#if USE_TV_GLOW
  { const uint8_t p[] = { TV_GLOW_PINS }; claimAll(p, true, F("TV glow"));
    const uint8_t t[] = { TV_GLOW_TRIGGER }; claimTrigger(TV_GLOW_START, t[0], F("TV glow trigger")); }
#endif
#if USE_LIGHTNING
  { const uint8_t p[] = { LIGHTNING_PINS };
    claim(p[0], true, F("lightning")); claim(p[1], true, F("lightning")); claim(p[2], false, F("lightning thunder"));
    const uint8_t t[] = { LIGHTNING_TRIGGER }; claimTrigger(LIGHTNING_START, t[0], F("lightning trigger")); }
#endif
#if USE_CROSSING
  { const uint8_t p[] = { CROSSING_PINS };
    claim(p[0], true, F("crossing lamp")); claim(p[1], true, F("crossing lamp"));
    for (uint8_t i = 2; i < 6; i++) claim(p[i], false, F("crossing"));
    const uint8_t t[] = { CROSSING_TRIGGER }; if (CROSSING_FIRE) claim(t[0], false, F("crossing trigger")); }
#endif
#if USE_GATES
  { const uint8_t p[] = { GATES_PINS }; claimAll(p, false, F("gates")); }
#endif
#if USE_TRAFFIC_LIGHT
  { const uint8_t p[] = { TRAFFIC_LIGHT_PINS }; claimAll(p, false, F("traffic light"));
    const uint8_t t[] = { TRAFFIC_LIGHT_TRIGGER }; claimTrigger(TRAFFIC_LIGHT_START, t[0], F("traffic light trigger")); }
#endif
#if USE_NEON
  { const uint8_t p[] = { NEON_PINS }; claimAll(p, true, F("neon"));
    const uint8_t t[] = { NEON_TRIGGER }; if (NEON_SOURCE == SOURCE_TRIGGER) claimTrigger(NEON_START, t[0], F("neon trigger")); }
#endif
#if USE_FLUORESCENT
  { const uint8_t p[] = { FLUORESCENT_PINS }; claimAll(p, true, F("fluorescent"));
    const uint8_t t[] = { FLUORESCENT_TRIGGER }; claimTrigger(FLUORESCENT_START, t[0], F("fluorescent trigger")); }
#endif
#if USE_BEACON
  { const uint8_t p[] = { BEACON_PINS }; claimAll(p, true, F("beacon"));
    const uint8_t t[] = { BEACON_TRIGGER }; claimTrigger(BEACON_START, t[0], F("beacon trigger")); }
#endif
#if USE_SOUND
  { const uint8_t p[] = { SOUND_PINS }; claimAll(p, false, F("sound"));
    const uint8_t t[] = { SOUND_TRIGGER }; if (SOUND_SOURCE == SOURCE_TRIGGER) claimTrigger(SOUND_START, t[0], F("sound trigger")); }
#endif
#if USE_DAY_NIGHT
  { const uint8_t p[] = { DAY_NIGHT_PINS };
    for (uint8_t i = 0; i < 3; i++) claim(p[i], true, F("day-night")); claim(p[3], false, F("day-night output"));
    const uint8_t t[] = { DAY_NIGHT_TRIGGER }; claimTrigger(DAY_NIGHT_START, t[0], F("day-night trigger")); }
#endif
  Serial.println(pinPlanOk ? F("✅ pin plan OK") : F("❌ fix the pins above in config.h"));
}

// ------------------------------------------------------------- setup 🚀
void setup() {
  Serial.begin(115200);
  Serial.println(F("🚂 Model Railroad Animation: multi-effect"));
  checkPins();
  randomSeed(analogRead(A7));              // A7 is input-only on a Nano: never in use

#if USE_WELDER
  welder.begin();        welderTrigger.begin();
#endif
#if USE_CAMPFIRE
  campfire.begin();      campfireTrigger.begin();
#endif
#if USE_TV_GLOW
  tvGlow.begin();        tvGlowTrigger.begin();
#endif
#if USE_LIGHTNING
  lightning.begin();     lightningTrigger.begin();
#endif
#if USE_CROSSING
  crossing.begin();      if (CROSSING_FIRE) crossingTrigger.begin();
#endif
#if USE_GATES
  gates.begin();
#endif
#if USE_TRAFFIC_LIGHT
  trafficLight.begin();  trafficLightTrigger.begin();
#endif
#if USE_NEON
  neon.cfg.pattern = NEON_PATTERN;
  neon.begin();          if (NEON_SOURCE == SOURCE_TRIGGER) neonTrigger.begin();
#endif
#if USE_FLUORESCENT
  fluorescent.begin();   fluorescentTrigger.begin();
#endif
#if USE_BEACON
  beacon.cfg.type = BEACON_TYPE;
  beacon.begin();        beaconTrigger.begin();
#endif
#if USE_SOUND
  { const uint16_t tracks[] = { SOUND_TRACKS };
    sound.cfg.mode = SOUND_MODE;
    sound.cfg.firstTrack = tracks[0];
    sound.cfg.lastTrack = tracks[1];
    sound.cfg.volume = SOUND_VOLUME; }
  sound.begin();         if (SOUND_SOURCE == SOURCE_TRIGGER) soundTrigger.begin();
#endif
#if USE_DAY_NIGHT
  dayNight.begin();      dayNightTrigger.begin();
#endif
}

// -------------------------------------------------------------- loop 🔁
// Order matters only for linked effects: a source updates before what it drives.
void loop() {
  uint32_t now = millis();

#if USE_DAY_NIGHT
  dayNight.update(now, dayNightTrigger.update(now));
#endif
#if USE_WELDER
  welder.update(now, welderTrigger.update(now));
#endif
#if USE_CAMPFIRE
  campfire.update(now, campfireTrigger.update(now));
#endif
#if USE_TV_GLOW
  tvGlow.update(now, tvGlowTrigger.update(now));
#endif
#if USE_LIGHTNING
  lightning.update(now, lightningTrigger.update(now));
#endif
#if USE_CROSSING
  crossing.update(now, CROSSING_FIRE && crossingTrigger.update(now));
#endif
#if USE_GATES
  gates.update(now);
#endif
#if USE_TRAFFIC_LIGHT
  trafficLight.update(now, trafficLightTrigger.update(now));
#endif
#if USE_NEON
  #if NEON_SOURCE == SOURCE_NIGHT && USE_DAY_NIGHT
    neon.update(now, dayNight.isNight());
  #else
    neon.update(now, neonTrigger.update(now));
  #endif
#endif
#if USE_FLUORESCENT
  fluorescent.update(now, fluorescentTrigger.update(now));
#endif
#if USE_BEACON
  beacon.update(now, beaconTrigger.update(now));
#endif
#if USE_SOUND
  #if SOUND_SOURCE == SOURCE_LIGHTNING && USE_LIGHTNING
    sound.update(now, lightning.thunder());
  #elif SOUND_SOURCE == SOURCE_GATES_BELL && USE_GATES
    sound.update(now, gates.bell());
  #elif SOUND_SOURCE == SOURCE_CROSSING && USE_CROSSING
    sound.update(now, crossing.flashing());
  #else
    sound.update(now, soundTrigger.update(now));
  #endif
#endif
}
