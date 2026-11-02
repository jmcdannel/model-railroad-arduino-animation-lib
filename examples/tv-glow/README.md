# 📺 TV Glow (Arduino Nano)

The flickering blue-white light of a television seen through a window.
Scenes cut every few seconds to a new brightness and color mix, with gentle
flicker inside each scene, occasional bright flashes (explosions,
commercials), near-black moments, and the odd bout of channel surfing.

Runs from power-up, or from a button, sensor or DEJA.js pin (see Start mode).

## 🔌 Wiring

| Nano pin | Resistor | LED        | Notes                                   |
|----------|----------|------------|-----------------------------------------|
| D9       | 220 Ω    | cool white |                                         |
| D10      | 220 Ω    | blue       |                                         |
| D11      | 220 Ω    | red        | optional, adds warm scenes; leave off if you like |
| GND      |          | all cathodes |                                       |

Pin → resistor → anode, cathode → GND. Point the LEDs at the inside wall of
the room rather than straight out the window so the glow washes the room
the way a real screen does.

## 📤 Upload

From the repo folder (no install needed):

```sh
arduino-cli compile --fqbn arduino:avr:nano --library . examples/tv-glow
arduino-cli upload  --fqbn arduino:avr:nano -p /dev/cu.usbserial-XXXX examples/tv-glow
# clone with old bootloader: --fqbn arduino:avr:nano:cpu=atmega328old
```

Or install the library once and open it from *File → Examples → Model
Railroad Animation*. Arduino IDE: download the repo as a ZIP, then *Sketch →
Include Library → Add .ZIP Library*. arduino-cli:
`arduino-cli config set library.enable_unsafe_install true`, then
`arduino-cli lib install --git-url https://github.com/jmcdannel/model-railroad-arduino-animation-lib.git`.
The example is *tv-glow*.

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

**When it's off:** the screen fades to black and starts a fresh scene when it's switched back on.
## 🎛️ Tuning

Set these in the sketch's `tune()` block, e.g. `c.sceneMax = 8000;`. Every
setting and its default is in [`src/MRA_TvGlow.h`](../../src/MRA_TvGlow.h).

- `sceneMin/Max` — how long scenes last
- `whiteMin/Max`, `blueMin/Max`, `redMin/Max` — brightness range per color (lower `whiteMax` for a bluer TV)
- `flickerMax`, `microMin/Max` — how much and how fast it wobbles inside a scene
- `fadePercent` — share of scene changes that fade rather than cut
- `flashPercent`, `darkPercent`, `surfPercent` — how often the special moments happen
