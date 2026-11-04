# 🚄 Scale Speedometer (Arduino Nano or ESP32)

Two IR sensors a measured distance apart time a train between them and turn
that into **scale speed** in mph or km/h. It works in either direction and
ignores the gaps between cars. You can show the speed on a display, print it
to the Serial Monitor, hand it to DEJA, or publish it over MQTT, in any
combination.

Every setting sits at the top of [`speedometer.ino`](speedometer.ino), in
five numbered blocks. The rest is plain code you're welcome to change.

## 🧺 Parts

- Arduino Nano (or any ESP32 board, for MQTT)
- 2 × IR obstacle sensor modules (FC-51, or TCRT5000 boards with a digital
  `D0` out). Any sensor that pulls a pin low when a train is over it works,
  including break-beam sensors and reed switches.
- Optional display, either:
  - a **TM1637** 4-digit LED module (shows `32.1`), or
  - a **0.96" SSD1306 OLED**, I2C (shows direction, scale, and big digits)

## 🔌 Wiring (Nano)

| Nano pin | Connects to | Notes |
|---|---|---|
| D2 | IR sensor A `OUT` | sensor `VCC` → 5V, `GND` → GND |
| D3 | IR sensor B `OUT` | D2 and D3 are interrupt pins: the timing is exact even while the display updates |
| D4 | TM1637 `CLK` | TM1637 only; `VCC` → 5V |
| D5 | TM1637 `DIO` | |
| A4 | OLED `SDA` | OLED only; `VCC` → 5V |
| A5 | OLED `SCL` | |

**ESP32:** use input pins with pull-ups for the sensors (32 and 33 work; 34 to
39 have no pull-ups), SDA 21 / SCL 22 for the OLED, and power the sensors
from **3V3**.

**Mounting the sensors:** point them up between the ties, and turn each pot
until it triggers on a car's underside at 2 to 3 cm but not on the ballast.
Then measure **centre to centre** between them, as exactly as you can, and put
it in `SENSOR_GAP_MM`. On a 300 mm gap, 1 mm out is 0.3% out on the speed.
Longer gaps are more accurate. Anything from 150 mm to a metre is fine.

## 🎛️ Settings

| Block | Setting | Default |
|---|---|---|
| 1. Sensors | `SENSOR_A_PIN`, `SENSOR_B_PIN`, `SENSOR_ACTIVE_LOW` | 2, 3, true |
| | `SENSOR_GAP_MM` | 300 |
| | `GIVE_UP_MS`: the second sensor never tripped | 15 s |
| | `REARM_MS`: both sensors clear before the next train | 2 s |
| 2. Scale | `SCALE`, `SCALE_NAME`, `USE_MPH` | 87.1, `"HO"`, true |
| 3. Display | `DISPLAY_TYPE`: `DISPLAY_NONE`, `DISPLAY_TM1637`, `DISPLAY_OLED` | none |
| 4. Serial | `SERIAL_MODE`: `SERIAL_TEXT`, `SERIAL_JSON`, `SERIAL_DEJA` | text |
| 5. MQTT | `USE_MQTT`, `MQTT_TOPIC` (ESP32 only) | off |

Common scales: Z 220 · N 160 (UK 148, Japan 150) · TT 120 · HO 87.1 ·
OO 76.2 · S 64 · O 48 (UK 43.5) · G 22.5.

## 💬 Ways to use it

**Serial Monitor (`SERIAL_TEXT`)**, 115200 baud:

```
🚄 Speedometer ready: 300 mm gap, HO (1:87.1)
🚄 A->B 1.74 s, 33.6 mph (HO)
```

**Scripts (`SERIAL_JSON`)**: one line per train, ready for a logger,
spreadsheet or speed-matching script:

```json
{"speed":33.6,"units":"mph","dir":"AB","ms":1740.0,"scale":87.1}
```

**DEJA (`SERIAL_DEJA`)**: plug the Nano into the DEJA server's USB. Both
sensors report as **DEJA sensors 0 and 1** on device `speedometer` (change
`DEJA_DEVICE_ID`), so blocks, triggers and effects in DEJA can use them, and
each speed shows up in the server log as `[speed] A->B 1.74 s, 33.6 mph (HO)`.
Add a device with that id in DEJA first (any USB Arduino type), then two
sensors on it with index 0 and 1. The sketch answers DEJA's `ping` and sends
the same `connected` and `alive` messages as DEJA's own boards.

**MQTT (`USE_MQTT`, ESP32)**: every reading is published as the JSON above
to `MQTT_TOPIC`. Point it at the DEJA server's broker (port 1883), or at Home
Assistant, Node-RED or anything else. Copy `secrets.example.h` to `secrets.h`
and fill in WiFi and the broker. `secrets.h` is git-ignored.

## 🚂 How it measures

1. Ready: waiting for either sensor.
2. The first sensor a train reaches starts the clock (stamped in an
   interrupt, to the microsecond).
3. The other sensor stops it: speed = gap ÷ time × scale.
4. Then it waits until **both** sensors have been clear for `REARM_MS`, so
   the rest of the train (and the gaps between cars) can't start a false
   second reading.

If only one sensor trips (the train stopped or backed out), it gives up
after `GIVE_UP_MS` and waits for the track to clear.

## 📤 Upload

Libraries (*Tools → Manage Libraries*, or `arduino-cli lib install`), only
for what you switch on: **TM1637** (Avishay Orpaz), **SSD1306Ascii** (Bill
Greiman), **PubSubClient** (MQTT).

```sh
arduino-cli compile --fqbn arduino:avr:nano examples/speedometer
arduino-cli upload  --fqbn arduino:avr:nano -p /dev/cu.usbserial-XXXX examples/speedometer
# ESP32 DevKit
arduino-cli compile --upload --fqbn esp32:esp32:esp32 -p /dev/cu.usbserial-XXXX examples/speedometer
```

🧪 Compiled for both boards (every variant in [`ci.json`](ci.json)) and
simulated on a computer: both directions, gaps between cars, give-up and
re-arm, and the DEJA messages. Not yet tried with real sensors.
