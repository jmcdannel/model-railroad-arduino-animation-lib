// 🚧 Crossing gates: one or two hobby servos added to a Crossing.
//
// Prototype sequence: the flashers start (and the bell rings), the gates
// wait a few seconds, then lower slowly and settle with a small bounce. When
// the train has cleared they rise, and the flashers keep going until the
// gates are fully up. If another train arrives while they're rising, they
// come straight back down.
//
// Needs the Servo library (arduino-cli lib install Servo). Servo takes over
// Timer1, so D9 and D10 can no longer fade LEDs: put the servos there and the
// flasher lamps on D5 and D6.
//
// The servos are released between moves (cfg.detachMs) so they don't buzz
// and creep; the gate's own friction holds it.
#pragma once
#include <Servo.h>
#include "MRA_Crossing.h"

namespace mra {

class CrossingGates {
 public:
  // 🎛️ Tuning. Set fields before begin().
  struct Config {
    uint8_t  upDeg = 90;               // servo angle with gate 1 raised…
    uint8_t  downDeg = 0;              // …and lowered. Gate 2 is mirrored (180 − angle)
    bool     mirrorGate2 = true;       //   false = gate 2 uses the same angles
    uint16_t delayMs = 3000;           // flashers before the gates move (prototype ≥ 3 s)
    uint16_t lowerMs = 7000;           // time to lower fully
    uint16_t raiseMs = 5000;           // time to raise fully
    uint8_t  bounceDeg = 4;            // settle bounce when the arm lands (0 = none)
    uint16_t bounceMs = 700;
    bool     bellUntilDown = true;     // bell stops once the gates are down (common
                                       //   practice); false = rings the whole time
    uint16_t detachMs = 800;           // release the servos this long after a move
                                       //   (0 = always hold)
    uint16_t minUs = 544, maxUs = 2400;   // servo pulse range for 0° and 180°
  } cfg;

  // gate2Pin can be NO_PIN for a single gate. bellPin (optional) is HIGH while
  // the bell should ring: wire it to a Sound board's trigger input.
  CrossingGates(Crossing &crossing, uint8_t gate1Pin = 9, uint8_t gate2Pin = 10, uint8_t bellPin = 7)
    : crossing_(crossing), pin1_(gate1Pin), pin2_(gate2Pin), bellPin_(bellPin) {}

  void begin() {
    outputPin(bellPin_);
    pos_ = 0;
    writeServos();                       // set the pulse before attaching: no jump
    attach();
    writeServos();
    restSince_ = lastTick_ = millis();
  }

  bool bell() const { return bell_; }
  bool down() const { return state_ == DOWN; }

  void update(uint32_t now) {
    if (now - lastTick_ < TICK_MS) return;
    lastTick_ = now;
    bool train = crossing_.active();

    switch (state_) {
      case UP:
        if (train) { state_ = WAITING; since_ = now; }
        break;
      case WAITING:                        // flashers only, gates still up
        if (!train) state_ = UP;
        else if (now - since_ >= cfg.delayMs) startMove(LOWERING, now);
        break;
      case LOWERING:
        if (!train) startMove(RAISING, now);
        else if (moveDone(now)) { state_ = DOWN; since_ = now; }
        break;
      case DOWN:
        if (!train) startMove(RAISING, now);
        break;
      case RAISING:
        if (train) startMove(LOWERING, now);   // another train: straight back down
        else if (moveDone(now)) { state_ = UP; restSince_ = now; }
        break;
    }

    // 📐 gate position (0 = up, 1 = down)
    if (state_ == LOWERING || state_ == RAISING) {
      uint32_t el = now - since_;
      float t = moveMs_ ? (float)el / moveMs_ : 1;
      if (t > 1) t = 1;
      float eased = (1 - cos(PI * t)) / 2;          // ease in and out
      pos_ = from_ + (to_ - from_) * eased;
      restSince_ = now;
    } else if (state_ == DOWN) {
      pos_ = 1;
      uint32_t el = now - since_;
      int span = abs((int)cfg.downDeg - (int)cfg.upDeg);
      if (cfg.bounceDeg && span && el < cfg.bounceMs) {   // two shrinking bounces
        float t = (float)el / cfg.bounceMs;
        pos_ = 1 - (float)cfg.bounceDeg / span * fabs(sin(2 * PI * t)) * (1 - t);
        restSince_ = now;
      }
    } else {
      pos_ = 0;
    }

    bool moving = (now - restSince_) < cfg.detachMs || cfg.detachMs == 0;
    if (moving && !attached_) attach();
    writeServos();
    if (!moving && attached_) detach();

    // 🔔 bell, and keep the flashers going until the gates are fully up
    bell_ = cfg.bellUntilDown ? (state_ == WAITING || state_ == LOWERING) : (state_ != UP);
    writeOn(bellPin_, bell_);
    crossing_.keepFlashing(state_ != UP);
  }

 private:
  static const uint8_t TICK_MS = 20;       // servos refresh every 20 ms anyway
  enum State : uint8_t { UP, WAITING, LOWERING, DOWN, RAISING };

  void startMove(State s, uint32_t now) {
    from_ = pos_;
    to_ = (s == LOWERING) ? 1 : 0;
    float full = (s == LOWERING) ? cfg.lowerMs : cfg.raiseMs;
    moveMs_ = (uint32_t)(full * fabs(to_ - from_));   // partial moves take less time
    state_ = s;
    since_ = now;
  }

  bool moveDone(uint32_t now) const { return now - since_ >= moveMs_; }

  uint16_t pulseFor(float deg) const { return cfg.minUs + (uint16_t)((cfg.maxUs - cfg.minUs) * deg / 180 + 0.5f); }

  void writeServos() {
    float deg = cfg.upDeg + ((float)cfg.downDeg - cfg.upDeg) * pos_;
    if (pin1_ != NO_PIN) s1_.writeMicroseconds(pulseFor(deg));
    if (pin2_ != NO_PIN) s2_.writeMicroseconds(pulseFor(cfg.mirrorGate2 ? 180 - deg : deg));
  }

  void attach() {
    if (pin1_ != NO_PIN) s1_.attach(pin1_, cfg.minUs, cfg.maxUs);
    if (pin2_ != NO_PIN) s2_.attach(pin2_, cfg.minUs, cfg.maxUs);
    attached_ = true;
  }

  void detach() {
    s1_.detach();
    s2_.detach();
    attached_ = false;
  }

  Crossing &crossing_;
  uint8_t   pin1_, pin2_, bellPin_;
  Servo     s1_, s2_;
  State     state_ = UP;
  float     pos_ = 0, from_ = 0, to_ = 0;
  uint32_t  since_ = 0, moveMs_ = 0, restSince_ = 0, lastTick_ = 0;
  bool      attached_ = false, bell_ = false;
};

}  // namespace mra
