# 🪧 Neon Sign (Arduino Nano)

A diner, motel or bar sign with up to four segments: letters, words, a
border, an arrow. Each segment strikes up with a quick buzzy flicker, then
glows with a faint shimmer. One segment can be failing: it sputters and
drops out now and then, like a tube on its way out.

## 🔌 Wiring

| Nano pin | Resistor | LED | Role |
|----------|----------|-----|------|
| D9  | 220 Ω | red / pink / blue | segment 1 |
| D10 | 220 Ω | same | segment 2 |
| D11 | 220 Ω | same | segment 3 |
| GND |       | all cathodes | |

Use PWM pins (3, 5, 6, 9, 10, 11) for the fade and shimmer; a fourth
segment fits on D3, D5 or D6. Several LEDs per segment: one resistor per
LED, all on the same pin, up to about 20 mA per pin. Put them behind a thin
cut-out sign face or inside clear tubing so the light spreads like glass.

## 📤 Upload

From the repo folder (no install needed):

```sh
arduino-cli compile --fqbn arduino:avr:nano --library . examples/neon-sign
arduino-cli upload  --fqbn arduino:avr:nano -p /dev/cu.usbserial-XXXX examples/neon-sign
# clone with old bootloader: --fqbn arduino:avr:nano:cpu=atmega328old
```

Or install the library once and open it from *File → Examples → Model
Railroad Animation*. Arduino IDE: download the repo as a ZIP, then *Sketch →
Include Library → Add .ZIP Library*. arduino-cli:
`arduino-cli config set library.enable_unsafe_install true`, then
`arduino-cli lib install --git-url https://github.com/jmcdannel/model-railroad-arduino-animation-lib.git`.
The example is *neon-sign*.

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

**When it's off:** the tubes go out in a few tenths of a second. On again: a fresh strike-up. For a sign that lights at dusk, wire D12 to the night output of a board running [day-night](../day-night/) (or use `NEON_SOURCE SOURCE_NIGHT` in the multi-effect sketch).

## 🎬 What it does

| Pattern | Looks like |
|---|---|
| `NEON_STEADY` | everything on; segments strike up a moment apart |
| `NEON_SPELL` (example default) | E · A · T one by one, hold, flash 3×, dark, repeat |
| `NEON_CHASE` | one segment at a time, round and round (arrows, borders) |
| `NEON_BLINK` | all on, all off ("OPEN") |

A cold tube (dark for over a second) flickers as it strikes; a warm one
relights instantly, so spell-and-flash patterns flash cleanly.

## 🎛️ Tuning

Set these in the sketch's `tune()` block, e.g. `c.brokenSegment = 2;`. Every
setting and its default is in [`src/MRA_Neon.h`](../../src/MRA_Neon.h).

- `pattern` — one of the four above
- `stepMs`, `holdMs`, `flashes`, `flashOnMs`/`flashOffMs`, `darkMs` — timing of SPELL
- `blinkOnMs`/`blinkOffMs` — BLINK timing
- `strikeFlickersMax`, `strikeOn*`/`strikeOff*` — how buzzy the strike-up is
- `level`, `shimmer` — brightness and glow wobble
- `brokenSegment` (0–3), `brokenEvery*`, `brokenDeadPercent` — the failing tube
