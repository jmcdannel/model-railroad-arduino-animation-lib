// 💡 Fluorescent tubes starting up: each tube waits a moment, its ends glow
// dimly while the starter preheats, it blinks bright once or a few times as
// it tries to strike, then catches and settles to full brightness. Tubes
// start a little apart, like a shop or office when someone hits the switch.
//
// cfg.badTube: that tube is worn out. It struggles to strike and drops out
// again every so often (or never strikes at all, with cfg.badNeverStrikes).
//
// Off: all tubes go out at once. On again: the whole startup plays again.
#pragma once
#include "MRA_Trigger.h"

namespace mra {

class Fluorescent {
 public:
  // 🎛️ Tuning. Set fields before begin().
  struct Config {
    uint8_t  level = 255;                     // full brightness
    uint16_t staggerMax = 1500;               // tubes start up to this far apart (ms)
    uint8_t  attemptsMin = 1, attemptsMax = 5;   // blinks before a tube catches
    uint8_t  glowLevel = 12;                  // dim end-glow while preheating
    uint16_t glowMin = 250, glowMax = 900;
    uint16_t blinkOnMin = 30, blinkOnMax = 120;   // a failed strike: bright blink…
    uint16_t blinkOffMin = 40, blinkOffMax = 200; //   …then dark before the next try
    uint8_t  warmStartPercent = 75;           // just-struck brightness…
    uint16_t warmupMs = 2500;                 //   …rising to full over this long
    uint8_t  badTube = NO_PIN;                // 0–3 = that tube is worn out
    bool     badNeverStrikes = false;         // true = it blinks forever, never catches
    uint32_t badEveryMin = 4000, badEveryMax = 25000;   // ms between drop-outs
  } cfg;

  // Up to four tubes (or banks of tubes). PWM pins give the dim end-glow and
  // warm-up; plain pins still blink and strike, just without the glow.
  Fluorescent(uint8_t tube0 = 9, uint8_t tube1 = NO_PIN, uint8_t tube2 = NO_PIN, uint8_t tube3 = NO_PIN)
    : pins_{tube0, tube1, tube2, tube3} {}

  void begin() {
    for (uint8_t i = 0; i < 4; i++) { outputPin(pins_[i]); t_[i] = Tube(); }
    lastTick_ = millis();
    running_ = false;
  }

  void update(uint32_t now, bool on) {
    if (now - lastTick_ < TICK_MS) return;
    lastTick_ = now;

    if (on && !running_) {                    // switch flipped on: every tube starts over
      for (uint8_t i = 0; i < 4; i++) {
        t_[i].state = WAIT;
        t_[i].until = now + random(cfg.staggerMax + 1);
      }
    }
    running_ = on;

    for (uint8_t i = 0; i < 4; i++) {
      if (pins_[i] == NO_PIN) continue;
      Tube &t = t_[i];
      if (!on) t.state = OFF;
      else step(i, t, now);
      writeLevel(pins_[i], t.level);
    }
  }

 private:
  static const uint8_t TICK_MS = 10;
  enum State : uint8_t { OFF, WAIT, GLOW, BLINK, DARK, WARM, ON };

  struct Tube {
    State    state = OFF;
    uint8_t  attemptsLeft = 0;
    uint32_t until = 0, since = 0;
    int      level = 0;
  };

  void startGlow(Tube &t, uint32_t now) {
    t.state = GLOW;
    t.until = now + between(cfg.glowMin, cfg.glowMax);
  }

  void step(uint8_t i, Tube &t, uint32_t now) {
    bool bad = (i == cfg.badTube);
    switch (t.state) {
      case OFF:
      case WAIT:
        t.level = 0;
        if (t.state == WAIT && due(now, t.until)) {
          t.attemptsLeft = between(cfg.attemptsMin, cfg.attemptsMax);
          if (bad) t.attemptsLeft += between(2, 6);   // a worn tube struggles longer
          startGlow(t, now);
        }
        break;

      case GLOW:                              // starter preheating the ends
        t.level = cfg.glowLevel;
        if (due(now, t.until)) {
          bool lastTry = t.attemptsLeft <= 1 && !(bad && cfg.badNeverStrikes);
          if (lastTry) {                      // it catches
            t.state = WARM;
            t.since = now;
          } else {                            // a failed strike: bright blink
            if (t.attemptsLeft > 1) t.attemptsLeft--;
            t.state = BLINK;
            t.until = now + between(cfg.blinkOnMin, cfg.blinkOnMax);
          }
        }
        break;

      case BLINK:
        t.level = cfg.level;
        if (due(now, t.until)) { t.state = DARK; t.until = now + between(cfg.blinkOffMin, cfg.blinkOffMax); }
        break;

      case DARK:
        t.level = 0;
        if (due(now, t.until)) startGlow(t, now);
        break;

      case WARM: {                            // struck: rise from warmStart% to full
        uint32_t el = now - t.since;
        if (el >= cfg.warmupMs) {
          t.state = ON;
          t.level = cfg.level;
          if (bad) t.until = now + between(cfg.badEveryMin, cfg.badEveryMax);
        } else {
          long start = (long)cfg.level * cfg.warmStartPercent / 100;
          t.level = start + (cfg.level - start) * (long)el / cfg.warmupMs;
        }
        break;
      }

      case ON:
        t.level = cfg.level;
        if (bad && due(now, t.until)) {       // worn tube drops out and struggles again
          t.attemptsLeft = between(2, 8);
          t.state = DARK;
          t.until = now + between(cfg.blinkOffMin, cfg.blinkOffMax);
        }
        break;
    }
  }

  uint8_t  pins_[4];
  Tube     t_[4];
  uint32_t lastTick_ = 0;
  bool     running_ = false;
};

}  // namespace mra
