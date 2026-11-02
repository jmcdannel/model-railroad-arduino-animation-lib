# 🧩 Multi-Effect: several animations on one Nano

Pick the effects you want in [`config.h`](config.h), give each its own pins
and its own trigger input, and they all run side by side. Each effect can
start at power-up, follow a DEJA.js IO pin or sensor, or toggle from a
button, independently of the others.

The default `config.h` runs a **welder** (D9–D11, on while A0 is HIGH), a
**campfire** (D3, D5, D6, button on A1 toggles it) and a **traffic light**
(D2, D4, D7, D8, D12, D13; night-flash when A2 goes LOW).

## 🔗 Effects that drive each other

On one board, effects can be linked without any wiring:

| Setting in `config.h` | Result |
|---|---|
| `SOUND_SOURCE SOURCE_LIGHTNING` | thunder plays after each lightning strike |
| `SOUND_SOURCE SOURCE_GATES_BELL` | the bell rings with the crossing gates |
| `SOUND_SOURCE SOURCE_CROSSING` | the bell rings while the crossing flashes |
| `NEON_SOURCE SOURCE_NIGHT` | the neon sign lights at dusk (needs day-night) |

## 📌 Pin budget

- Fading needs PWM pins: **3, 5, 6, 9, 10, 11**. Six in all, and the welder,
  campfire, TV glow and day-night use three each.
- Crossing gates use the Servo library, which takes PWM away from 9 and 10.
- Plain on/off pins: 2, 4, 7, 8, 12, 13 and A0–A5.
- A6/A7 can't be digital pins. D0/D1 are the USB serial port.

After uploading, open Serial Monitor at **115200**. The sketch checks the
pin plan and warns about pins used twice, fades on non-PWM pins, and
servo conflicts.

## 📏 How much fits

| Build | Flash | RAM |
|---|---|---|
| Default (welder + campfire + traffic light) | 9.5 KB (30%) | 534 B (26%) |
| All twelve effects at once | 29.9 KB (97%) | 1,620 B (79%) |

## 📤 Upload

From the repo folder (no install needed):

```sh
arduino-cli compile --fqbn arduino:avr:nano --library . examples/multi-effect
arduino-cli upload  --fqbn arduino:avr:nano -p /dev/cu.usbserial-XXXX examples/multi-effect
# clone with old bootloader: --fqbn arduino:avr:nano:cpu=atmega328old
```

Or install the library once and open it from *File → Examples → Model
Railroad Animation*. Arduino IDE: download the repo as a ZIP, then *Sketch →
Include Library → Add .ZIP Library*. arduino-cli:
`arduino-cli config set library.enable_unsafe_install true`, then
`arduino-cli lib install --git-url https://github.com/jmcdannel/model-railroad-arduino-animation-lib.git`.
The example is *multi-effect*.

Turning on crossing gates? Run `arduino-cli lib install Servo` first.
