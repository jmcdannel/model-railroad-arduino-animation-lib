/*
 * 🚨 Rotating Beacon — Arduino Nano
 * ----------------------------------
 * A lamp with a turning reflector, faked with brightness: as the beam sweeps
 * past, the LED swells to a sharp peak and falls back to a faint glow. Three
 * kinds:
 *   BEACON_ROTATING  amber on a tow truck, plant roof or snow plow; red on a
 *                    fire station (one or two beams)
 *   BEACON_AIRPORT   white + green on opposite sides, turning slowly
 *   BEACON_TOWER     red obstruction light on a radio or water tower, fading
 *                    on and off ~30 times a minute
 * Rotating types spin up when switched on and coast down when switched off.
 *
 * Wiring (pin → resistor → LED anode, cathode → GND):
 *   D9  → 220 Ω → amber / red / white LED (lamp A)
 *   D10 → 220 Ω → green LED (airport) or a second red LED (tower's 2nd tier)
 * Both must be PWM pins (3, 5, 6, 9, 10, 11): the sweep is a fade.
 *
 * The effect itself lives in src/MRA_Beacon.h.
 */
#include <MRA_Beacon.h>
using namespace mra;

// ---------------------------------------------------------------- Pins 📌
Beacon beacon(9, 10);                      // lamp A, lamp B (NO_PIN if unused)

// ---------------------------------------------------------- Start mode 🎛️
// POWER_UP: turns from power-up. To switch it from D12 instead:
//   Trigger trigger(WHILE_TRIGGERED, 12);              DEJA IO pin / relay / switch
//   Trigger trigger(TOGGLE, 12, ACTIVE_LOW);           button on / button off
// Off: the lamp fades out while the motor coasts to a stop.
Trigger trigger(POWER_UP);

// ------------------------------------------------------------- Tuning 🎛️
// Every setting and its default is in src/MRA_Beacon.h. Uncomment to change.
void tune(Beacon::Config &c) {
  c.type = BEACON_ROTATING;                // BEACON_ROTATING, BEACON_AIRPORT, BEACON_TOWER
  // c.rpm = 60;                           // ROTATING ~45–90, AIRPORT 12–15
  // c.reflectors = 2;                     // ROTATING: double beam
  // c.sharpness = 8;                      // bigger = narrower, snappier beam
  // c.ambient = 10;                       // glow when the beam points away
  // c.towerFlashesPerMin = 30;            // TOWER: 20–40
}

void setup() {
  tune(beacon.cfg);
  beacon.begin();
  trigger.begin();
}

void loop() {
  uint32_t now = millis();
  beacon.update(now, trigger.update(now));
}
