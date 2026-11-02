# 📺 TV Glow (Arduino Nano)

The flickering blue-white light of a television seen through a window.
Scenes cut every few seconds to a new brightness and color mix, with gentle
flicker inside each scene, occasional bright flashes (explosions,
commercials), near-black moments, and the odd bout of channel surfing.

Runs from power-up. No buttons, no libraries.

## 🔌 Wiring

| Nano pin | Resistor | LED        | Notes                                   |
|----------|----------|------------|-----------------------------------------|
| D9       | 220 Ω    | cool white |                                         |
| D10      | 220 Ω    | blue       |                                         |
| D11      | 220 Ω    | red        | optional, adds warm scenes; leave off if you like |
| GND      |          | all cathodes |                                       |

Pin → resistor → anode, cathode → GND. Point the LEDs at the inside wall of
the room rather than straight out the window so the glow washes the room
the way a real screen does.

## 📤 Upload

```sh
arduino-cli compile --fqbn arduino:avr:nano tv-glow
arduino-cli upload  --fqbn arduino:avr:nano -p /dev/cu.usbserial-XXXX tv-glow
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

**When the trigger is off:** the screen fades to black and starts a fresh scene when the trigger returns.

## 🎛️ Tuning

- `SCENE_MIN/MAX` — how long scenes last
- `WHITE_/BLUE_/RED_ MIN/MAX` — brightness range per color (lower `WHITE_MAX` for a bluer TV)
- `FLICKER_MAX`, `MICRO_MIN/MAX` — how much and how fast it wobbles inside a scene
- `FADE_PERCENT` — share of scene changes that fade rather than cut
- `FLASH_PERCENT`, `DARK_PERCENT`, `SURF_PERCENT` — how often the special moments happen
