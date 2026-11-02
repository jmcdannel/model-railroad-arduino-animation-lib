// 🚨 Beacons: a lamp with a turning reflector, faked with brightness. As the
// beam sweeps past you the LED swells to a sharp peak, then falls back to a
// faint glow, exactly like a rotating light seen from the side.
//
//   BEACON_ROTATING  one lamp (amber on a tow truck or plant roof, red on a
//                    fire station…); cfg.reflectors = 2 for a double beam
//   BEACON_AIRPORT   white (pin A) and green (pin B) on opposite sides, slow
//   BEACON_TOWER     radio/water tower obstruction light: a red lamp that
//                    fades on and off ~30 times a minute (no rotation);
//                    pin B, if used, flashes with it (a second tier)
//
// Rotating beacons spin up when switched on and coast to a stop when
// switched off, so the first few sweeps are slow.
//
// Off: the lamp fades out (rotating types also slow down while it fades).
#pragma once
#include "MRA_Trigger.h"

namespace mra {

enum BeaconType : uint8_t { BEACON_ROTATING, BEACON_AIRPORT, BEACON_TOWER };

class Beacon {
 public:
  // 🎛️ Tuning. Set fields before begin().
  struct Config {
    BeaconType type = BEACON_ROTATING;
    float    rpm = 60;                 // ROTATING ~45–90; AIRPORT 12–15 (FAA: 24–30 flashes/min)
    uint8_t  reflectors = 1;           // ROTATING: 1 or 2 beams
    uint8_t  sharpness = 8;            // bigger = narrower, snappier beam (1–16)
    uint8_t  level = 255;              // peak brightness
    uint8_t  ambient = 10;             // glow when the beam points away
    uint16_t spinUpMs = 2500;          // motor getting up to speed
    uint16_t spinDownMs = 3500;        // coasting to a stop
    uint8_t  towerFlashesPerMin = 30;  // TOWER: prototype 20–40
    uint8_t  towerOnPercent = 50;      // TOWER: share of each cycle that's lit
    uint8_t  fadeEase = 6;             // lamp warm-up / cool-down (1 = instant)
  } cfg;

  // Pin B is only used by AIRPORT (green) and TOWER (second tier).
  Beacon(uint8_t pinA = 9, uint8_t pinB = NO_PIN) : pinA_(pinA), pinB_(pinB) {}

  void begin() {
    outputPin(pinA_);
    outputPin(pinB_);
    lastTick_ = millis();
  }

  void update(uint32_t now, bool on) {
    uint32_t dt = now - lastTick_;
    if (dt < TICK_MS) return;
    lastTick_ = now;

    // 🔌 lamp power: ease toward full or dark (incandescent warm-up / cool-down)
    easeToward(power_, on ? 1000 : 0, cfg.fadeEase);

    int a, b;
    if (cfg.type == BEACON_TOWER) {
      uint32_t period = 60000UL / cfg.towerFlashesPerMin;
      if (on && !wasOn_) towerStart_ = now;   // start lit
      bool lit = on && ((now - towerStart_) % period) < period * cfg.towerOnPercent / 100;
      easeToward(tower_, lit ? cfg.level : 0, cfg.fadeEase);
      a = b = tower_;
    } else {
      // ⚙️ motor: spin up while on, coast down while off
      if (on)  spin_ += (float)dt / cfg.spinUpMs;
      else     spin_ -= (float)dt / cfg.spinDownMs;
      spin_ = constrain(spin_, 0.0f, 1.0f);
      angle_ += dt * cfg.rpm * spin_ / 60000.0f;
      angle_ -= (int)angle_;                 // keep it within one turn

      int beam = beamLevel(angle_);
      if (cfg.type == BEACON_ROTATING && cfg.reflectors > 1) {
        int back = beamLevel(angle_ + 0.5f);
        if (back > beam) beam = back;
      }
      a = beam;
      b = (cfg.type == BEACON_AIRPORT) ? beamLevel(angle_ + 0.5f) : 0;
      a = (long)a * power_ / 1000;
      b = (long)b * power_ / 1000;
    }
    wasOn_ = on;
    writeLevel(pinA_, a);
    writeLevel(pinB_, b);
  }

 private:
  static const uint8_t TICK_MS = 10;

  // brightness of a beam pointing `turns` of a revolution away from you
  int beamLevel(float turns) const {
    float c = cos(2 * PI * turns);
    if (c < 0) c = 0;
    float p = 1;
    for (uint8_t i = 0; i < cfg.sharpness; i++) p *= c;   // narrow the beam
    return cfg.ambient + (int)((cfg.level - cfg.ambient) * p + 0.5f);
  }

  uint8_t  pinA_, pinB_;
  uint32_t lastTick_ = 0, towerStart_ = 0;
  float    spin_ = 0, angle_ = 0;
  int      power_ = 0, tower_ = 0;
  bool     wasOn_ = false;
};

}  // namespace mra
