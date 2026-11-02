// ⛈️ Lightning storm: clusters of 1–6 flashes with random brightness, some
// stuttering, a fast afterglow after each flash, then nothing for a while.
// The storm drifts between active spells (strikes every few seconds) and
// quiet spells (a strike every half minute or so).
//
// Optional "thunder" output: goes HIGH for thunderPulseMs at the start of
// every strike. Wire it to a Sound board's trigger (or, on one Nano, pass
// lightning.thunder() to sound.update()) and the thunder follows the flash.
//
// Off: no strikes, a strike in progress is cut. The first strike comes about
// 2 s after it's switched back on.
#pragma once
#include "MRA_Trigger.h"

namespace mra {

class Lightning {
 public:
  // 🎛️ Tuning. Set fields before begin().
  struct Config {
    uint8_t  tickMs = 5;
    uint32_t activeSpellMin = 20000, activeSpellMax = 90000;   // storm rhythm
    uint32_t quietSpellMin  = 15000, quietSpellMax  = 60000;
    uint32_t gapActiveMin   = 1500,  gapActiveMax   = 9000;    // between strikes, active
    uint32_t gapQuietMin    = 12000, gapQuietMax    = 40000;   // between strikes, quiet
    uint8_t  flashesMin = 1,  flashesMax = 6;                  // flashes per strike
    uint16_t flashOnMin = 15, flashOnMax = 90;                 // ms a flash is lit
    uint16_t flashGapMin = 40, flashGapMax = 350;              // ms between flashes
    uint8_t  lvlMin = 70, lvlMax = 255;                        // flash brightness
    uint8_t  stutterPercent = 30;                              // % of flashes that stutter
    uint16_t stutterOnMin = 8,   stutterOnMax = 25;
    uint16_t stutterOffMin = 10, stutterOffMax = 40;
    uint8_t  afterglowDiv = 3;                // after a flash the level drops 1/3 per tick
    uint8_t  bPercentMin = 30, bPercentMax = 100;              // LED B shows this % of LED A
    uint16_t thunderPulseMs = 100;
    uint16_t firstStrikeMs = 3000;                             // after power-up
  } cfg;

  // flashB and thunder are optional: pass NO_PIN to leave them out
  Lightning(uint8_t flashAPin = 9, uint8_t flashBPin = 10, uint8_t thunderPin = 2)
    : pinA_(flashAPin), pinB_(flashBPin), pinThunder_(thunderPin) {}

  void begin() {
    pinMode(pinA_, OUTPUT);
    if (pinB_ != NO_PIN) pinMode(pinB_, OUTPUT);
    if (pinThunder_ != NO_PIN) { pinMode(pinThunder_, OUTPUT); digitalWrite(pinThunder_, LOW); }
    write();
    lastTick_ = millis();
    spellAt_ = lastTick_ + between(cfg.activeSpellMin, cfg.activeSpellMax);
    phase_ = WAITING;
    phaseAt_ = lastTick_ + cfg.firstStrikeMs;
  }

  // true while the thunder pulse is HIGH (for chaining effects on one board)
  bool thunder() const { return thunderOff_ != 0; }

  void update(uint32_t now, bool on) {
    if (now - lastTick_ < cfg.tickMs) return;
    lastTick_ = now;

    // 🌩️ the storm drifts between active and quiet spells
    if (due(now, spellAt_)) {
      activeSpell_ = !activeSpell_;
      spellAt_ = now + (activeSpell_ ? between(cfg.activeSpellMin, cfg.activeSpellMax)
                                     : between(cfg.quietSpellMin,  cfg.quietSpellMax));
    }

    if (thunderOff_ && due(now, thunderOff_)) {
      writeOn(pinThunder_, false);
      thunderOff_ = 0;
    }

    if (!on) {                                   // dark, and hold until it's back
      if (lvlA_ || lvlB_) { lvlA_ = 0; lvlB_ = 0; write(); }
      if (thunderOff_) { writeOn(pinThunder_, false); thunderOff_ = 0; }
      phase_ = WAITING;
      phaseAt_ = now + 2000;
      return;
    }

    if (due(now, phaseAt_)) {
      switch (phase_) {
        case WAITING:
          startStrike(now);
          break;

        case LIT:                                // flash (or stutter pulse) ends
          pulsesLeft_--;
          phase_ = DARK;
          if (pulsesLeft_ > 0) {
            phaseAt_ = now + between(cfg.stutterOffMin, cfg.stutterOffMax);
          } else {
            flashesLeft_--;
            phaseAt_ = now + between(cfg.flashGapMin, cfg.flashGapMax);
          }
          break;

        case DARK:
          if (pulsesLeft_ > 0) {                 // next stutter pulse, a bit dimmer
            lightOn(lvl_ * random(60, 101) / 100);
            phase_ = LIT;
            phaseAt_ = now + between(cfg.stutterOnMin, cfg.stutterOnMax);
          } else if (flashesLeft_ > 0) {
            startFlash(now);
          } else {
            scheduleStrike(now);
          }
          break;
      }
    }

    // ✨ afterglow: fast decay whenever we're not lit
    if (phase_ != LIT && (lvlA_ || lvlB_)) {
      lvlA_ -= lvlA_ / cfg.afterglowDiv + (lvlA_ ? 1 : 0);
      lvlB_ -= lvlB_ / cfg.afterglowDiv + (lvlB_ ? 1 : 0);
      if (lvlA_ < 0) lvlA_ = 0;
      if (lvlB_ < 0) lvlB_ = 0;
      write();
    }
  }

 private:
  enum Phase : uint8_t { WAITING, LIT, DARK };

  void write() {
    analogWrite(pinA_, lvlA_);
    writeLevel(pinB_, lvlB_);
  }

  void lightOn(int level) {
    lvlA_ = level;
    lvlB_ = (int)((long)level * between(cfg.bPercentMin, cfg.bPercentMax) / 100);
    write();
  }

  void scheduleStrike(uint32_t now) {
    phase_ = WAITING;
    phaseAt_ = now + (activeSpell_ ? between(cfg.gapActiveMin, cfg.gapActiveMax)
                                   : between(cfg.gapQuietMin,  cfg.gapQuietMax));
  }

  void startFlash(uint32_t now) {
    lvl_ = between(cfg.lvlMin, cfg.lvlMax);
    bool stutter = random(100) < cfg.stutterPercent;
    pulsesLeft_ = stutter ? random(3, 7) : 1;
    lightOn(lvl_);
    phase_ = LIT;
    phaseAt_ = now + (stutter ? between(cfg.stutterOnMin, cfg.stutterOnMax)
                              : between(cfg.flashOnMin,   cfg.flashOnMax));
  }

  void startStrike(uint32_t now) {
    flashesLeft_ = between(cfg.flashesMin, cfg.flashesMax);
    if (pinThunder_ != NO_PIN) { digitalWrite(pinThunder_, HIGH); }
    thunderOff_ = now + cfg.thunderPulseMs;
    if (thunderOff_ == 0) thunderOff_ = 1;      // 0 means "no pulse"
    startFlash(now);
  }

  uint8_t  pinA_, pinB_, pinThunder_;
  Phase    phase_ = WAITING;
  uint32_t lastTick_ = 0, phaseAt_ = 0, spellAt_ = 0, thunderOff_ = 0;
  bool     activeSpell_ = true;
  uint8_t  flashesLeft_ = 0, pulsesLeft_ = 0;
  int      lvl_ = 0, lvlA_ = 0, lvlB_ = 0;
};

}  // namespace mra
