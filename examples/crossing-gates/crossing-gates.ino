/*
 * 🚧 Crossing Gates — Arduino Nano
 * ---------------------------------
 * The grade crossing flashers plus one or two servo-driven gate arms and a
 * bell output. A train hits a sensor: the flashers start and the bell rings,
 * the gates wait 3 s, lower slowly and land with a small bounce. When the
 * train has cleared, the gates rise and the flashers stop once they're up.
 *
 * Needs the Servo library:  arduino-cli lib install Servo
 *
 * Wiring:
 *   D5  → 220 Ω → red LED (left lamp)        D6 → 220 Ω → red LED (right lamp)
 *   D9  → gate 1 servo signal (orange/yellow)
 *   D10 → gate 2 servo signal (mirrored, for the other side of the road)
 *   D2  ← IR sensor A OUT    D3 ← IR sensor B OUT    D4 ← button → GND
 *   D7  → bell output: HIGH while the bell should ring. Wire it to D12 of a
 *         Nano running the sound example (plus GND to GND).
 * Servo power: red → 5V, brown/black → GND. Two micro servos are fine on the
 * Nano's 5V from a good USB supply; for bigger servos use a separate 5 V
 * supply with its GND joined to the Nano's GND. A 470 µF capacitor across
 * the servo supply stops resets when they start moving.
 * Servo takes over Timer1, so D9 and D10 can't fade LEDs: that's why the
 * lamps moved to D5 and D6 here.
 *
 * The effect lives in src/MRA_Crossing.h and src/MRA_CrossingGates.h.
 */
#include <MRA_CrossingGates.h>
using namespace mra;

// ---------------------------------------------------------------- Pins 📌
Crossing      crossing(5, 6, 2, 3, 4, NO_PIN);   // lamp L, lamp R, sensor A, sensor B, button, active out
CrossingGates gates(crossing, 9, 10, 7);         // gate 1, gate 2 (NO_PIN for one gate), bell out

// ---------------------------------------------------------- Start mode 🎛️
// Armed from power-up: the sensors and button run it. D12 is an extra way to
// fire it (DEJA IO pin, detection block…): gates down while it's asserted.
// Off by default because an unwired D12 reads HIGH, which would mean "train!".
const bool USE_TRIGGER = false;
Trigger trigger(WHILE_TRIGGERED, 12);

// ------------------------------------------------------------- Tuning 🎛️
// Every setting and its default is in src/MRA_Crossing.h and MRA_CrossingGates.h.
void tune(Crossing::Config &c, CrossingGates::Config &g) {
  // g.upDeg   = 90;    g.downDeg = 0;     // set these to match your servo horns
  // g.delayMs = 3000;                      // flashers before the gates move
  // g.lowerMs = 7000;  g.raiseMs = 5000;   // how long the arms take
  // g.bounceDeg = 4;                       // landing bounce (0 = none)
  // g.bellUntilDown = true;                // false = bell rings until the gates are up
  // c.holdMs  = 3000;                      // how long after the train clears
}

void setup() {
  tune(crossing.cfg, gates.cfg);
  crossing.begin();
  gates.begin();
  trigger.begin();
}

void loop() {
  uint32_t now = millis();
  bool fire = USE_TRIGGER && trigger.update(now);
  crossing.update(now, fire);
  gates.update(now);
}
