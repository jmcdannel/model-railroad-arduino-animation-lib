/*
 * 🔔 Sound (DFPlayer Mini) — Arduino Nano
 * ----------------------------------------
 * Crossing bells, thunder, ambient sounds from a DFPlayer Mini MP3 module
 * and a small speaker. Pick a preset below:
 *   BELL     loops a bell while D12 is HIGH. Wire D12 to the crossing-gates
 *            bell output (D7), or to the crossing's active output (D5).
 *   THUNDER  plays one thunder clip per pulse on D12, after a random delay
 *            (farther strikes = longer delay and quieter). Wire D12 to the
 *            lightning example's thunder output (D2).
 *   AMBIENT  plays a random clip every 20–90 s from power-up: dogs, a
 *            whistle, distant thunder.
 *
 * SD card (FAT32): a folder named "mp3" holding 0001.mp3, 0002.mp3, …
 *   BELL uses 0001, THUNDER picks from 0002–0006, AMBIENT from 0007–0009.
 *   Change firstTrack / lastTrack below to match your card.
 *
 * Wiring:
 *   D10 → 1 kΩ → DFPlayer RX       DFPlayer TX → D11 (optional, unused)
 *   DFPlayer VCC → 5V, GND → GND, SPK_1 / SPK_2 → 8 Ω speaker (≤ 3 W)
 *   DFPlayer BUSY → D8 (optional: lets AMBIENT wait for a clip to finish)
 *   D12 ← trigger from another board (plus GND to GND)
 *
 * The effect itself lives in src/MRA_Sound.h.
 */
#include <MRA_Sound.h>
using namespace mra;

enum Preset { BELL, THUNDER, AMBIENT };
const Preset PRESET = BELL;

// ---------------------------------------------------------------- Pins 📌
Sound sound(10, 8, 11);                    // TX → player RX, BUSY, RX ← player TX

// ---------------------------------------------------------- Start mode 🎛️
// BELL and THUNDER listen to D12 (HIGH = ring / strike). AMBIENT runs from
// power-up. Other choices:
//   Trigger trigger(TOGGLE, 12, ACTIVE_LOW);           button: sound on / off
//   Trigger trigger(WHILE_TRIGGERED, 12, ACTIVE_LOW);  a button you hold
// Off: a looping sound fades out; AMBIENT stops picking new clips.
Trigger trigger(PRESET == AMBIENT ? POWER_UP : WHILE_TRIGGERED, 12);

// ------------------------------------------------------------- Tuning 🎛️
// Every setting and its default is in src/MRA_Sound.h.
void tune(Sound::Config &c) {
  c.volume = 20;                           // 0–30
  switch (PRESET) {
    case BELL:
      c.mode = SOUND_LOOP;
      c.firstTrack = c.lastTrack = 1;
      c.fadeOutMs = 300;
      break;
    case THUNDER:
      c.mode = SOUND_EACH_TRIGGER;
      c.firstTrack = 2;  c.lastTrack = 6;
      c.delayMin = 300;  c.delayMax = 4000;   // sound travels ~1 km in 3 s
      c.distanceDrop = 8;                     // farthest is 8 steps quieter
      break;
    case AMBIENT:
      c.mode = SOUND_RANDOM;
      c.firstTrack = 7;  c.lastTrack = 9;
      c.gapMin = 20000;  c.gapMax = 90000;
      break;
  }
}

void setup() {
  tune(sound.cfg);
  randomSeed(analogRead(A0));
  sound.begin();
  trigger.begin();
}

void loop() {
  uint32_t now = millis();
  sound.update(now, trigger.update(now));
}
