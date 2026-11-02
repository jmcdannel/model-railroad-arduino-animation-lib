/*
 * 🪧 Neon Sign — Arduino Nano
 * ----------------------------
 * A diner, motel or bar sign with up to four segments (letters, words, a
 * border, an arrow). Each segment strikes up with a quick buzzy flicker,
 * then glows with a faint shimmer. Patterns: steady, spell-out-and-flash
 * ("E·A·T  EAT EAT EAT"), chase, or blink. One segment can be "failing":
 * it sputters and drops out now and then.
 *
 * Wiring (pin → resistor → LED anode, cathode → GND):
 *   D9  → 220 Ω → segment 1 (red / pink / blue LED, or LED filament)
 *   D10 → 220 Ω → segment 2
 *   D11 → 220 Ω → segment 3
 * More LEDs per segment: one resistor per LED, all on the same pin, up to
 * about 20 mA total per pin. Tip: put the LEDs behind a thin cut-out sign
 * face or inside clear tubing so the light spreads like a glass tube.
 *
 * Neon at dusk: wire D12 to the night output of a Nano running the
 * day-night example and use WHILE_TRIGGERED below.
 *
 * The effect itself lives in src/MRA_Neon.h.
 */
#include <MRA_Neon.h>
using namespace mra;

// ---------------------------------------------------------------- Pins 📌
Neon sign(9, 10, 11);                      // segments in order; NO_PIN for unused ones

// ---------------------------------------------------------- Start mode 🎛️
// POWER_UP: lights at power-up. To switch it from D12 instead:
//   Trigger trigger(WHILE_TRIGGERED, 12);              DEJA IO pin / day-night output
//   Trigger trigger(TOGGLE, 12, ACTIVE_LOW);           button: OPEN / CLOSED
// Off: the tubes go out. On again: a fresh strike-up.
Trigger trigger(POWER_UP);

// ------------------------------------------------------------- Tuning 🎛️
// Every setting and its default is in src/MRA_Neon.h. Uncomment to change.
void tune(Neon::Config &c) {
  c.pattern = NEON_SPELL;                  // NEON_STEADY, NEON_SPELL, NEON_CHASE, NEON_BLINK
  // c.stepMs  = 600;                      // SPELL / CHASE: time per segment
  // c.flashes = 3;                        // SPELL: flashes once it's all lit
  // c.brokenSegment = 2;                  // segment 3 (0-based) is failing
  // c.level   = 255;   c.shimmer = 6;     // brightness and glow wobble
}

void setup() {
  tune(sign.cfg);
  randomSeed(analogRead(A0));
  sign.begin();
  trigger.begin();
}

void loop() {
  uint32_t now = millis();
  sign.update(now, trigger.update(now));
}
