// 🪧 Neon sign: up to four segments (letters, words, a border, an arrow…).
// Each segment strikes up with a quick buzzing flicker, then glows with a
// faint shimmer. Patterns:
//
//   NEON_STEADY     everything on (segments strike up a moment apart)
//   NEON_SPELL      segments light one by one, hold, flash together, go dark,
//                   repeat: "E·A·T  EAT EAT EAT"
//   NEON_CHASE      one segment at a time, round and round (arrows, borders)
//   NEON_BLINK      all on, all off ("OPEN" in a window)
//
// cfg.brokenSegment: that segment is on its way out. It sputters and drops
// out at random, sometimes staying dark for a second before catching again.
//
// Off: the tubes go out within a few tenths of a second (a quick fade, not a
// hard cut). On again: the pattern starts over with a fresh strike-up.
#pragma once
#include "MRA_Trigger.h"

namespace mra {

enum NeonPattern : uint8_t { NEON_STEADY, NEON_SPELL, NEON_CHASE, NEON_BLINK };

class Neon {
 public:
  // 🎛️ Tuning. Set fields before begin().
  struct Config {
    NeonPattern pattern = NEON_STEADY;
    uint8_t  level = 255;                     // full brightness (PWM 0–255)
    uint8_t  shimmer = 6;                     // ± random wobble while lit
    uint8_t  strikeFlickersMax = 4;           // buzzy flickers when a segment lights
    uint16_t strikeOnMin = 20,  strikeOnMax = 80;
    uint16_t strikeOffMin = 20, strikeOffMax = 120;
    uint16_t staggerMax = 250;                // NEON_STEADY: segments strike this far apart
    uint16_t stepMs = 600;                    // SPELL / CHASE: time per segment
    uint16_t holdMs = 2500;                   // SPELL: all lit before the flashes
    uint8_t  flashes = 3;                     // SPELL: flash-together count
    uint16_t flashOnMs = 450, flashOffMs = 300;
    uint16_t darkMs = 1200;                   // SPELL: dark gap before it repeats
    uint16_t blinkOnMs = 1500, blinkOffMs = 700;   // BLINK
    uint8_t  brokenSegment = NO_PIN;          // 0–3 = that segment is failing
    uint16_t brokenEveryMin = 2000, brokenEveryMax = 12000;   // ms between sputters
    uint8_t  brokenDeadPercent = 25;          // % of sputters that stay dark a while
    uint16_t brokenDeadMin = 300, brokenDeadMax = 1800;
  } cfg;

  // Segments in order. Use PWM pins (3, 5, 6, 9, 10, 11) for the fade and
  // shimmer; leave unused segments as NO_PIN.
  Neon(uint8_t seg0 = 9, uint8_t seg1 = NO_PIN, uint8_t seg2 = NO_PIN, uint8_t seg3 = NO_PIN)
    : pins_{seg0, seg1, seg2, seg3} {
    count_ = 0;
    for (uint8_t i = 0; i < 4 && pins_[i] != NO_PIN; i++) count_ = i + 1;
  }

  void begin() {
    lastTick_ = millis();
    for (uint8_t i = 0; i < count_; i++) {
      outputPin(pins_[i]);
      seg_[i] = Segment();
      seg_[i].offSince = lastTick_ - WARM_MS;    // tubes start cold
    }
    running_ = false;
  }

  void update(uint32_t now, bool on) {
    if (count_ == 0 || now - lastTick_ < TICK_MS) return;
    lastTick_ = now;

    if (on && !running_) startPattern(now);
    if (!on) running_ = false;

    if (running_ && due(now, stepAt_)) advancePattern(now);

    for (uint8_t i = 0; i < count_; i++) {
      bool want = running_ && (wantMask_ >> i) & 1;
      updateSegment(i, now, want);
    }
  }

 private:
  static const uint8_t TICK_MS = 10;
  static const uint16_t WARM_MS = 1000;     // dark longer than this = cold tube

  struct Segment {
    bool     want = false, lit = false;
    uint8_t  flickersLeft = 0;
    uint32_t nextAt = 0, brokenAt = 0, offSince = 0;
    int      level = 0;
  };

  // ---- pattern controller: decides which segments should be lit ----------
  void startPattern(uint32_t now) {
    running_ = true;
    step_ = 0;
    wantMask_ = 0;
    stepAt_ = startAt_ = now;
    if (cfg.pattern == NEON_STEADY) {         // stagger the strike-ups
      for (uint8_t i = 0; i < count_; i++) delay_[i] = random(cfg.staggerMax + 1);
    }
  }

  void advancePattern(uint32_t now) {
    uint8_t all = (1 << count_) - 1;
    switch (cfg.pattern) {
      case NEON_STEADY: {                     // each segment lights once its delay passes
        uint8_t mask = 0;
        for (uint8_t i = 0; i < count_; i++) if (now - startAt_ >= delay_[i]) mask |= 1 << i;
        wantMask_ = mask;
        stepAt_ = now + (mask == all ? 60000UL : TICK_MS);
        break;
      }
      case NEON_SPELL: {
        // steps: 0..count-1 add a segment, then hold, then flashes, then dark
        uint8_t flashSteps = cfg.flashes * 2;
        if (step_ < count_) {
          wantMask_ |= 1 << step_;
          stepAt_ = now + (step_ + 1 < count_ ? cfg.stepMs : cfg.holdMs);
        } else if (step_ < count_ + flashSteps) {
          bool off = ((step_ - count_) % 2) == 0;
          wantMask_ = off ? 0 : all;
          stepAt_ = now + (off ? cfg.flashOffMs : cfg.flashOnMs);
        } else {
          wantMask_ = 0;
          stepAt_ = now + cfg.darkMs;
          step_ = 0xFF;                       // wraps to 0 below
        }
        step_++;
        break;
      }
      case NEON_CHASE:
        wantMask_ = 1 << (step_ % count_);
        step_ = (step_ + 1) % count_;
        stepAt_ = now + cfg.stepMs;
        break;
      case NEON_BLINK:
        wantMask_ = (step_ & 1) ? 0 : all;
        stepAt_ = now + ((step_ & 1) ? cfg.blinkOffMs : cfg.blinkOnMs);
        step_ ^= 1;
        break;
    }
  }

  // ---- one tube: strike-up flicker, shimmer, failing-tube sputters ---------
  void updateSegment(uint8_t i, uint32_t now, bool want) {
    Segment &s = seg_[i];
    bool broken = (i == cfg.brokenSegment);

    if (want && !s.want) {                    // just switched on: a cold tube flickers as it
      bool cold = now - s.offSince >= WARM_MS;  //   strikes; a warm one relights at once
      s.flickersLeft = cold ? random(cfg.strikeFlickersMax + 1) : 0;
      s.lit = false;
      s.nextAt = now;
      s.brokenAt = now + between(cfg.brokenEveryMin, cfg.brokenEveryMax);
    }
    if (!want) {                              // quick fade out
      if (s.want) s.offSince = now;
      s.want = false;
      s.level -= s.level / 2 + 1;
      if (s.level < 0) s.level = 0;
      writeLevel(pins_[i], s.level);
      return;
    }

    s.want = true;

    if (s.flickersLeft == 0 && broken && due(now, s.brokenAt)) {   // failing tube acts up
      s.flickersLeft = between(3, 8);
      s.nextAt = now;
      s.brokenAt = now + between(cfg.brokenEveryMin, cfg.brokenEveryMax);
      if (random(100) < cfg.brokenDeadPercent) {                     // drops out for a while
        s.lit = false;
        s.nextAt = now + between(cfg.brokenDeadMin, cfg.brokenDeadMax);
      }
    }

    if (s.flickersLeft > 0) {
      if (due(now, s.nextAt)) {
        s.lit = !s.lit;
        if (!s.lit) s.flickersLeft--;
        s.nextAt = now + (s.lit ? between(cfg.strikeOnMin, cfg.strikeOnMax)
                                : between(cfg.strikeOffMin, cfg.strikeOffMax));
        s.level = s.lit ? between(cfg.level / 2, cfg.level) : 0;
      }
      if (s.flickersLeft == 0) s.lit = true;  // caught: steady from here
    } else {
      s.lit = true;
      s.level = clamp255((int)cfg.level - cfg.shimmer + random(2 * cfg.shimmer + 1));
    }
    writeLevel(pins_[i], s.lit ? s.level : 0);
  }

  uint8_t  pins_[4];
  uint8_t  count_;
  Segment  seg_[4];
  uint16_t delay_[4] = {0, 0, 0, 0};
  uint32_t lastTick_ = 0, stepAt_ = 0, startAt_ = 0;
  uint8_t  step_ = 0, wantMask_ = 0;
  bool     running_ = false;
};

}  // namespace mra
