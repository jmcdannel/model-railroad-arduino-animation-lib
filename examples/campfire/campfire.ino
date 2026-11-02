/*
 * 🔥 Campfire / Fireplace Flicker — Arduino Nano
 * ------------------------------------------------
 * Three LEDs flicker like a fire: two "flame" LEDs (yellow + orange) dance
 * quickly and independently while a red "ember" LED breathes slowly
 * underneath. Random gusts flare the whole fire up; lulls let it settle.
 *
 * Works for a campfire, hobo barrel, fireplace behind a window, caboose
 * stove, forge, trash fire — anything that burns.
 *
 * Wiring (pin → resistor → LED anode, cathode → GND):
 *   D9  → 220 Ω → yellow LED        (flame 1)
 *   D10 → 220 Ω → orange/amber LED  (flame 2)
 *   D11 → 150–220 Ω → red LED       (ember)
 * Tip: bunch the LEDs together and diffuse them (tissue paper, a dab of
 * hot glue) so the colors blend into one flame.
 *
 * The effect itself lives in src/MRA_Campfire.h.
 */
#include <MRA_Campfire.h>
using namespace mra;

// ---------------------------------------------------------------- Pins 📌
Campfire fire(9, 10, 11);                  // flame 1, flame 2, ember

// ---------------------------------------------------------- Start mode 🎛️
// POWER_UP: burns from power-up. To switch it from D12 instead:
//   Trigger trigger(WHILE_TRIGGERED, 12);              DEJA IO pin / relay / switch
//   Trigger trigger(WHILE_TRIGGERED, 12, ACTIVE_LOW, 300000);  button → 5 minutes
//   Trigger trigger(TOGGLE, 12, ACTIVE_LOW);           button on / button off
// Off: the fire fades out; it relights the moment it's switched back on.
Trigger trigger(POWER_UP);

// ------------------------------------------------------------- Tuning 🎛️
// Every setting and its default is in src/MRA_Campfire.h. Uncomment to change.
void tune(Campfire::Config &c) {
  // c.flameLvlMin = 40;    c.flameLvlMax = 255;   // flame brightness range
  // c.flameEase   = 3;                            // 1 = snappy, 8+ = lazy
  // c.gustBoost   = 70;    c.lullDrop    = -60;   // how hard gusts and lulls hit
  // c.lullPercent = 35;                           // % of events that are lulls
}

void setup() {
  tune(fire.cfg);
  randomSeed(analogRead(A0));
  fire.begin();
  trigger.begin();
}

void loop() {
  uint32_t now = millis();
  fire.update(now, trigger.update(now));
}
