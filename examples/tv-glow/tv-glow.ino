/*
 * 📺 TV Glow — Arduino Nano
 * --------------------------
 * The flickering blue-white light of a television seen through a window.
 * Scenes cut every few seconds to a new brightness and color mix, with
 * gentle flicker inside each scene, occasional bright flashes (explosions,
 * commercials), near-black moments, and the odd bout of channel surfing.
 *
 * Wiring (pin → resistor → LED anode, cathode → GND):
 *   D9  → 220 Ω → cool-white LED
 *   D10 → 220 Ω → blue LED
 *   D11 → 220 Ω → red LED   (optional — adds warm scenes; leave unwired if you don't have one)
 * Point the LEDs at the inside wall of the room, not straight out the window,
 * so the glow washes the room like a real screen would.
 *
 * The effect itself lives in src/MRA_TvGlow.h.
 */
#include <MRA_TvGlow.h>
using namespace mra;

// ---------------------------------------------------------------- Pins 📌
TvGlow tv(9, 10, 11);                      // white, blue, red

// ---------------------------------------------------------- Start mode 🎛️
// POWER_UP: plays from power-up. To switch it from D12 instead:
//   Trigger trigger(WHILE_TRIGGERED, 12);              DEJA IO pin / relay / switch
//   Trigger trigger(TOGGLE, 12, ACTIVE_LOW);           button on / button off
//   Trigger trigger(WHILE_TRIGGERED, 12, ACTIVE_HIGH, 120000);  PIR sensor → 2 minutes
// Off: the screen fades to black; a fresh scene starts when it's back on.
Trigger trigger(POWER_UP);

// ------------------------------------------------------------- Tuning 🎛️
// Every setting and its default is in src/MRA_TvGlow.h. Uncomment to change.
void tune(TvGlow::Config &c) {
  // c.sceneMin = 600;   c.sceneMax = 6000;   // scene length (ms)
  // c.whiteMax = 200;   c.blueMax  = 255;    // overall brightness
  // c.flashPercent = 15;                     // % of scenes with a bright flash
  // c.surfPercent  = 6;                      // % chance of channel surfing
}

void setup() {
  tune(tv.cfg);
  randomSeed(analogRead(A0));
  tv.begin();
  trigger.begin();
}

void loop() {
  uint32_t now = millis();
  tv.update(now, trigger.update(now));
}
