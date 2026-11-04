# 🔀 Turnout Panel (Arduino Nano or ESP32)

Slow-motion servo turnouts with everything a fascia panel needs:

- 🔘 **a push button** per turnout: press to throw, press again to close
- 💡 **two LEDs** per turnout showing the route; the new one blinks while
  the points move
- ⚡ **a frog relay** that flips polarity halfway through the throw, while
  the points are clear of both rails
- 💾 **memory**: every turnout comes back where it was after power-off, and
  doesn't sweep across the rails at boot

And the same turnouts from anywhere else:

- 🚂 **DEJA** over USB, or MQTT on an ESP32: DEJA's own `turnout` and
  `servo` commands, with DEJA's motion profiles (fast, medium, slow,
  bounce). Throws made on the fascia go back to DEJA as sensors.
- 💬 **The Serial Monitor**: `t0` toggles turnout 0, `c0` closes it, `d0`
  diverges it, `p` lists them all

Find each servo's two angles with the [servo tuner](../servo-tuner/) first.
Both sketches use DEJA's pulse mapping (0–180° → 650–2350 µs), so the
numbers carry straight over.

## 🔀 Setting up turnouts

```cpp
const Turnout TURNOUTS[] = {
  // name        servo  closed thrown  button  LED closed  LED thrown  frog relay
  { "Yard lead",  9,    72,    108,    2,      4,          5,          6 },
  { "Siding",    10,    80,    100,    3,      7,          8,          NO_PIN },
};
```

- `servo` is a Nano pin (`DRIVER_PINS`) or a PCA9685 channel (`DRIVER_PCA9685`).
- Use `NO_PIN` for anything a turnout doesn't have.
- The row number (0, 1, 2…) is the turnout's index in DEJA.

A Nano with servos on its own pins handles about 3 turnouts with buttons,
LEDs and frogs. With a **PCA9685** the servos move to the board, which leaves
the Nano's pins for buttons and LEDs.

## 🔌 Wiring

| From | To | Notes |
|---|---|---|
| Servo signal | the `servo` pin, or a PCA9685 channel | |
| Servo + / − | **a separate 5 V supply** + its GND to the Nano's GND | servos brown out the Nano's 5V pin |
| Button | its pin → button → GND | no resistor needed |
| LED | its pin → 1 kΩ → LED → GND | a bicolor LED works on two pins |
| Frog relay | its pin → a relay module's `IN` | **HIGH when thrown**. The relay's changeover contacts feed the frog from one rail or the other. |
| PCA9685 | `SDA` → A4, `SCL` → A5, `VCC` → 5V, servo power → `V+` | ESP32: SDA 21, SCL 22 |

## 🐢 Motion

`BUTTON_PROFILE` sets how buttons and serial commands move the points:

| Profile | Throw |
|---|---|
| `PROFILE_INSTANT` | straight there |
| `PROFILE_FAST` | 0.5 s |
| `PROFILE_MEDIUM` | 1 s |
| `PROFILE_SLOW` | 2 s (default) |
| `PROFILE_BOUNCE` | 1.5 s, overshoots and settles like a ground throw |

These match DEJA's firmware step for step, so a turnout thrown from DEJA
and one thrown from the fascia look the same. DEJA sends its own profile
with each command.

`HOLD_SERVOS = false` stops driving each servo once it arrives. That's
quieter, but stiff points can push the servo back a little.

## 🚂 With DEJA

**Over USB:** set `SERIAL_MODE SERIAL_DEJA`, plug the Nano into the DEJA
server, and add a device with id **`turnouts`** (or your `DEVICE_ID`) on that
port. **Over WiFi (ESP32 + PCA9685):** set `USE_MQTT true` and `LAYOUT_ID`,
fill in `secrets.h`, and add it in DEJA as an MQTT device with the same id.

Add a turnout in DEJA on this device, and give it **index = its row** in
`TURNOUTS`:

- **Servo turnouts** send `servo` with DEJA's own straight and divergent
  angles and profile. The panel moves to DEJA's angle, and its LEDs and frog
  follow whichever end that angle is nearer.
- **Other turnout types** send `turnout` with `state` true or false. The
  panel uses its own angles and `BUTTON_PROFILE`.

When a turnout finishes moving (from the fascia, serial or DEJA), the panel
sends `{ "sensor": <row>, "state": 1 }` for thrown or `0` for closed. Add
sensors with those indexes in DEJA if you want fascia throws to show up
there. DEJA doesn't update the turnout itself from a sensor, so link them
with an automation if you need to. The sketch answers `ping` and sends
`connected` and `alive` like DEJA's own boards.

## 📤 Upload

Libraries: **ArduinoJson**, **Adafruit PWM Servo Driver Library** for a
PCA9685, and **PubSubClient** for MQTT. Servo and EEPROM come with the Nano's
core.

```sh
arduino-cli compile --upload --fqbn arduino:avr:nano -p /dev/cu.usbserial-XXXX examples/turnout-panel
```

🧪 Every build in [`ci.json`](ci.json) compiles, and it was simulated on a
computer: a slow throw from the button with the frog flipping at the
halfway point and the LED blinking, DEJA `turnout` and `servo` (bounce)
commands, a command for another device ignored, and power-off memory with
no sweep at boot. Not yet tried with real servos.
