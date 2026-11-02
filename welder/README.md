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

**Arduino IDE:** open `welder.ino`, Tools → Board → *Arduino Nano*, pick the
port, Upload. If the upload fails with `stk500_getsync()` errors, set
Tools → Processor → *ATmega328P (Old Bootloader)* — most clone Nanos need it.

**arduino-cli:**

```sh
arduino-cli compile --fqbn arduino:avr:nano welder
arduino-cli upload  --fqbn arduino:avr:nano -p /dev/cu.usbserial-XXXX welder
# clone with old bootloader:
arduino-cli upload  --fqbn arduino:avr:nano:cpu=atmega328old -p /dev/cu.usbserial-XXXX welder
```

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

## 🎛️ External trigger (D12)

Off by default. To let DEJA.js (or a relay contact, or a toggle switch)
control this effect, set `USE_TRIGGER = true` and wire:

| From                      | To        |
|---------------------------|-----------|
| DEJA IO pin (Mega / ESP32 / Pico W) | Nano D12 |
| IO board GND              | Nano GND  |

HIGH runs the effect, matching DEJA's "on". The Nano uses its internal
pull-up, so an unplugged wire reads HIGH and the board keeps running on its
own. For a relay contact or switch to GND, or inverted logic, set
`TRIGGER_ACTIVE_LOW = true`. See the top-level README for the DEJA side.

**When the trigger is off:** no new weld jobs start, a job in progress stops, and the red glow cools down naturally.

## 🎛️ Tuning

Everything lives in the `Tuning` block at the top of the sketch:

- `IDLE_MIN/MAX` — how long the shop sits dark between jobs
- `JOB_MIN/MAX` — how long each weld job lasts
- `BURST_*` / `PAUSE_*` — rhythm of the arc (longer pauses = moving along a seam)
- `WHITE_*` / `BLUE_*` — flicker speed and brightness range of each color
- `HEAT_UP_MS` — how fast the red reaches full glow
- `COOL_TAU_MS` — how slowly the red fades (bigger = longer glow)
- `RED_MAX` — cap the red brightness if it overpowers the scene

Set `DEBUG = true` and open Serial Monitor at 9600 to watch state changes.

## 🙏 Credits

Based on the classic model-railroad arc-welder sketches: Steve Spence's
original, Paul & David Bradt's Model Railroader SN230 version, and the
Cornhill & Atherton finite-state-machine build on Model Railroad Hobbyist.
Rewritten from scratch with PWM intensity, burst structure and the thermal
red glow.
