# 🔥 Campfire / Fireplace Flicker (Arduino Nano)

Three LEDs flicker like a fire. Two "flame" LEDs (yellow and orange) dance
quickly and independently; a red "ember" LED breathes slowly underneath.
Random gusts flare the fire up, lulls let it settle. Works for a campfire,
hobo barrel, fireplace behind a window, caboose stove, forge, or trash fire.

Runs from power-up, or from a button, sensor or DEJA.js pin (see Start mode).

## 🔌 Wiring

| Nano pin | Resistor  | LED            | Role    |
|----------|-----------|----------------|---------|
| D9       | 220 Ω     | yellow         | flame 1 |
| D10      | 220 Ω     | orange / amber | flame 2 |
| D11      | 150–220 Ω | red            | ember   |
| GND      |           | all cathodes   |         |

Pin → resistor → anode (long leg), cathode → GND. Bunch the three LEDs
together and diffuse them (tissue paper, a dab of hot glue) so the colors
blend into one flame. For a fireplace, aim them at the back wall of the room.

## 📤 Upload

From the repo folder (no install needed):

```sh
arduino-cli compile --fqbn arduino:avr:nano --library . examples/campfire
arduino-cli upload  --fqbn arduino:avr:nano -p /dev/cu.usbserial-XXXX examples/campfire
# clone with old bootloader: --fqbn arduino:avr:nano:cpu=atmega328old
```

Or install the library once and open it from *File → Examples → Model
Railroad Animation*. Arduino IDE: download the repo as a ZIP, then *Sketch →
Include Library → Add .ZIP Library*. arduino-cli:
`arduino-cli config set library.enable_unsafe_install true`, then
`arduino-cli lib install --git-url https://github.com/jmcdannel/model-railroad-arduino-animation-lib.git`.
The example is *campfire*.

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

**When it's off:** the fire fades out over about a second and relights the moment it's switched back on.
## 🎛️ Tuning

Set these in the sketch's `tune()` block, e.g. `c.flameEase = 5;`. Every
setting and its default is in [`src/MRA_Campfire.h`](../../src/MRA_Campfire.h).

- `flameLvlMin/Max`, `flameStepMin/Max`, `flameEase` — flame brightness range, how often it changes, how snappy
- `ember*` — same for the red ember (slow and warm by default)
- `eventEvery*`, `eventLen*` — how often and how long gusts/lulls happen
- `gustBoost`, `lullDrop`, `lullPercent` — how strong they are and the gust/lull mix

Want a calmer fireplace? Raise `flameEase` to 5–6 and `flameStepMin` to 80.
