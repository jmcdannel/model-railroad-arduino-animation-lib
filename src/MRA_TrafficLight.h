// 🚥 Traffic light: one intersection, two phase groups.
//   Group A = north/south heads     Group B = east/west heads
// Wire ONE head on group A for a single light, or TWO heads per group (in
// parallel, each LED with its own resistor) for a synced four-way.
//
// Cycle: A green → A yellow → all red → B green → B yellow → all red → …
// cfg.nightFlash = true: always late-night mode (A flashes yellow, B red).
// Off: night-flash (or dark, with cfg.offNightFlash = false). On: the normal
// cycle, entered through all-red.
#pragma once
#include "MRA_Trigger.h"

namespace mra {

class TrafficLight {
 public:
  // 🎛️ Tuning. Set fields before begin().
  struct Config {
    uint32_t greenAMs = 14000;      // main road gets the longer green
    uint32_t greenBMs = 9000;
    uint32_t yellowMs = 3000;
    uint32_t allRedMs = 1500;       // both red between phases (prototype practice)
    bool     nightFlash = false;    // true = always flash
    uint16_t flashMs = 500;
    bool     offNightFlash = true;  // off → true: flash like late night, false: dark
  } cfg;

  TrafficLight(uint8_t aRed = 2, uint8_t aYellow = 3, uint8_t aGreen = 4,
               uint8_t bRed = 5, uint8_t bYellow = 6, uint8_t bGreen = 7)
    : pins_{aRed, aYellow, aGreen, bRed, bYellow, bGreen} {}

  void begin() {
    for (uint8_t i = 0; i < 6; i++) outputPin(pins_[i]);
    setLamps(0);                    // update() picks the right mode on its first pass
  }

  void update(uint32_t now, bool on) {
    Mode want = cfg.nightFlash ? MODE_FLASH
              : on             ? MODE_CYCLE
              : cfg.offNightFlash ? MODE_FLASH : MODE_DARK;
    if (want != mode_) enterMode(want, now);

    if (mode_ == MODE_FLASH) {
      if (due(now, stepAt_)) {
        flashOn_ = !flashOn_;
        setLamps(flashOn_ ? (aY | bR) : 0);
        stepAt_ = now + cfg.flashMs;
      }
    } else if (mode_ == MODE_CYCLE) {
      if (due(now, stepAt_)) {
        step_ = (step_ + 1) % 6;
        setLamps(lamps(step_));
        stepAt_ = now + stepMs(step_);
      }
    }
  }

 private:
  // lamp bitmask: bit 0..5 = A red, A yellow, A green, B red, B yellow, B green
  static const uint8_t aR = 1, aY = 2, aG = 4, bR = 8, bY = 16, bG = 32;
  static uint8_t lamps(uint8_t s) {
    switch (s) {
      case 0:  return aG | bR;
      case 1:  return aY | bR;
      case 3:  return aR | bG;
      case 4:  return aR | bY;
      default: return aR | bR;     // 2 and 5: all red
    }
  }

  enum Mode : uint8_t { MODE_CYCLE, MODE_FLASH, MODE_DARK };

  uint32_t stepMs(uint8_t s) const {
    switch (s) {
      case 0:  return cfg.greenAMs;
      case 3:  return cfg.greenBMs;
      case 1:
      case 4:  return cfg.yellowMs;
      default: return cfg.allRedMs;
    }
  }

  void setLamps(uint8_t mask) {
    for (uint8_t i = 0; i < 6; i++) writeOn(pins_[i], (mask >> i) & 1);
  }

  void enterMode(Mode m, uint32_t now) {
    mode_ = m;
    if (m == MODE_CYCLE) {
      step_ = 2;                    // always re-enter through all-red
      setLamps(lamps(step_));
      stepAt_ = now + stepMs(step_);
    } else {
      flashOn_ = false;
      setLamps(0);
      stepAt_ = now;
    }
  }

  uint8_t  pins_[6];
  Mode     mode_ = MODE_DARK;
  uint8_t  step_ = 2;
  uint32_t stepAt_ = 0;
  bool     flashOn_ = false;
};

}  // namespace mra
