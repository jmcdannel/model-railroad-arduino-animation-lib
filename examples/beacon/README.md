# 🚨 Rotating Beacon (Arduino Nano)

A lamp with a turning reflector, faked with brightness: as the beam sweeps
past, the LED swells to a sharp peak and falls back to a faint glow, just
like a rotating light seen from the side. Rotating types spin up when
switched on and coast down when switched off.

| Type | Looks like |
|---|---|
| `BEACON_ROTATING` (default) | amber on a tow truck, plant roof or snow plow; red on a fire station. `reflectors = 2` for a double beam |
| `BEACON_AIRPORT` | white and green on opposite sides, turning slowly (12–15 rpm) |
| `BEACON_TOWER` | red obstruction light on a radio or water tower, fading on and off ~30 times a minute |

## 🔌 Wiring

| Nano pin | Resistor | LED | Role |
|----------|----------|-----|------|
| D9  | 220 Ω | amber / red / white | lamp A |
| D10 | 220 Ω | green (airport) or red (tower's second tier) | lamp B, optional |
| GND |       | all cathodes | |

Both must be PWM pins (3, 5, 6, 9, 10, 11): the sweep is a fade. Pass
`NO_PIN` for lamp B if you don't use it.

## 📤 Upload

From the repo folder (no install needed):

```sh
arduino-cli compile --fqbn arduino:avr:nano --library . examples/beacon
arduino-cli upload  --fqbn arduino:avr:nano -p /dev/cu.usbserial-XXXX examples/beacon
# clone with old bootloader: --fqbn arduino:avr:nano:cpu=atmega328old
```

Or install the library once and open it from *File → Examples → Model
Railroad Animation*. Arduino IDE: download the repo as a ZIP, then *Sketch →
Include Library → Add .ZIP Library*. arduino-cli:
`arduino-cli config set library.enable_unsafe_install true`, then
`arduino-cli lib install --git-url https://github.com/jmcdannel/model-railroad-arduino-animation-lib.git`.
The example is *beacon*.

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

**When it's off:** the lamp fades out while the motor coasts to a stop.

## 🎛️ Tuning

Set these in the sketch's `tune()` block, e.g. `c.type = BEACON_AIRPORT;`. Every
setting and its default is in [`src/MRA_Beacon.h`](../../src/MRA_Beacon.h).

- `type` — rotating, airport or tower
- `rpm` — turning speed (rotating ~45–90, airport 12–15)
- `reflectors` — 1 or 2 beams (rotating)
- `sharpness` — bigger = narrower, snappier beam
- `level`, `ambient` — peak brightness and the glow when the beam points away
- `spinUpMs`, `spinDownMs` — motor start and coast
- `towerFlashesPerMin`, `towerOnPercent` — tower rhythm
