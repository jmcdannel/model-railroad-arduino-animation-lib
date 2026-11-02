# 🚥 Traffic Light (Arduino Nano)

One intersection with two phase groups. Group A is north/south, group B is
east/west. Wire one head on group A for a single traffic light. Wire two
heads on each group for a full four-way intersection, and all four stay in
sync automatically.

Cycle: A green → A yellow → all red → B green → B yellow → all red → repeat.
Powers up in all-red. Set `NIGHT_FLASH = true` for late-night mode (A flashes
yellow, B flashes red).

## 🔌 Wiring

| Nano pin | Group | LED    |
|----------|-------|--------|
| D2       | A     | red    |
| D3       | A     | yellow |
| D4       | A     | green  |
| D5       | B     | red    |
| D6       | B     | yellow |
| D7       | B     | green  |
| GND      |       | all cathodes |

Pin → resistor → anode, cathode → GND. Single head per group: 220 Ω. Two
heads per group (the four-way setup): wire the matching LEDs in parallel,
**each with its own 470 Ω resistor**, so each pin stays well under its 40 mA
limit. These are plain on/off pins, no PWM needed.

```
 four-way:   N head ─┐              E head ─┐
             S head ─┴─ group A     W head ─┴─ group B
```

## 📤 Upload

```sh
arduino-cli compile --fqbn arduino:avr:nano traffic-light
arduino-cli upload  --fqbn arduino:avr:nano -p /dev/cu.usbserial-XXXX traffic-light
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

**When the trigger is off:** the intersection drops to night-flash (A yellow, B red). Set `OFF_NIGHT_FLASH = false` for dark instead. Trigger on brings it back through all-red into the normal cycle.

## 🎛️ Tuning

- `GREEN_A_MS`, `GREEN_B_MS` — green time per direction (main road gets more)
- `YELLOW_MS`, `ALL_RED_MS` — yellow and the all-red clearance between phases
- `NIGHT_FLASH`, `FLASH_MS` — late-night flashing mode
