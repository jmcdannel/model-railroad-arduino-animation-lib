// 🚂 Model Railroad Animation — shared helpers for every effect.
#pragma once
#include <Arduino.h>

namespace mra {

const uint8_t NO_PIN = 255;   // "not wired" for optional pins

// ⏱️ wrap-safe "has this time arrived?" for millis() timestamps
inline bool due(uint32_t now, uint32_t at) { return (int32_t)(now - at) >= 0; }

inline int clamp255(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }

// 🎲 random value in [lo, hi], both ends included
inline long between(long lo, long hi) { return random(lo, hi + 1); }

// 💡 move cur 1/ease of the way toward target (at least one step)
inline void easeToward(int &cur, int target, uint8_t ease) {
  int diff = target - cur;
  int step = diff / ease;
  if (step == 0 && diff != 0) step = (diff > 0) ? 1 : -1;
  cur += step;
}

inline void outputPin(uint8_t pin) { if (pin != NO_PIN) { pinMode(pin, OUTPUT); digitalWrite(pin, LOW); } }
inline void writeLevel(uint8_t pin, int level) { if (pin != NO_PIN) analogWrite(pin, level); }
inline void writeOn(uint8_t pin, bool on) { if (pin != NO_PIN) digitalWrite(pin, on ? HIGH : LOW); }

// 🔘 debounced digital input; with pull-up so an unwired pin reads "released"
// unless activeLow is false (then unwired = asserted, matching DEJA's HIGH = on).
struct DebouncedInput {
  uint8_t  pin;
  bool     activeLow;
  uint8_t  debounceMs;
  bool     stable = false, lastRaw = false, seeded = false;
  uint32_t changedAt = 0;

  DebouncedInput(uint8_t pin, bool activeLow, uint8_t debounceMs = 30)
    : pin(pin), activeLow(activeLow), debounceMs(debounceMs) {}

  void begin() { if (pin != NO_PIN) pinMode(pin, INPUT_PULLUP); }

  // seedFromPin: start from the real pin state (no edge at power-up)
  bool read(uint32_t now, bool seedFromPin = true) {
    if (pin == NO_PIN) return false;
    bool raw = (digitalRead(pin) == LOW) == activeLow;
    if (!seeded) { seeded = true; if (seedFromPin) stable = raw; lastRaw = raw; changedAt = now; }
    if (raw != lastRaw) { lastRaw = raw; changedAt = now; }
    else if (raw != stable && now - changedAt >= debounceMs) stable = raw;
    return stable;
  }
};

}  // namespace mra
