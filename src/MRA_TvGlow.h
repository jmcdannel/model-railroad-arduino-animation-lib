// 📺 TV glow: the flickering blue-white light of a television through a
// window. Scenes cut every few seconds to a new brightness and color mix,
// with gentle flicker inside each scene, occasional bright flashes, near-black
// moments, and the odd bout of channel surfing.
//
// Off: the screen fades to black; a fresh scene starts when it's back on.
#pragma once
#include "MRA_Trigger.h"

namespace mra {

class TvGlow {
 public:
  // 🎛️ Tuning. Set fields before begin().
  struct Config {
    uint8_t  tickMs = 20;
    uint16_t sceneMin = 600, sceneMax = 6000;      // how long each scene lasts
    uint8_t  whiteMin = 8,  whiteMax = 200;        // brightness range per color
    uint8_t  blueMin  = 20, blueMax  = 255;
    uint8_t  redMin   = 0,  redMax   = 70;
    uint8_t  flickerMax = 35;                      // max ± wobble around the scene level
    uint16_t microMin = 40, microMax = 160;         // ms between wobbles
    uint8_t  cutEase = 1;                          // 1 = hard cut
    uint8_t  fadeEase = 12;                        // bigger = slower fade
    uint8_t  fadePercent = 25;                     // % of scene changes that fade
    uint8_t  flashPercent = 15;                    // % of scenes with a bright flash
    uint16_t flashMin = 30, flashMax = 120;
    uint8_t  darkPercent = 10;                     // % near-black scenes
    uint8_t  surfPercent = 6;                      // % channel surfing: 3–8 rapid cuts
    uint16_t surfMin = 150, surfMax = 400;
  } cfg;

  TvGlow(uint8_t whitePin = 9, uint8_t bluePin = 10, uint8_t redPin = 11)
    : pins_{whitePin, bluePin, redPin} {}

  void begin() {
    ch_[0] = { pins_[0], cfg.whiteMin, cfg.whiteMax, 0, 0, 0 };
    ch_[1] = { pins_[1], cfg.blueMin,  cfg.blueMax,  0, 0, 0 };
    ch_[2] = { pins_[2], cfg.redMin,   cfg.redMax,   0, 0, 0 };
    for (uint8_t i = 0; i < 3; i++) { pinMode(pins_[i], OUTPUT); analogWrite(pins_[i], 0); }
    lastTick_ = millis();
    ease_ = cfg.cutEase;
    newScene(lastTick_);
  }

  void update(uint32_t now, bool on) {
    if (now - lastTick_ < cfg.tickMs) return;
    lastTick_ = now;

    if (!on) {                                    // fade to black, fresh scene when back
      flashAt_ = 0; flashUntil_ = 0; sceneAt_ = now;
      for (uint8_t i = 0; i < 3; i++) ch_[i].target = 0;
      ease_ = 4;
    } else {
      if (due(now, sceneAt_)) newScene(now);

      // 💥 flash: slam everything bright, then let it ease back down
      if (flashAt_ && due(now, flashAt_)) {
        flashAt_ = 0;
        flashUntil_ = now + between(cfg.flashMin, cfg.flashMax);
        ch_[0].cur = 255; ch_[1].cur = 255; ch_[2].cur = 120;
      }
      if (flashUntil_) {
        if (!due(now, flashUntil_)) {
          for (uint8_t i = 0; i < 3; i++) analogWrite(ch_[i].pin, ch_[i].cur);
          return;
        }
        flashUntil_ = 0;
        ease_ = 3;                                // settle back fairly quickly
      }

      // 〰️ micro-flicker around the scene level
      if (due(now, microAt_)) {
        microAt_ = now + between(cfg.microMin, cfg.microMax);
        for (uint8_t i = 0; i < 3; i++)
          ch_[i].target = clamp255(ch_[i].base + random(-flicker_, flicker_ + 1));
      }
    }

    for (uint8_t i = 0; i < 3; i++) {
      easeToward(ch_[i].cur, ch_[i].target, ease_);
      analogWrite(ch_[i].pin, ch_[i].cur);
    }
  }

 private:
  struct Channel {
    uint8_t pin;
    uint8_t lvlMin, lvlMax;
    int     cur, base, target;
  };

  void newScene(uint32_t now) {
    uint16_t len;
    bool dark = false;

    if (surfLeft_ > 0) {                          // mid-surf: quick cut, short scene
      surfLeft_--;
      len = between(cfg.surfMin, cfg.surfMax);
      ease_ = cfg.cutEase;
    } else if (random(100) < cfg.surfPercent) {   // start surfing
      surfLeft_ = random(3, 9);
      len = between(cfg.surfMin, cfg.surfMax);
      ease_ = cfg.cutEase;
    } else {
      len   = between(cfg.sceneMin, cfg.sceneMax);
      ease_ = (random(100) < cfg.fadePercent) ? cfg.fadeEase : cfg.cutEase;
      dark  = (random(100) < cfg.darkPercent);
    }

    for (uint8_t i = 0; i < 3; i++) {
      ch_[i].base = dark ? random(0, 6) : between(ch_[i].lvlMin, ch_[i].lvlMax);
      ch_[i].target = ch_[i].base;
    }
    flicker_ = dark ? 2 : random(3, cfg.flickerMax + 1);
    sceneAt_ = now + len;
    flashAt_ = (!dark && random(100) < cfg.flashPercent) ? now + random(100, len) : 0;
  }

  uint8_t  pins_[3];
  Channel  ch_[3];
  uint32_t lastTick_ = 0, sceneAt_ = 0, microAt_ = 0, flashAt_ = 0, flashUntil_ = 0;
  uint8_t  ease_ = 1, flicker_ = 0, surfLeft_ = 0;
};

}  // namespace mra
