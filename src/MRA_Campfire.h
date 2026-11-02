// 🔥 Campfire / fireplace: two flame LEDs dance quickly and independently
// while a red ember breathes slowly underneath. Random gusts flare the whole
// fire up; lulls let it settle.
//
// Off: the fire fades out; it relights the moment it's switched back on.
#pragma once
#include "MRA_Trigger.h"

namespace mra {

class Campfire {
 public:
  // 🎛️ Tuning. Set fields before begin().
  struct Config {
    uint8_t  tickMs = 10;                              // update rate
    uint8_t  flameLvlMin = 40,   flameLvlMax = 255;    // flames: brightness range,
    uint16_t flameStepMin = 30,  flameStepMax = 140;   //   ms between new targets,
    uint8_t  flameEase = 3;                            //   easing (1 = snap, 8+ = lazy)
    uint8_t  emberLvlMin = 90,   emberLvlMax = 200;    // ember: slow, warm, never out
    uint16_t emberStepMin = 300, emberStepMax = 1500;
    uint8_t  emberEase = 12;
    uint32_t eventEveryMin = 3000, eventEveryMax = 12000;   // gusts and lulls
    uint16_t eventLenMin = 400,    eventLenMax = 2000;
    int16_t  gustBoost = 70;                           // added to flame targets in a gust
    int16_t  lullDrop = -60;                           // added during a lull
    uint8_t  lullPercent = 35;                         // % of events that are lulls
  } cfg;

  Campfire(uint8_t flame1Pin = 9, uint8_t flame2Pin = 10, uint8_t emberPin = 11)
    : pins_{flame1Pin, flame2Pin, emberPin} {}

  void begin() {
    for (uint8_t i = 0; i < 3; i++) {
      pinMode(pins_[i], OUTPUT);
      analogWrite(pins_[i], 0);
      bool ember = (i == 2);
      f_[i] = { pins_[i],
                ember ? cfg.emberLvlMin : cfg.flameLvlMin,   ember ? cfg.emberLvlMax : cfg.flameLvlMax,
                ember ? cfg.emberStepMin : cfg.flameStepMin, ember ? cfg.emberStepMax : cfg.flameStepMax,
                ember ? cfg.emberEase : cfg.flameEase, 0, 0, 0 };
    }
    uint32_t now = millis();
    for (uint8_t i = 0; i < 3; i++) pickTarget(f_[i], now, 0);
    eventAt_ = now + between(cfg.eventEveryMin, cfg.eventEveryMax);
    lastTick_ = now;
  }

  void update(uint32_t now, bool on) {
    if (now - lastTick_ < cfg.tickMs) return;
    lastTick_ = now;

    // 🌬️ gusts and lulls
    if (due(now, eventAt_)) {
      if (!eventOn_) {
        eventOn_ = true;
        boost_   = (random(100) < cfg.lullPercent) ? cfg.lullDrop : cfg.gustBoost;
        eventAt_ = now + between(cfg.eventLenMin, cfg.eventLenMax);
      } else {
        eventOn_ = false;
        boost_   = 0;
        eventAt_ = now + between(cfg.eventEveryMin, cfg.eventEveryMax);
      }
    }

    // 🔥 flames: new random target when due, then ease toward it
    for (uint8_t i = 0; i < 3; i++) {
      Flame &f = f_[i];
      int offset = (i == 2) ? boost_ / 3 : boost_;   // ember reacts gently
      if (!on)                     { f.target = 0; f.next = now; }   // fade out; relight at once
      else if (due(now, f.next))   pickTarget(f, now, offset);
      easeToward(f.cur, f.target, f.ease);
      analogWrite(f.pin, f.cur);
    }
  }

 private:
  struct Flame {
    uint8_t  pin;
    uint8_t  lvlMin, lvlMax;
    uint16_t stepMin, stepMax;
    uint8_t  ease;
    int      cur, target;
    uint32_t next;
  };

  void pickTarget(Flame &f, uint32_t now, int offset) {
    f.target = clamp255(between(f.lvlMin, f.lvlMax) + offset);
    f.next   = now + between(f.stepMin, f.stepMax);
  }

  uint8_t  pins_[3];
  Flame    f_[3];
  uint32_t lastTick_ = 0, eventAt_ = 0;
  bool     eventOn_ = false;
  int      boost_ = 0;
};

}  // namespace mra
