/*
 * 💡 Fluorescent Startup — Arduino Nano
 * --------------------------------------
 * Shop, office, warehouse or station lights coming on: each tube waits a
 * moment, its ends glow while the starter preheats, it blinks once or a few
 * times as it tries to strike, then catches and settles to full brightness.
 * The tubes start a little apart, so the room fills in unevenly like the
 * real thing. Optionally one tube is worn out and keeps acting up.
 *
 * Wiring (pin → resistor → LED anode, cathode → GND):
 *   D9  → 220 Ω → cool-white LED (tube / fixture 1)
 *   D10 → 220 Ω → cool-white LED (tube / fixture 2)
 *   D11 → 220 Ω → cool-white LED (tube / fixture 3)
 * A whole building of fixtures on one pin: drive them through a transistor
 * or MOSFET instead of straight from the pin (~20 mA max per pin).
 *
 * The effect itself lives in src/MRA_Fluorescent.h.
 */
#include <MRA_Fluorescent.h>
using namespace mra;

// ---------------------------------------------------------------- Pins 📌
Fluorescent lights(9, 10, 11);             // up to 4 tubes; NO_PIN for unused ones

// ---------------------------------------------------------- Start mode 🎛️
// POWER_UP: the lights come on once, at power-up. To switch them from D12:
//   Trigger trigger(WHILE_TRIGGERED, 12);              DEJA IO pin / relay / switch
//   Trigger trigger(TOGGLE, 12, ACTIVE_LOW);           a wall-switch button
//   Trigger trigger(WHILE_TRIGGERED, 12, ACTIVE_HIGH, 60000);  PIR: someone walks in
// Off: all tubes go out at once. On again: the whole startup plays again.
Trigger trigger(POWER_UP);

// ------------------------------------------------------------- Tuning 🎛️
// Every setting and its default is in src/MRA_Fluorescent.h. Uncomment to change.
void tune(Fluorescent::Config &c) {
  // c.staggerMax  = 1500;                 // tubes start up to this far apart (ms)
  // c.attemptsMax = 5;                    // most blinks before a tube catches
  // c.badTube     = 1;                    // tube 2 (0-based) is worn out
  // c.badNeverStrikes = false;            // true = it never catches
}

void setup() {
  tune(lights.cfg);
  randomSeed(analogRead(A0));
  lights.begin();
  trigger.begin();
}

void loop() {
  uint32_t now = millis();
  lights.update(now, trigger.update(now));
}
