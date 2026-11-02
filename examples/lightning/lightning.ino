/*
 * ⛈️ Lightning Storm — Arduino Nano
 * ----------------------------------
 * White LEDs strobe like distant lightning: clusters of 1–6 flashes with
 * random brightness, some stuttering, a fast afterglow after each flash,
 * then nothing for a while. The storm drifts between active spells
 * (strikes every few seconds) and quiet spells (a strike every half minute
 * or so), so it never feels like a metronome.
 *
 * Wiring (pin → resistor → LED anode, cathode → GND):
 *   D9  → 150–220 Ω → white LED (main)
 *   D10 → 150–220 Ω → white LED (optional second; use NO_PIN to leave it out)
 *   D2  → optional "thunder" output: goes HIGH for 100 ms at the start of
 *         every strike. Wire it to D12 of a Nano running the sound example
 *         (plus GND to GND) and the thunder follows the lightning.
 * Hide the LEDs behind the backdrop or above a cloud flat, pointing at the
 * sky, so you see the flash on the backdrop rather than the LED itself.
 *
 * The effect itself lives in src/MRA_Lightning.h.
 */
#include <MRA_Lightning.h>
using namespace mra;

// ---------------------------------------------------------------- Pins 📌
Lightning storm(9, 10, 2);                 // flash A, flash B, thunder out

// ---------------------------------------------------------- Start mode 🎛️
// POWER_UP: storms from power-up. To switch it from D12 instead:
//   Trigger trigger(WHILE_TRIGGERED, 12);              DEJA IO pin / relay / switch
//   Trigger trigger(WHILE_TRIGGERED, 12, ACTIVE_LOW, 120000);  button → 2 minutes
//   Trigger trigger(TOGGLE, 12, ACTIVE_LOW);           button on / button off
// Off: dark, a strike in progress is cut. First strike ~2 s after it's back on.
Trigger trigger(POWER_UP);

// ------------------------------------------------------------- Tuning 🎛️
// Every setting and its default is in src/MRA_Lightning.h. Uncomment to change.
void tune(Lightning::Config &c) {
  // c.gapActiveMin = 1500;   c.gapActiveMax = 9000;    // between strikes, active spell
  // c.gapQuietMin  = 12000;  c.gapQuietMax  = 40000;   // between strikes, quiet spell
  // c.flashesMax   = 6;                                // flashes per strike
  // c.thunderPulseMs = 100;                            // thunder output pulse
}

void setup() {
  tune(storm.cfg);
  randomSeed(analogRead(A0));
  storm.begin();
  trigger.begin();
}

void loop() {
  uint32_t now = millis();
  storm.update(now, trigger.update(now));
}
