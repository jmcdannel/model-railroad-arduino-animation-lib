# 💡 Fluorescent Startup (Arduino Nano)

Shop, office, warehouse or station lights coming on. Each tube waits a
moment, its ends glow dimly while the starter preheats, it blinks bright
once or a few times as it tries to strike, then catches and settles to full
brightness. The tubes start a little apart, so the room fills in unevenly
like the real thing. One tube can be worn out and keep acting up.

## 🔌 Wiring

| Nano pin | Resistor | LED | Role |
|----------|----------|-----|------|
| D9  | 220 Ω | cool white | tube / fixture 1 |
| D10 | 220 Ω | cool white | tube / fixture 2 |
| D11 | 220 Ω | cool white | tube / fixture 3 |
| GND |       | all cathodes | |

Up to four tubes (a fourth on D3, D5 or D6). PWM pins give the dim end-glow
and the warm-up; plain pins still blink and strike, just without the glow.
A whole building of fixtures on one pin needs a transistor or MOSFET: a pin
gives about 20 mA.

## 📤 Upload

From the repo folder (no install needed):

```sh
arduino-cli compile --fqbn arduino:avr:nano --library . examples/fluorescent
arduino-cli upload  --fqbn arduino:avr:nano -p /dev/cu.usbserial-XXXX examples/fluorescent
# clone with old bootloader: --fqbn arduino:avr:nano:cpu=atmega328old
```

Or install the library once and open it from *File → Examples → Model
Railroad Animation*. Arduino IDE: download the repo as a ZIP, then *Sketch →
Include Library → Add .ZIP Library*. arduino-cli:
`arduino-cli config set library.enable_unsafe_install true`, then
`arduino-cli lib install --git-url https://github.com/jmcdannel/model-railroad-arduino-animation-lib.git`.
The example is *fluorescent*.

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

**When it's off:** all tubes go out at once. On again: the whole startup plays again. A PIR sensor with `WHILE_TRIGGERED` and a run-on time makes a room that lights up when someone walks in.

## 🎬 What it does

```
WAIT ──▶ GLOW ──▶ BLINK ──▶ DARK ──▶ GLOW … ──▶ WARM ──▶ ON
0–1.5 s  dim ends  bright    short     1–5 tries   75%→100%
         preheat   flash     gap                   over 2.5 s
```

## 🎛️ Tuning

Set these in the sketch's `tune()` block, e.g. `c.badTube = 1;`. Every
setting and its default is in [`src/MRA_Fluorescent.h`](../../src/MRA_Fluorescent.h).

- `staggerMax` — how far apart the tubes start
- `attemptsMin/Max` — blinks before a tube catches
- `glowLevel`, `glowMin/Max` — the dim end-glow while preheating
- `blinkOn*`/`blinkOff*` — a failed strike
- `warmStartPercent`, `warmupMs` — the settle to full brightness
- `badTube` (0–3), `badNeverStrikes`, `badEvery*` — the worn-out tube
