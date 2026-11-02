/*
 * 🚥 Traffic Light — Arduino Nano
 * --------------------------------
 * One intersection, two phase groups:
 *   Group A = north/south heads     Group B = east/west heads
 * Wire ONE head on group A for a single traffic light. Wire TWO heads on
 * each group (LEDs in parallel, each with its own resistor) for a full
 * four-way intersection — all four heads stay in sync automatically.
 *
 * Cycle: A green → A yellow → all red → B green → B yellow → all red → …
 *
 * Wiring (pin → resistor → LED anode, cathode → GND):
 *   Group A:  D2 → red   D3 → yellow   D4 → green
 *   Group B:  D5 → red   D6 → yellow   D7 → green
 * With two heads per group use 470 Ω resistors so each pin sources well
 * under its 40 mA limit. These are plain on/off pins — no PWM needed.
 *
 * The effect itself lives in src/MRA_TrafficLight.h.
 */
#include <MRA_TrafficLight.h>
using namespace mra;

// ---------------------------------------------------------------- Pins 📌
TrafficLight lights(2, 3, 4, 5, 6, 7);     // A red, yellow, green, B red, yellow, green

// ---------------------------------------------------------- Start mode 🎛️
// POWER_UP: cycles from power-up. To switch it from D12 instead:
//   Trigger trigger(WHILE_TRIGGERED, 12);              DEJA IO pin / relay / switch
//   Trigger trigger(TOGGLE, 12, ACTIVE_LOW);           button: day ↔ night mode
// Off: night-flash (A yellow, B red), or dark with c.offNightFlash = false.
// On: the normal cycle, entered through all-red.
Trigger trigger(POWER_UP);

// ------------------------------------------------------------- Tuning 🎛️
// Every setting and its default is in src/MRA_TrafficLight.h. Uncomment to change.
void tune(TrafficLight::Config &c) {
  // c.greenAMs = 14000;   c.greenBMs = 9000;   // main road gets the longer green
  // c.yellowMs = 3000;    c.allRedMs = 1500;
  // c.nightFlash = false;                      // true = always late-night flashing
  // c.offNightFlash = true;                    // off → flash (true) or dark (false)
}

void setup() {
  tune(lights.cfg);
  lights.begin();
  trigger.begin();
}

void loop() {
  uint32_t now = millis();
  lights.update(now, trigger.update(now));
}
