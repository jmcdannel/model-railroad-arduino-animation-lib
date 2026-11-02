// 🎛️ Start modes: how an effect gets switched on. Shared by every effect.
//
//   POWER_UP         runs from power-up, no wiring. The pin is ignored.
//   WHILE_TRIGGERED  runs while the pin is asserted, plus runOnMs after it
//                    lets go. DEJA.js IO pin, relay, toggle switch, IR/PIR
//                    sensor, or a momentary button with runOnMs > 0
//                    ("press → runs for 30 s").
//   TOGGLE           each press flips it on/off. Momentary button.
//
// Polarity: ACTIVE_HIGH (default) = the pin going HIGH means "on", which is
// what a DEJA IO pin or a PIR sensor sends; an unwired pin reads HIGH, so the
// effect still runs on its own. ACTIVE_LOW = the pin pulled to GND means "on":
// a button or relay contact to GND, or an IR obstacle module (FC-51, TCRT5000).
#pragma once
#include "MRA_Common.h"

namespace mra {

enum StartMode : uint8_t { POWER_UP, WHILE_TRIGGERED, TOGGLE };
const bool ACTIVE_HIGH = false;
const bool ACTIVE_LOW  = true;

class Trigger {
 public:
  Trigger(StartMode mode = POWER_UP, uint8_t pin = 12, bool activeLow = ACTIVE_HIGH,
          uint32_t runOnMs = 0, uint8_t debounceMs = 30)
    : mode_(mode), runOnMs_(runOnMs), in_(pin, activeLow, debounceMs) {}

  void begin() {
    if (mode_ != POWER_UP) in_.begin();
    on_ = (mode_ == POWER_UP);
  }

  // Call every loop(). Returns whether the effect should be running.
  bool update(uint32_t now) {
    bool was = on_;
    if (mode_ == POWER_UP) {
      on_ = true;
    } else {
      bool asserted = in_.read(now);
      if (mode_ == WHILE_TRIGGERED) {
        if (asserted) { on_ = true; lastAsserted_ = now; held_ = true; }
        else if (held_ && now - lastAsserted_ < runOnMs_) on_ = true;
        else { on_ = false; held_ = false; }
      } else {                                   // TOGGLE: flip on each press
        if (asserted && !wasAsserted_) on_ = !on_;
      }
      wasAsserted_ = asserted;
    }
    started_ = on_ && !was;
    return on_;
  }

  bool on() const { return on_; }
  bool started() const { return started_; }     // switched on during the last update()
  StartMode mode() const { return mode_; }

 private:
  StartMode      mode_;
  uint32_t       runOnMs_;
  DebouncedInput in_;
  bool           on_ = false, started_ = false, wasAsserted_ = false, held_ = false;
  uint32_t       lastAsserted_ = 0;
};

}  // namespace mra
