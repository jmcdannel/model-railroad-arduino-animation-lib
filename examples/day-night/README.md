# 🌅 Day-Night Room Lighting (Arduino Nano)

Dawn, day, dusk and night on three channels of LED strip: daylight white,
warm white for sunrise and sunset, and blue for moonlight. Dusk goes white
→ golden → orange → blue hour → moonlight; dawn runs it back. Levels are
gamma-corrected, so the fades look even to the eye.

A **night output** goes HIGH halfway through dusk and LOW halfway through
dawn. Use it to switch building lights, street lamps, or the
[neon sign](../neon-sign/) on at dusk.

## 🔌 Wiring

LED strips draw far more current than an Arduino pin can give, so each
channel goes through a logic-level N-channel MOSFET (IRLZ44N, or a
ready-made MOSFET module):

| Nano pin | Goes to | Strip |
|----------|---------|-------|
| D9  | MOSFET gate | daylight white (−) |
| D10 | MOSFET gate | warm white (−) |
| D11 | MOSFET gate | blue (−) |
| D2  | night output | to D12 of other boards, or a transistor / relay module |

- Strip (+) → 12 V supply (+). Supply (−) → MOSFET sources → Nano GND.
- A 10 kΩ resistor from each gate to GND keeps the strips off while the
  Nano boots.
- Just testing? Three LEDs with 220 Ω resistors on D9–D11 show the same
  cycle.

## 📤 Upload

From the repo folder (no install needed):

```sh
arduino-cli compile --fqbn arduino:avr:nano --library . examples/day-night
arduino-cli upload  --fqbn arduino:avr:nano -p /dev/cu.usbserial-XXXX examples/day-night
# clone with old bootloader: --fqbn arduino:avr:nano:cpu=atmega328old
```

Or install the library once and open it from *File → Examples → Model
Railroad Animation*. Arduino IDE: download the repo as a ZIP, then *Sketch →
Include Library → Add .ZIP Library*. arduino-cli:
`arduino-cli config set library.enable_unsafe_install true`, then
`arduino-cli lib install --git-url https://github.com/jmcdannel/model-railroad-arduino-animation-lib.git`.
The example is *day-night*.

## 🎛️ Start mode

Pick one line in the sketch's *Start mode* block:

| Start mode | Wiring | Use it for |
|---|---|---|
| `Trigger trigger(POWER_UP);` (default) | none | runs from power-up |
| `Trigger trigger(WHILE_TRIGGERED, 12);` | D12 ← DEJA IO pin, relay, switch; GND ↔ GND | runs while D12 is HIGH |
| `Trigger trigger(WHILE_TRIGGERED, 12, ACTIVE_LOW, 60000);` | D12 ← button or IR sensor to GND | each press/detection runs 60 s |
| `Trigger trigger(TOGGLE, 12, ACTIVE_LOW);` | D12 ← button to GND | press on, press off |

`ACTIVE_HIGH` (the default) means HIGH = on, which is what a DEJA IO pin
or a PIR sensor sends; an unwired pin reads HIGH, so the board still runs
on its own. `ACTIVE_LOW` is for anything that pulls the pin to GND. See the
top-level README for the DEJA side.

**When it's off:** depends on `onMeansDay`. **false** (default): on runs the cycle; off fades the lights out and pauses the clock (on again resumes where it was). **true**: on = sunrise into day and stay there; off = sunset into night and stay there. That makes a "daylight" switch for DEJA.js. Flip it mid-sunset and it turns smoothly back into a sunrise.

## 🎬 What it does

```
DAY (10 min) ──▶ DUSK (2 min) ──▶ NIGHT (4 min) ──▶ DAWN (1.5 min) ──▶ DAY …
white            golden → orange   moonlight blue     first light → sunrise
                 → blue hour       night output ON    night output OFF
```

## 🎛️ Tuning

Set these in the sketch's `tune()` block, e.g. `c.nightMs = 300000;`. Every
setting and its default is in [`src/MRA_DayNight.h`](../../src/MRA_DayNight.h).

- `dayMs`, `duskMs`, `nightMs`, `dawnMs` — length of each part of the cycle
- `dayLevel`, `warmLevel`, `nightLevel` — the brightest each channel gets (keep moonlight dim)
- `onMeansDay` — what the trigger means (see Start mode)
- `startPhase` — where the cycle starts at power-up
- `lightsOnPercent`, `lightsOffPercent` — when the night output switches
- `offFadeMs` — fade-out when switched off in cycle mode

8-bit PWM shows small steps at the very dim end of a long fade; that's
normal for an Arduino Nano.
