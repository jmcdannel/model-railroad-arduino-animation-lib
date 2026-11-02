// 🔥 Arc welder: blue/white arc flicker, then a red glow that cools down.
//
//   IDLE   → dark, random pause (welder repositioning / grabbing a new piece)
//   STRIKE → 1–4 short white pops as the rod tries to catch
//   WELD   → arc "bursts": white + blue flicker at independent random rates,
//            with short pauses between bursts (moving along the seam)
// The red LED follows a tiny thermal model: it heats while the arc runs and
// cools exponentially afterwards, so the glow lingers after the last burst.
//
// Off: no new jobs start, a job in progress stops, the red glow cools down.
//
// Based on the classic model-railroad arc-welder sketches by Steve Spence,
// Paul & David Bradt (Model Railroader SN230) and the Cornhill & Atherton
// finite-state-machine version on Model Railroad Hobbyist. Rewritten from
// scratch with PWM intensity, burst structure and a thermal red glow.
#pragma once
#include "MRA_Trigger.h"

namespace mra {

class Welder {
 public:
  // 🎛️ Tuning. All times in ms; every range is re-rolled each time.
  // Set fields before begin().
  struct Config {
    uint32_t idleMin = 3000,  idleMax = 12000;        // dark gap between weld jobs
    uint8_t  strikesMin = 1,  strikesMax = 4;         // pops before the arc catches
    uint16_t strikeOnMin = 30,  strikeOnMax = 110;
    uint16_t strikeOffMin = 200, strikeOffMax = 700;
    uint32_t jobMin = 6000,   jobMax = 16000;         // a job = bursts + pauses
    uint16_t burstMin = 700,  burstMax = 4000;
    uint16_t pauseMin = 150,  pauseMax = 900;
    uint16_t whiteOnMin = 5,  whiteOnMax = 60;        // white: fast, bright, mostly on
    uint16_t whiteOffMin = 5, whiteOffMax = 110;
    uint8_t  whiteLvlMin = 110, whiteLvlMax = 255;    // PWM 0–255
    uint16_t blueOnMin = 3,   blueOnMax = 25;         // blue: sparser, snappier
    uint16_t blueOffMin = 10, blueOffMax = 220;
    uint8_t  blueLvlMin = 140, blueLvlMax = 255;
    uint8_t  redMax = 255;                            // brightest the glow gets
    float    heatUpMs = 3500;                         // ms of arc to reach full heat
    float    coolTauMs = 2800;                        // bigger = slower, longer fade
    float    heatFloor = 0.01;                        // below this the LED is off
    uint16_t firstJobMs = 1500;                       // first job soon after power-up
  } cfg;

  Welder(uint8_t whitePin = 9, uint8_t bluePin = 10, uint8_t redPin = 11)
    : whitePin_(whitePin), bluePin_(bluePin), redPin_(redPin) {}

  void begin() {
    pinMode(whitePin_, OUTPUT);
    pinMode(bluePin_, OUTPUT);
    pinMode(redPin_, OUTPUT);
    analogWrite(whitePin_, 0);
    analogWrite(bluePin_, 0);
    analogWrite(redPin_, 0);
    white_ = { whitePin_, cfg.whiteOnMin, cfg.whiteOnMax, cfg.whiteOffMin, cfg.whiteOffMax, cfg.whiteLvlMin, cfg.whiteLvlMax, false, 0 };
    blue_  = { bluePin_,  cfg.blueOnMin,  cfg.blueOnMax,  cfg.blueOffMin,  cfg.blueOffMax,  cfg.blueLvlMin,  cfg.blueLvlMax,  false, 0 };
    lastTick_ = millis();
    enterIdle(lastTick_);
    stateUntil_ = lastTick_ + cfg.firstJobMs;
  }

  void update(uint32_t now, bool on) {
    switch (state_) {
      case IDLE:
        if (on && due(now, stateUntil_)) enterStrike(now);
        break;

      case STRIKE:
        if (!on) { analogWrite(whitePin_, 0); enterIdle(now); break; }
        if (due(now, phaseUntil_)) {
          if (strikeLit_) {                          // pop finished
            analogWrite(whitePin_, 0);
            strikeLit_ = false;
            strikesLeft_--;
            if (strikesLeft_ == 0) { enterWeld(now); break; }
            phaseUntil_ = now + between(cfg.strikeOffMin, cfg.strikeOffMax);
          } else {                                   // start a pop
            analogWrite(whitePin_, between(cfg.whiteLvlMin, cfg.whiteLvlMax));
            strikeLit_ = true;
            phaseUntil_ = now + between(cfg.strikeOnMin, cfg.strikeOnMax);
          }
        }
        break;

      case WELD:
        if (!on || due(now, stateUntil_)) { enterIdle(now); break; }
        if (due(now, phaseUntil_)) {                 // toggle burst ↔ pause
          arcOn_ = !arcOn_;
          phaseUntil_ = now + (arcOn_ ? between(cfg.burstMin, cfg.burstMax)
                                      : between(cfg.pauseMin, cfg.pauseMax));
        }
        break;
    }

    // ⚡ arc flicker: white + blue run independently while a burst is on
    bool arcActive = (state_ == WELD) && arcOn_;
    white_.update(now, arcActive);
    blue_.update(now, arcActive);

    // 🌡️ red glow: heat while arcing, exponential cool-down otherwise
    uint32_t dt = now - lastTick_;
    lastTick_ = now;
    if (dt) {
      if (arcActive) {
        heat_ += dt / cfg.heatUpMs;
        if (heat_ > 1.0) heat_ = 1.0;
      } else {
        heat_ -= heat_ * (dt / cfg.coolTauMs);       // first-order cooling
        if (heat_ < cfg.heatFloor) heat_ = 0.0;
      }
      analogWrite(redPin_, (uint8_t)(heat_ * heat_ * cfg.redMax + 0.5));   // gamma-ish
    }
  }

 private:
  // One independent random on/off flasher per arc LED
  struct Flicker {
    uint8_t  pin;
    uint16_t onMin, onMax, offMin, offMax;
    uint8_t  lvlMin, lvlMax;
    bool     on;
    uint32_t next;

    void update(uint32_t now, bool active) {
      if (!active) {
        if (on) { analogWrite(pin, 0); on = false; }
        next = now;                                  // fire at once on next activation
        return;
      }
      if (!due(now, next)) return;
      on = !on;
      if (on) {
        analogWrite(pin, between(lvlMin, lvlMax));
        next = now + between(onMin, onMax);
      } else {
        analogWrite(pin, 0);
        next = now + between(offMin, offMax);
      }
    }
  };

  enum State : uint8_t { IDLE, STRIKE, WELD };

  void enterIdle(uint32_t now) {
    state_ = IDLE;
    arcOn_ = false;
    stateUntil_ = now + between(cfg.idleMin, cfg.idleMax);
  }

  void enterStrike(uint32_t now) {
    state_ = STRIKE;
    strikesLeft_ = between(cfg.strikesMin, cfg.strikesMax);
    strikeLit_ = false;
    phaseUntil_ = now;                               // first pop right away
  }

  void enterWeld(uint32_t now) {
    state_ = WELD;
    stateUntil_ = now + between(cfg.jobMin, cfg.jobMax);
    arcOn_ = true;
    phaseUntil_ = now + between(cfg.burstMin, cfg.burstMax);
  }

  uint8_t  whitePin_, bluePin_, redPin_;
  Flicker  white_, blue_;
  State    state_ = IDLE;
  uint32_t stateUntil_ = 0, phaseUntil_ = 0, lastTick_ = 0;
  uint8_t  strikesLeft_ = 0;
  bool     strikeLit_ = false, arcOn_ = false;
  float    heat_ = 0.0;
};

}  // namespace mra
