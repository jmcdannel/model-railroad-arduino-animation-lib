// 🌅 Day-night room lighting: dawn, day, dusk and night on three channels of
// LED strip (through MOSFETs), with an optional "night" output that switches
// other things on at dusk: building lights, a neon sign, street lamps.
//
//   day channel    cool/daylight white
//   warm channel   warm white or amber: sunrise and sunset
//   night channel  blue: moonlight
//
// Dusk goes white → golden → orange → blue hour → moonlight; dawn runs it
// back the other way. Levels are gamma-corrected so fades look even.
//
// What "on" means (cfg.onMeansDay):
//   false (default)  on = run the day-night cycle; off = lights fade out and
//                    the clock pauses (on again resumes where it was)
//   true             on = sunrise into day and stay there; off = sunset
//                    into night and stay there. A DEJA.js "daylight" switch.
//
// LED strips draw far more than an Arduino pin can give: each channel needs a
// logic-level MOSFET (IRLZ44N, or a ready-made MOSFET module) between the
// strip's negative lead and GND, with its gate on the PWM pin.
#pragma once
#include "MRA_Trigger.h"

namespace mra {

enum DayPhase : uint8_t { PHASE_DAY, PHASE_DUSK, PHASE_NIGHT, PHASE_DAWN };

class DayNight {
 public:
  // 🎛️ Tuning. Set fields before begin().
  struct Config {
    uint32_t dayMs   = 600000;          // 10 min of daylight…
    uint32_t duskMs  = 120000;          // 2 min sunset
    uint32_t nightMs = 240000;          // 4 min of night
    uint32_t dawnMs  = 90000;           // 1.5 min sunrise
    uint8_t  dayLevel = 255;            // brightest each channel gets
    uint8_t  warmLevel = 220;
    uint8_t  nightLevel = 60;           // moonlight should be dim
    bool     onMeansDay = false;        // see above
    DayPhase startPhase = PHASE_DAY;    // where the cycle starts at power-up
    uint8_t  lightsOnPercent = 50;      // night output goes HIGH this far into dusk…
    uint8_t  lightsOffPercent = 50;     // …and LOW this far into dawn
    uint16_t offFadeMs = 3000;          // cycle mode: fade-out when switched off
  } cfg;

  // Any channel can be NO_PIN. Use PWM pins (3, 5, 6, 9, 10, 11) for the strips.
  DayNight(uint8_t dayPin = 9, uint8_t warmPin = 10, uint8_t nightPin = 11, uint8_t nightOutPin = NO_PIN)
    : pins_{dayPin, warmPin, nightPin}, nightOut_(nightOutPin) {}

  void begin() {
    for (uint8_t i = 0; i < 3; i++) outputPin(pins_[i]);
    outputPin(nightOut_);
    phase_ = cfg.startPhase;
    elapsed_ = 0;
    night_ = (phase_ == PHASE_NIGHT);
    lastTick_ = millis();
  }

  bool isNight() const { return night_; }
  DayPhase phase() const { return phase_; }

  void update(uint32_t now, bool on) {
    uint32_t dt = now - lastTick_;
    if (dt < TICK_MS) return;
    lastTick_ = now;

    if (cfg.onMeansDay) {
      // ☀️ / 🌙 follow the switch: head for day while on, night while off
      if (on && (phase_ == PHASE_NIGHT || phase_ == PHASE_DUSK)) reverse(PHASE_DAWN, cfg.duskMs, cfg.dawnMs);
      if (!on && (phase_ == PHASE_DAY || phase_ == PHASE_DAWN)) reverse(PHASE_DUSK, cfg.dawnMs, cfg.duskMs);
      advance(dt, true);
      master_ = 1000;
    } else {
      // 🔁 run the cycle while on; fade out and pause while off
      if (on) advance(dt, false);
      int step = (int)(1000UL * dt / (cfg.offFadeMs ? cfg.offFadeMs : 1));
      master_ = on ? master_ + step : master_ - step;
      master_ = constrain(master_, 0, 1000);
    }

    uint8_t mix[3];
    currentMix(mix);
    const uint8_t maxLvl[3] = { cfg.dayLevel, cfg.warmLevel, cfg.nightLevel };
    for (uint8_t i = 0; i < 3; i++) {
      // perceived % → PWM with gamma 2, scaled by the channel max and master fade
      long lvl = (long)maxLvl[i] * mix[i] * mix[i] / 10000;
      writeLevel(pins_[i], lvl * master_ / 1000);
    }
    writeOn(nightOut_, night_);
  }

 private:
  static const uint8_t TICK_MS = 20;

  uint32_t phaseMs(DayPhase p) const {
    switch (p) {
      case PHASE_DAY:   return cfg.dayMs;
      case PHASE_DUSK:  return cfg.duskMs;
      case PHASE_NIGHT: return cfg.nightMs;
      default:          return cfg.dawnMs;
    }
  }

  // switch dusk ↔ dawn mid-way, keeping the light where it is
  void reverse(DayPhase to, uint32_t fromMs, uint32_t toMs) {
    uint32_t done = (phase_ == PHASE_DUSK || phase_ == PHASE_DAWN) ? elapsed_ : fromMs;
    uint32_t remaining = fromMs > done ? fromMs - done : 0;
    elapsed_ = fromMs ? (uint32_t)((float)toMs * remaining / fromMs) : 0;
    phase_ = to;
  }

  void advance(uint32_t dt, bool holdDayNight) {
    elapsed_ += dt;
    uint32_t len = phaseMs(phase_);
    if (elapsed_ < len) return;
    if (holdDayNight && (phase_ == PHASE_DAY || phase_ == PHASE_NIGHT)) { elapsed_ = len; return; }
    elapsed_ = 0;
    if (holdDayNight) phase_ = (phase_ == PHASE_DUSK) ? PHASE_NIGHT : PHASE_DAY;
    else              phase_ = (DayPhase)((phase_ + 1) % 4);
  }

  // perceived brightness 0–100 for day, warm, night right now
  void currentMix(uint8_t mix[3]) {
    // keyframes: position % through the transition, then day, warm, night %
    static const uint8_t DUSK[5][4] = {
      {   0, 100,  10,   0 },             // daylight
      {  30,  70,  55,   0 },             // golden hour
      {  55,  25, 100,  10 },             // sunset glow
      {  80,   3,  45,  60 },             // blue hour
      { 100,   0,   0, 100 },             // moonlight
    };
    static const uint8_t DAWN[5][4] = {
      {   0,   0,   0, 100 },
      {  30,   0,  25,  90 },             // first light
      {  55,  10,  80,  40 },             // sunrise
      {  80,  55,  60,   5 },             // morning
      { 100, 100,  10,   0 },
    };
    uint32_t len = phaseMs(phase_);
    uint32_t p = len ? (uint32_t)((float)elapsed_ * 100 / len) : 100;
    uint8_t pct = p > 100 ? 100 : p;

    if (phase_ == PHASE_DAY || phase_ == PHASE_NIGHT) {
      const uint8_t *k = (phase_ == PHASE_DAY) ? DAWN[4] : DUSK[4];
      for (uint8_t c = 0; c < 3; c++) mix[c] = k[c + 1];
    } else {
      const uint8_t (*k)[4] = (phase_ == PHASE_DUSK) ? DUSK : DAWN;
      uint8_t i = 0;
      while (i < 3 && pct > k[i + 1][0]) i++;
      uint8_t span = k[i + 1][0] - k[i][0];
      for (uint8_t c = 0; c < 3; c++) {
        int a = k[i][c + 1], b = k[i + 1][c + 1];
        mix[c] = a + (b - a) * (int)(pct - k[i][0]) / span;
      }
    }

    // 🏠 night output, with a little hysteresis built in by the phase
    if (phase_ == PHASE_DUSK && pct >= cfg.lightsOnPercent) night_ = true;
    if (phase_ == PHASE_NIGHT) night_ = true;
    if (phase_ == PHASE_DAWN && pct >= cfg.lightsOffPercent) night_ = false;
    if (phase_ == PHASE_DAY) night_ = false;
  }

  uint8_t  pins_[3], nightOut_;
  DayPhase phase_ = PHASE_DAY;
  uint32_t elapsed_ = 0, lastTick_ = 0;
  int      master_ = 0;
  bool     night_ = false;
};

}  // namespace mra
