# 🔔 Sound with a DFPlayer Mini (Arduino Nano)

Crossing bells, thunder and ambient sounds from a DFPlayer Mini MP3 module
(a few dollars) and a small speaker. The sketch has three presets:

| Preset | Mode | What it does | Wire D12 to |
|---|---|---|---|
| `BELL` (default) | `SOUND_LOOP` | loops track 1 while D12 is HIGH, fades out after | [crossing-gates](../crossing-gates/) bell output (D7) or [crossing](../crossing/) active output (D5) |
| `THUNDER` | `SOUND_EACH_TRIGGER` | one random clip (tracks 2–6) per pulse, after a 0.3–4 s delay; longer delay = farther = quieter | [lightning](../lightning/) thunder output (D2) |
| `AMBIENT` | `SOUND_RANDOM` | a random clip (tracks 7–9) every 20–90 s from power-up | nothing |

## 💾 SD card

FAT32, with a folder named `mp3` holding numbered files: `0001.mp3`,
`0002.mp3`, … The number is the track; anything after it is ignored, so
`0001_crossing_bell.mp3` works. Change `firstTrack` / `lastTrack` in the
sketch to match your card. For a seamless bell loop, trim the clip so it
starts and ends between rings.

## 🔌 Wiring

| Nano pin | DFPlayer pin | Notes |
|----------|--------------|-------|
| D10 | RX | through a **1 kΩ resistor** (stops the hiss from 5 V signals) |
| D11 | TX | optional, nothing is read from it |
| D8  | BUSY | optional: lets AMBIENT wait for a clip to finish |
| 5V  | VCC | |
| GND | GND | both GND pins |
|     | SPK_1, SPK_2 | an 8 Ω speaker, up to 3 W |
| D12 | | trigger from another board (plus GND to GND) |

Uses SoftwareSerial, which comes with the Arduino AVR core: no extra
library to install.

## 📤 Upload

From the repo folder (no install needed):

```sh
arduino-cli compile --fqbn arduino:avr:nano --library . examples/sound
arduino-cli upload  --fqbn arduino:avr:nano -p /dev/cu.usbserial-XXXX examples/sound
# clone with old bootloader: --fqbn arduino:avr:nano:cpu=atmega328old
```

Or install the library once and open it from *File → Examples → Model
Railroad Animation*. Arduino IDE: download the repo as a ZIP, then *Sketch →
Include Library → Add .ZIP Library*. arduino-cli:
`arduino-cli config set library.enable_unsafe_install true`, then
`arduino-cli lib install --git-url https://github.com/jmcdannel/model-railroad-arduino-animation-lib.git`.
The example is *sound*.

## 🎛️ Start mode

Pick one line in the sketch's *Start mode* block:

| Start mode | Wiring | Use it for |
|---|---|---|
| `Trigger trigger(WHILE_TRIGGERED, 12);` (BELL / THUNDER) | D12 ← another board's output; GND ↔ GND | plays while / each time D12 goes HIGH |
| `Trigger trigger(POWER_UP);` (AMBIENT) | none | plays from power-up |
| `Trigger trigger(TOGGLE, 12, ACTIVE_LOW);` | D12 ← button to GND | press on, press off |
| `Trigger trigger(WHILE_TRIGGERED, 12, ACTIVE_LOW);` | D12 ← button to GND | a horn while you hold the button |

`ACTIVE_HIGH` (the default) means HIGH = on, which is what a DEJA IO pin
or a PIR sensor sends; an unwired pin reads HIGH, so the board still runs
on its own. `ACTIVE_LOW` is for anything that pulls the pin to GND. See the
top-level README for the DEJA side.

**When it's off:** a looping sound fades out; AMBIENT stops picking new clips; THUNDER just waits for the next pulse.

## 🎛️ Tuning

Set these in the sketch's `tune()` block, e.g. `c.volume = 25;`. Every
setting and its default is in [`src/MRA_Sound.h`](../../src/MRA_Sound.h).

- `mode` — `SOUND_LOOP`, `SOUND_EACH_TRIGGER` or `SOUND_RANDOM`
- `firstTrack`, `lastTrack` — the clips it picks from
- `volume` — 0–30
- `fadeOutMs` — fade when switched off (0 = cut)
- `delayMin/Max`, `distanceDrop` — thunder distance: the wait, and how much quieter the farthest one is
- `gapMin/Max` — time between ambient clips
- `interrupt` — whether a new trigger cuts off a clip still playing (false needs BUSY wired)
- `bootMs` — DFPlayer start-up time; raise it if the first sound goes missing
