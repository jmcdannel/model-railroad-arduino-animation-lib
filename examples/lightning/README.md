# ⛈️ Lightning Storm (Arduino Nano)

White LEDs strobe like distant lightning: clusters of 1–6 flashes with random
brightness, some stuttering, a fast afterglow after each flash, then nothing
for a while. The storm drifts between active spells (a strike every few
seconds) and quiet spells (one every half minute or so).

Runs from power-up, or from a button, sensor or DEJA.js pin (see Start mode).

## 🔌 Wiring

| Nano pin | Resistor  | Connects to      | Notes                                      |
|----------|-----------|------------------|--------------------------------------------|
| D9       | 150–220 Ω | white LED        | main flash                                 |
| D10      | 150–220 Ω | white LED        | optional second flash (`NO_PIN` if unused) |
| D2       |           | thunder trigger  | optional: HIGH for 100 ms at every strike  |
| GND      |           | all cathodes     |                                            |

Hide the LEDs behind the backdrop or above a cloud flat, pointing at the sky,
so you see the flash on the backdrop rather than the LED itself. Two LEDs a
foot or two apart give the storm some width.

**Thunder later:** D2 pulses at the start of every strike. Feed it to a sound
module (DFPlayer Mini through a small transistor, or another Arduino's input)
and the thunder follows the lightning. Pass `NO_PIN` as the third pin if unused.
On one Nano with the sound module, the multi-effect example links them
directly (`SOUND_SOURCE SOURCE_LIGHTNING`).

## 📤 Upload

From the repo folder (no install needed):

```sh
arduino-cli compile --fqbn arduino:avr:nano --library . examples/lightning
arduino-cli upload  --fqbn arduino:avr:nano -p /dev/cu.usbserial-XXXX examples/lightning
# clone with old bootloader: --fqbn arduino:avr:nano:cpu=atmega328old
```

Or install the library once and open it from *File → Examples → Model
Railroad Animation*. Arduino IDE: download the repo as a ZIP, then *Sketch →
Include Library → Add .ZIP Library*. arduino-cli:
`arduino-cli config set library.enable_unsafe_install true`, then
`arduino-cli lib install --git-url https://github.com/jmcdannel/model-railroad-arduino-animation-lib.git`.
The example is *lightning*.

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

**When it's off:** no strikes (one in progress is cut). The first strike comes about 2 s after it's switched back on.
## 🎛️ Tuning

Set these in the sketch's `tune()` block, e.g. `c.flashesMax = 4;`. Every
setting and its default is in [`src/MRA_Lightning.h`](../../src/MRA_Lightning.h).

- `activeSpell*`, `quietSpell*` — how long the storm stays busy or quiet
- `gapActive*`, `gapQuiet*` — time between strikes in each spell
- `flashesMin/Max`, `flashOn*`, `flashGap*` — shape of each strike
- `lvlMin/Max` — flash brightness range
- `stutterPercent` — share of flashes that stutter
- `bPercentMin/Max` — how much the second LED joins in
