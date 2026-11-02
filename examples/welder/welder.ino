/*
 * 🔥 Arc Welder Effect — Arduino Nano
 * ------------------------------------
 * Drop it in a shop/industry building. Blue + white LEDs flicker like an arc,
 * then a red LED glows and slowly cools once the welder stops.
 *
 * Wiring (each LED: pin → resistor → anode, cathode → GND):
 *   D9  → 220 Ω → white LED
 *   D10 → 220 Ω → blue LED
 *   D11 → 150–220 Ω → red LED
 *   (D9/D10/D11 are all PWM pins on the Nano.)
 *
 * The effect itself lives in src/MRA_Welder.h. Credits in CREDITS.md.
 */
#include <MRA_Welder.h>
using namespace mra;

// ---------------------------------------------------------------- Pins 📌
Welder welder(9, 10, 11);                  // white, blue, red

// ---------------------------------------------------------- Start mode 🎛️
// POWER_UP: welds from power-up. To switch it from D12 instead:
//   Trigger trigger(WHILE_TRIGGERED, 12);              DEJA IO pin / relay / switch
//   Trigger trigger(WHILE_TRIGGERED, 12, ACTIVE_LOW, 60000);  button → 1 minute
//   Trigger trigger(TOGGLE, 12, ACTIVE_LOW);           button on / button off
// Off: no new jobs start, a job in progress stops, the red glow cools down.
Trigger trigger(POWER_UP);

// ------------------------------------------------------------- Tuning 🎛️
// Every setting and its default is in src/MRA_Welder.h. Uncomment to change.
void tune(Welder::Config &c) {
  // c.idleMin   = 3000;   c.idleMax = 12000;   // dark gap between jobs (ms)
  // c.jobMin    = 6000;   c.jobMax  = 16000;   // how long a job lasts (ms)
  // c.coolTauMs = 2800;                         // bigger = longer red glow
  // c.redMax    = 255;                          // cap the red if it's too strong
}

void setup() {
  tune(welder.cfg);
  randomSeed(analogRead(A0));              // floating pin → a different show every power-up
  welder.begin();
  trigger.begin();
}

void loop() {
  uint32_t now = millis();
  welder.update(now, trigger.update(now));
}
