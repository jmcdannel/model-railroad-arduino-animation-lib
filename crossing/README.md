# 🚦 Grade Crossing Flashers (Arduino Nano)

Two red lamps alternate at the prototype 50 flashes per minute with a soft
incandescent-style fade. Triggered by IR sensors in the track, a pushbutton,
or both. A spare output goes HIGH while the flashers run, ready for a bell
module or gate servo later.

## 🔌 Wiring

| Nano pin | Connects to                | Notes                                        |
|----------|----------------------------|----------------------------------------------|
| D9       | 220 Ω → red LED (left)     | cathode → GND                                |
| D10      | 220 Ω → red LED (right)    | cathode → GND                                |
| D2       | IR sensor A `OUT`          | sensor `VCC` → 5V, `GND` → GND               |
| D3       | IR sensor B `OUT`          | optional (`USE_SENSOR_B = false` if unused)  |
| D4       | pushbutton → GND           | optional; internal pull-up, no resistor      |
| D5       | "active" output            | optional; HIGH while flashing                |

**Sensors:** the cheap IR obstacle modules (FC-51, or TCRT5000 boards with a
digital `D0` out) pull `OUT` LOW when they see something. Mount one between
the ties pointing up, and turn its pot until it triggers on a car's underside
at 2–3 cm but not on the ballast. Strong room light or sunlight on the sensor
can cause false triggers. Shade it if needed.

## 🚂 How the trigger logic works

- **Two sensors (best):** one on each approach, a train length or more out
  from the crossing. The first sensor a train hits starts the flashers. They
  keep going until the train has passed the *far* sensor and been clear of
  it for `HOLD_MS` (3 s). Works in both directions, and gaps between cars
  never stop it early.
- **One sensor:** set `USE_SENSOR_B = false` and put it right at the
  crossing. Flashers run while the sensor sees the train, plus `HOLD_MS`.
- **Button:** press → flashers run for `BUTTON_RUN_MS` (20 s). Press again to
  extend. Works alongside the sensors.
- **Safety nets:** if sensor activity stops altogether (train reversed out),
  the flashers release after `RELEASE_TIMEOUT_MS` (15 s). `MAX_ACTIVE_MS`
  (2 min) caps a stalled train. If your trains take more than 15 s to get
  from one sensor to the other, raise `RELEASE_TIMEOUT_MS`.

## 📤 Upload

```sh
arduino-cli compile --fqbn arduino:avr:nano crossing
arduino-cli upload  --fqbn arduino:avr:nano -p /dev/cu.usbserial-XXXX crossing
# clone with old bootloader: --fqbn arduino:avr:nano:cpu=atmega328old
```

## 🎛️ External trigger (D12)

Off by default. To let DEJA.js (or a relay contact, or a toggle switch)
control this effect, set `USE_TRIGGER = true` and wire:

| From                      | To        |
|---------------------------|-----------|
| DEJA IO pin (Mega / ESP32 / Pico W) | Nano D12 |
| IO board GND              | Nano GND  |

HIGH runs the effect, matching DEJA's "on". The Nano uses its internal
pull-up, so an unplugged wire reads HIGH and the board keeps running on its
own. For a relay contact or switch to GND, or inverted logic, set
`TRIGGER_ACTIVE_LOW = true`. See the top-level README for the DEJA side.

**When the trigger is off:** nothing changes — the IR sensors and button keep working. Trigger **on** is a third input: the flashers run while D12 is asserted and for `HOLD_MS` after, so a DEJA sensor or route can fire the crossing.

> ⚠️ With `USE_TRIGGER = true`, an unplugged D12 reads HIGH, which means *flashers on*. Keep the wire connected or set `TRIGGER_ACTIVE_LOW = true` and use a contact to GND.

## 🎛️ Tuning

- `FLASHES_PER_MIN` — prototype range is 35–65; 50 is typical
- `INCANDESCENT_FADE`, `FADE_MS` — soft bulb-style ramp; set false for hard LED snap
- `HOLD_MS`, `RELEASE_TIMEOUT_MS`, `MAX_ACTIVE_MS`, `BUTTON_RUN_MS` — see above
- `SENSOR_ACTIVE_LOW` — set false if your sensor goes HIGH on detect
