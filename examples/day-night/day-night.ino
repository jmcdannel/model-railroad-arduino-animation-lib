/*
 * 🌅 Day-Night Room Lighting — Arduino Nano
 * ------------------------------------------
 * Dawn, day, dusk and night on three channels of LED strip: daylight white,
 * warm white (sunrise and sunset) and blue (moonlight). Dusk goes white →
 * golden → orange → blue hour → moonlight; dawn runs it back. A "night"
 * output goes HIGH partway through dusk and LOW partway through dawn, to
 * switch on building lights, street lamps or a neon sign.
 *
 * Wiring: LED strips need far more current than a pin can give. Each
 * channel goes through a logic-level N-channel MOSFET (IRLZ44N, or a
 * ready-made MOSFET module):
 *   D9  → MOSFET gate → daylight-white strip (−)
 *   D10 → MOSFET gate → warm-white strip (−)
 *   D11 → MOSFET gate → blue strip (−)
 *   Strip (+) → 12 V supply (+). Supply (−) → MOSFET sources → Nano GND.
 *   Add a 10 kΩ resistor from each gate to GND so the strips stay off while
 *   the Nano boots.
 *   D2  → night output: to D12 of other effect boards (plus GND to GND), or
 *         to a transistor / relay module for building lights.
 * Just testing? Three LEDs with 220 Ω resistors on D9–D11 show the same thing.
 *
 * The effect itself lives in src/MRA_DayNight.h.
 */
#include <MRA_DayNight.h>
using namespace mra;

// ---------------------------------------------------------------- Pins 📌
DayNight room(9, 10, 11, 2);               // day, warm, night, night output

// ---------------------------------------------------------- Start mode 🎛️
// POWER_UP: the day-night cycle runs from power-up. To switch it from D12:
//   Trigger trigger(WHILE_TRIGGERED, 12);              DEJA IO pin / relay / switch
//   Trigger trigger(TOGGLE, 12, ACTIVE_LOW);           button
// With c.onMeansDay = false (default): on = cycle runs, off = lights fade out
// and the clock pauses. With true: on = sunrise into day, off = sunset into
// night (a "daylight" switch for DEJA.js).
Trigger trigger(POWER_UP);

// ------------------------------------------------------------- Tuning 🎛️
// Every setting and its default is in src/MRA_DayNight.h. Uncomment to change.
void tune(DayNight::Config &c) {
  // c.dayMs  = 600000;   c.duskMs = 120000;   // 10 min day, 2 min sunset
  // c.nightMs = 240000;  c.dawnMs = 90000;    // 4 min night, 1.5 min sunrise
  // c.nightLevel = 60;                        // moonlight brightness
  // c.onMeansDay = false;
  // c.startPhase = PHASE_DAY;                 // PHASE_DAY, PHASE_DUSK, PHASE_NIGHT, PHASE_DAWN
}

void setup() {
  tune(room.cfg);
  room.begin();
  trigger.begin();
}

void loop() {
  uint32_t now = millis();
  room.update(now, trigger.update(now));
}
