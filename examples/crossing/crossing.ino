/*
 * 🚦 Grade Crossing Flashers — Arduino Nano
 * ------------------------------------------
 * Two red lamps alternate at the prototype 50 flashes per minute, with a
 * soft incandescent-style fade on each lamp. Triggered by IR sensors in the
 * track, a pushbutton, or both. Want gates too? See the crossing-gates
 * example.
 *
 * Wiring:
 *   D9  → 220 Ω → red LED (left lamp)    cathode → GND
 *   D10 → 220 Ω → red LED (right lamp)   cathode → GND
 *   D2  ← IR sensor A OUT   (FC-51 / TCRT5000 module: VCC→5V, GND→GND, OUT→D2)
 *   D3  ← IR sensor B OUT   (NO_PIN for a single sensor at the crossing)
 *   D4  ← pushbutton → GND  (internal pull-up, no resistor needed)
 *   D5  → optional "active" output: HIGH while the flashers run
 * The cheap IR obstacle modules pull OUT LOW when they see something. Mount
 * one between the ties pointing up and turn its pot until it triggers on a
 * car's underside at 2–3 cm but not on the ballast. Strong room light or
 * sunlight on the sensor can cause false triggers — shade it if needed.
 *
 * The effect itself lives in src/MRA_Crossing.h.
 */
#include <MRA_Crossing.h>
using namespace mra;

// ---------------------------------------------------------------- Pins 📌
Crossing crossing(9, 10, 2, 3, 4, 5);      // lamp L, lamp R, sensor A, sensor B, button, active out

// ---------------------------------------------------------- Start mode 🎛️
// The crossing is armed from power-up: its own sensors and button run it.
// D12 is an extra way to fire it (a DEJA IO pin, a detection block…):
// flashers run while D12 is asserted and for holdMs after. Off by default
// because an unwired D12 reads HIGH, which would mean "train!".
const bool USE_TRIGGER = false;
Trigger trigger(WHILE_TRIGGERED, 12);      // ACTIVE_LOW for a relay contact to GND

// ------------------------------------------------------------- Tuning 🎛️
// Every setting and its default is in src/MRA_Crossing.h. Uncomment to change.
void tune(Crossing::Config &c) {
  // c.flashesPerMin    = 50;      // prototype: 35–65
  // c.incandescentFade = true;    // false = hard LED snap
  // c.holdMs           = 3000;    // keep flashing after the train clears
  // c.buttonRunMs      = 20000;   // one button press
}

void setup() {
  tune(crossing.cfg);
  crossing.begin();
  trigger.begin();
}

void loop() {
  uint32_t now = millis();
  bool fire = USE_TRIGGER && trigger.update(now);
  crossing.update(now, fire);
}
