# ⛈️ Lightning Storm (Arduino Nano)

White LEDs strobe like distant lightning: clusters of 1–6 flashes with random
brightness, some stuttering, a fast afterglow after each flash, then nothing
for a while. The storm drifts between active spells (a strike every few
seconds) and quiet spells (one every half minute or so).

Runs from power-up. No buttons, no libraries.

## 🔌 Wiring

| Nano pin | Resistor  | Connects to      | Notes                                      |
|----------|-----------|------------------|--------------------------------------------|
| D9       | 150–220 Ω | white LED        | main flash                                 |
| D10      | 150–220 Ω | white LED        | optional second flash (`USE_FLASH_B`)      |
| D2       |           | thunder trigger  | optional: HIGH for 100 ms at every strike  |
| GND      |           | all cathodes     |                                            |

Hide the LEDs behind the backdrop or above a cloud flat, pointing at the sky,
so you see the flash on the backdrop rather than the LED itself. Two LEDs a
foot or two apart give the storm some width.

**Thunder later:** D2 pulses at the start of every strike. Feed it to a sound
module (DFPlayer Mini through a small transistor, or another Arduino's input)
and the thunder follows the lightning. Set `USE_THUNDER = false` if unused.

## 📤 Upload

```sh
arduino-cli compile --fqbn arduino:avr:nano lightning
arduino-cli upload  --fqbn arduino:avr:nano -p /dev/cu.usbserial-XXXX lightning
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

**When the trigger is off:** no strikes (one in progress is cut). The first strike comes about 2 s after the trigger returns.

## 🎛️ Tuning

- `ACTIVE_SPELL_*`, `QUIET_SPELL_*` — how long the storm stays busy or quiet
- `GAP_ACTIVE_*`, `GAP_QUIET_*` — time between strikes in each spell
- `FLASHES_MIN/MAX`, `FLASH_ON_*`, `FLASH_GAP_*` — shape of each strike
- `LVL_MIN/MAX` — flash brightness range
- `STUTTER_PERCENT` — share of flashes that stutter
- `B_PERCENT_MIN/MAX` — how much the second LED joins in
