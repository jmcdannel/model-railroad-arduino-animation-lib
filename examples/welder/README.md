# 🔥 Arc Welder Effect (Arduino Nano)

Blue + white LEDs flicker like an arc welder, then a red LED glows and slowly
cools. Tuck the LEDs inside a shop or industry building and the light spilling
out the windows and door says "somebody's welding in there."

Runs automatically from power-up. No buttons, no libraries.

## 🔌 Wiring

| Nano pin | Resistor   | LED   |
|----------|------------|-------|
| D9       | 220 Ω      | white |
| D10      | 220 Ω      | blue  |
| D11      | 150–220 Ω  | red   |
| GND      |            | all cathodes (short legs) |

Each LED: **pin → resistor → anode (long leg)**, cathode to GND.
Any 3 mm or 5 mm LED works. Warm-white reads as "shop light", cool-white reads
as "arc" — cool-white is the better pick here.

**Power:** USB, or 7–12 V DC into `VIN`. If you feed it from an accessory bus,
make sure it's DC (a bridge rectifier + capacitor if the bus is AC).

```
          ┌─────────────┐
   5V ────┤ Nano        │
          │   D9  ──[220Ω]──▶|── white ──┐
          │   D10 ──[220Ω]──▶|── blue  ──┤
          │   D11 ──[220Ω]──▶|── red   ──┤
          │   GND ───────────────────────┘
          └─────────────┘
```

## 📤 Upload

From the repo folder (no install needed):

```sh
arduino-cli compile --fqbn arduino:avr:nano --library . examples/welder
arduino-cli upload  --fqbn arduino:avr:nano -p /dev/cu.usbserial-XXXX examples/welder
# clone with old bootloader: --fqbn arduino:avr:nano:cpu=atmega328old
```

Or install the library once and open it from *File → Examples → Model
Railroad Animation*. Arduino IDE: download the repo as a ZIP, then *Sketch →
Include Library → Add .ZIP Library*. arduino-cli:
`arduino-cli config set library.enable_unsafe_install true`, then
`arduino-cli lib install --git-url https://github.com/jmcdannel/model-railroad-arduino-animation-lib.git`.
The example is *welder*.

## 🎬 What it does

```
IDLE ──▶ STRIKE ──▶ WELD ──▶ IDLE ──▶ …
 dark    1–4 white   bursts of white+blue flicker,
 3–12 s  pops        short pauses between, 6–16 s
                     └─ red heats up during the arc,
                        then fades out over ~8–10 s
```

The red LED runs on a tiny thermal model instead of a fixed fade: it warms
while the arc is on and cools exponentially when it stops, so a long weld
leaves a longer glow than a short one.

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

**When it's off:** no new weld jobs start, a job in progress stops, and the red glow cools down naturally.
## 🎛️ Tuning

Set these in the sketch's `tune()` block, e.g. `c.coolTauMs = 4000;`. Every
setting and its default is in [`src/MRA_Welder.h`](../../src/MRA_Welder.h).

- `idleMin/Max` — how long the shop sits dark between jobs
- `jobMin/Max` — how long each weld job lasts
- `burst*` / `pause*` — rhythm of the arc (longer pauses = moving along a seam)
- `white*` / `blue*` — flicker speed and brightness range of each color
- `heatUpMs` — how fast the red reaches full glow
- `coolTauMs` — how slowly the red fades (bigger = longer glow)
- `redMax` — cap the red brightness if it overpowers the scene

## 🙏 Credits

Based on the classic model-railroad arc-welder sketches: Steve Spence's
original, Paul & David Bradt's Model Railroader SN230 version, and the
Cornhill & Atherton finite-state-machine build on Model Railroad Hobbyist.
Rewritten from scratch with PWM intensity, burst structure and the thermal
red glow.
