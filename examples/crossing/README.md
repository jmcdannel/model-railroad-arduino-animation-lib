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
| D3       | IR sensor B `OUT`          | optional (`NO_PIN` if unused)              |
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
  it for `holdMs` (3 s). Works in both directions, and gaps between cars
  never stop it early.
- **One sensor:** pass `NO_PIN` for sensor B and put it right at the
  crossing. Flashers run while the sensor sees the train, plus `holdMs`.
- **Button:** press → flashers run for `buttonRunMs` (20 s). Press again to
  extend. Works alongside the sensors.
- **Safety nets:** if sensor activity stops altogether (train reversed out),
  the flashers release after `releaseTimeoutMs` (15 s). `maxActiveMs`
  (2 min) caps a stalled train. If your trains take more than 15 s to get
  from one sensor to the other, raise `releaseTimeoutMs`.

## 📤 Upload

From the repo folder (no install needed):

```sh
arduino-cli compile --fqbn arduino:avr:nano --library . examples/crossing
arduino-cli upload  --fqbn arduino:avr:nano -p /dev/cu.usbserial-XXXX examples/crossing
# clone with old bootloader: --fqbn arduino:avr:nano:cpu=atmega328old
```

Or install the library once and open it from *File → Examples → Model
Railroad Animation*. Arduino IDE: download the repo as a ZIP, then *Sketch →
Include Library → Add .ZIP Library*. arduino-cli:
`arduino-cli config set library.enable_unsafe_install true`, then
`arduino-cli lib install --git-url https://github.com/jmcdannel/model-railroad-arduino-animation-lib.git`.
The example is *crossing*.

## 🎛️ Start mode

The crossing is armed from power-up: its own sensors and button run it, no
trigger needed. D12 is an extra way to **fire** it (a DEJA.js IO pin, a
detection block, a route): set `USE_TRIGGER = true` in the sketch and wire
D12 ← DEJA IO pin, GND ↔ GND. The flashers run while D12 is HIGH and for
`holdMs` after. For a relay contact to GND use
`Trigger trigger(WHILE_TRIGGERED, 12, ACTIVE_LOW);`.

> ⚠️ With `USE_TRIGGER = true`, an unplugged D12 reads HIGH, which means
> *flashers on*. Keep the wire connected, or use `ACTIVE_LOW` and a contact
> to GND.

Want gates too? The [crossing-gates](../crossing-gates/) example adds one or
two servo gate arms and a bell output to this same crossing.
## 🎛️ Tuning

Set these in the sketch's `tune()` block, e.g. `c.holdMs = 5000;`. Every
setting and its default is in [`src/MRA_Crossing.h`](../../src/MRA_Crossing.h).

- `flashesPerMin` — prototype range is 35–65; 50 is typical
- `incandescentFade`, `fadeMs` — soft bulb-style ramp; set false for hard LED snap
- `holdMs`, `releaseTimeoutMs`, `maxActiveMs`, `buttonRunMs` — see above
- `sensorActiveLow` — set false if your sensor goes HIGH on detect
