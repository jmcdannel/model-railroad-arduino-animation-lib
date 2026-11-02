// 🚦 Grade crossing flashers: two red lamps alternate at the prototype 50
// flashes per minute, with a soft incandescent-style fade. Run by IR
// sensors in the track, a pushbutton, and/or an external "fire" input.
//
//   • Two sensors (best): one on each approach. The first sensor a train hits
//     starts the flashers; they keep going until the train has passed the far
//     sensor and been clear of it for holdMs. Works in both directions, and
//     gaps between cars never stop the flashers early.
//   • One sensor (sensor B = NO_PIN): put it right at the crossing. Flashers
//     run while the sensor sees the train plus holdMs after.
//   • Button: press → flashers run for buttonRunMs. Press again to extend.
//   • fire (update()'s second argument): flashers run while it's true and
//     for holdMs after. A DEJA.js IO pin, a detection block, a timer…
// If sensor activity stops altogether (train reversed out, or one sensor
// only), the flashers release after releaseTimeoutMs. maxActiveMs is a safety
// net so a stalled train can't leave them going forever.
//
// The crossing is always armed: unlike other effects, update()'s second
// argument fires it rather than enabling it.
#pragma once
#include "MRA_Trigger.h"

namespace mra {

class Crossing {
 public:
  // 🎛️ Tuning. Set fields before begin().
  struct Config {
    bool     sensorActiveLow = true;      // true for FC-51 / TCRT5000 IR modules
    uint8_t  flashesPerMin = 50;          // prototype: 35–65
    bool     incandescentFade = true;     // soft ramp like old bulbs; false = LED snap
    uint8_t  fadeMs = 70;
    uint32_t holdMs = 3000;               // keep flashing after the train clears
    uint32_t releaseTimeoutMs = 15000;    // no sensor activity at all → release
    uint32_t maxActiveMs = 120000;        // safety net
    uint32_t buttonRunMs = 20000;
    uint8_t  debounceMs = 30;
    uint8_t  tickMs = 10;
  } cfg;

  // Any input or the "active" output can be NO_PIN.
  Crossing(uint8_t lampLPin = 9, uint8_t lampRPin = 10, uint8_t sensorAPin = 2,
           uint8_t sensorBPin = 3, uint8_t buttonPin = 4, uint8_t activeOutPin = 5)
    : lampL_(lampLPin), lampR_(lampRPin), activeOut_(activeOutPin),
      sensorA_(sensorAPin, true), sensorB_(sensorBPin, true), button_(buttonPin, true) {}

  void begin() {
    sensorA_.activeLow = sensorB_.activeLow = cfg.sensorActiveLow;
    sensorA_.debounceMs = sensorB_.debounceMs = button_.debounceMs = cfg.debounceMs;
    pinMode(lampL_, OUTPUT);
    pinMode(lampR_, OUTPUT);
    outputPin(activeOut_);
    sensorA_.begin();
    sensorB_.begin();
    button_.begin();
    analogWrite(lampL_, 0);
    analogWrite(lampR_, 0);
    lastTick_ = millis();
  }

  bool active() const { return wanted_; }       // a train (or button/fire) wants protection
  bool flashing() const { return flashing_; }
  void keepFlashing(bool keep) { keep_ = keep; } // e.g. until the gates are back up

  void update(uint32_t now, bool fire = false) {
    if (now - lastTick_ < cfg.tickMs) return;
    lastTick_ = now;
    bool twoSensors = sensorB_.pin != NO_PIN;

    // ---- inputs ---------------------------------------------------------
    bool a = sensorA_.read(now, false);
    bool b = twoSensors ? sensorB_.read(now, false) : false;
    bool pressed = button_.read(now, false);
    if (pressed && !buttonWas_) manualUntil_ = now + cfg.buttonRunMs;
    buttonWas_ = pressed;
    if (fire) fireLast_ = now;
    bool external = fire || (fireLast_ && now - fireLast_ < cfg.holdMs);

    bool anySensor = a || b;
    if (anySensor) lastSensorMs_ = now;

    // ---- sensor logic ---------------------------------------------------
    if (!active_) {
      if (anySensor) {
        active_ = true;
        activeSince_ = now;
        firstWasA_ = a;
        farSeen_ = !twoSensors;                   // single sensor: nothing "far" to wait for
      }
    } else {
      if (twoSensors && ((firstWasA_ && b) || (!firstWasA_ && a))) farSeen_ = true;
      uint32_t quiet = now - lastSensorMs_;
      if (!anySensor && ((farSeen_ && quiet >= cfg.holdMs) || quiet >= cfg.releaseTimeoutMs)) active_ = false;
      if (now - activeSince_ >= cfg.maxActiveMs) active_ = false;
    }

    bool manual = manualUntil_ && !due(now, manualUntil_);
    wanted_ = active_ || manual || external;
    bool wantFlash = wanted_ || keep_;

    if (wantFlash && !flashing_) flashStart_ = now;   // always start on the left lamp
    flashing_ = wantFlash;
    writeOn(activeOut_, flashing_);

    // ---- lamps ----------------------------------------------------------
    int targetL = 0, targetR = 0;
    if (flashing_) {
      uint32_t period = 60000UL / cfg.flashesPerMin;
      bool leftOn = ((now - flashStart_) % period) < period / 2;
      targetL = leftOn ? 255 : 0;
      targetR = leftOn ? 0 : 255;
    }

    if (cfg.incandescentFade) {
      int step = (int)(255UL * cfg.tickMs / cfg.fadeMs);
      if (step < 1) step = 1;
      levelL_ = stepToward(levelL_, targetL, step);
      levelR_ = stepToward(levelR_, targetR, step);
    } else {
      levelL_ = targetL;
      levelR_ = targetR;
    }
    analogWrite(lampL_, levelL_);
    analogWrite(lampR_, levelR_);
  }

 private:
  static int stepToward(int cur, int target, int step) {
    if (cur < target) return (cur + step > target) ? target : cur + step;
    if (cur > target) return (cur - step < target) ? target : cur - step;
    return cur;
  }

  uint8_t        lampL_, lampR_, activeOut_;
  DebouncedInput sensorA_, sensorB_, button_;
  bool           active_ = false, firstWasA_ = false, farSeen_ = false;
  bool           buttonWas_ = false, flashing_ = false, wanted_ = false, keep_ = false;
  uint32_t       activeSince_ = 0, lastSensorMs_ = 0, manualUntil_ = 0, fireLast_ = 0;
  uint32_t       flashStart_ = 0, lastTick_ = 0;
  int            levelL_ = 0, levelR_ = 0;
};

}  // namespace mra
