# 🚥 Traffic Light (Arduino Nano)

One intersection with two phase groups. Group A is north/south, group B is
east/west. Wire one head on group A for a single traffic light. Wire two
heads on each group for a full four-way intersection, and all four stay in
sync automatically.

Cycle: A green → A yellow → all red → B green → B yellow → all red → repeat.
Powers up in all-red. Set `nightFlash = true` for late-night mode (A flashes
yellow, B flashes red).

## 🔌 Wiring

| Nano pin | Group | LED    |
|----------|-------|--------|
| D2       | A     | red    |
| D3       | A     | yellow |
| D4       | A     | green  |
| D5       | B     | red    |
| D6       | B     | yellow |
| D7       | B     | green  |
| GND      |       | all cathodes |

Pin → resistor → anode, cathode → GND. Single head per group: 220 Ω. Two
heads per group (the four-way setup): wire the matching LEDs in parallel,
**each with its own 470 Ω resistor**, so each pin stays well under its 40 mA
limit. These are plain on/off pins, no PWM needed.

```
 four-way:   N head ─┐              E head ─┐
             S head ─┴─ group A     W head ─┴─ group B
```

## 📤 Upload

From the repo folder (no install needed):

```sh
arduino-cli compile --fqbn arduino:avr:nano --library . examples/traffic-light
arduino-cli upload  --fqbn arduino:avr:nano -p /dev/cu.usbserial-XXXX examples/traffic-light
# clone with old bootloader: --fqbn arduino:avr:nano:cpu=atmega328old
```

Or install the library once and open it from *File → Examples → Model
Railroad Animation*. Arduino IDE: download the repo as a ZIP, then *Sketch →
Include Library → Add .ZIP Library*. arduino-cli:
`arduino-cli config set library.enable_unsafe_install true`, then
`arduino-cli lib install --git-url https://github.com/jmcdannel/model-railroad-arduino-animation-lib.git`.
The example is *traffic-light*.

## 🎛️ Start mode

Pick one line in the sketch's *Start mode* block:

| Start mode | Wiring | Use it for |
|---|---|---|
| `Trigger trigger(powerUp);` (default) | none | runs from power-up |
| `Trigger trigger(whileTriggered, 12);` | D12 ← DEJA IO pin, relay, switch; GND ↔ GND | runs while D12 is HIGH |
| `Trigger trigger(whileTriggered, 12, activeLow, 60000);` | D12 ← button or IR sensor to GND | each press/detection runs 60 s |
| `Trigger trigger(TOGGLE, 12, activeLow);` | D12 ← button to GND | press on, press off |

`activeHigh` (the default) means HIGH = on, which is what a DEJA IO pin
or a PIR sensor sends; an unwired pin reads HIGH, so the board still runs
on its own. `activeLow` is for anything that pulls the pin to GND. See the
top-level README for the DEJA side.

**When it's off:** the intersection drops to night-flash (A yellow, B red). Set `offNightFlash = false` for dark instead. Switching on brings it back through all-red into the normal cycle.
## 🎛️ Tuning

Set these in the sketch's `tune()` block, e.g. `c.greenAMs = 20000;`. Every
setting and its default is in [`src/MRA_TrafficLight.h`](../../src/MRA_TrafficLight.h).

- `greenAMs`, `greenBMs` — green time per direction (main road gets more)
- `yellowMs`, `allRedMs` — yellow and the all-red clearance between phases
- `nightFlash`, `flashMs` — late-night flashing mode
