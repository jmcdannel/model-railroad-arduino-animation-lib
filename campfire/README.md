# 🔥 Campfire / Fireplace Flicker (Arduino Nano)

Three LEDs flicker like a fire. Two "flame" LEDs (yellow and orange) dance
quickly and independently; a red "ember" LED breathes slowly underneath.
Random gusts flare the fire up, lulls let it settle. Works for a campfire,
hobo barrel, fireplace behind a window, caboose stove, forge, or trash fire.

Runs from power-up. No buttons, no libraries.

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

```sh
arduino-cli compile --fqbn arduino:avr:nano campfire
arduino-cli upload  --fqbn arduino:avr:nano -p /dev/cu.usbserial-XXXX campfire
# clone with old bootloader: --fqbn arduino:avr:nano:cpu=atmega328old
```

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

**When the trigger is off:** the fire fades out over about a second and relights the moment the trigger returns.

## 🎛️ Tuning

- `FLAME_LVL_MIN/MAX`, `FLAME_STEP_MIN/MAX`, `FLAME_EASE` — flame brightness range, how often it changes, how snappy
- `EMBER_*` — same for the red ember (slow and warm by default)
- `EVENT_EVERY_*`, `EVENT_LEN_*` — how often and how long gusts/lulls happen
- `GUST_BOOST`, `LULL_DROP`, `LULL_PERCENT` — how strong they are and the gust/lull mix

Want a calmer fireplace? Raise `FLAME_EASE` to 5–6 and `FLAME_STEP_MIN` to 80.
