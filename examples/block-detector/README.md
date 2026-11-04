# 🚦 Block Occupancy Detector (Arduino Nano or ESP32)

Watches your blocks and reports which ones have a train in them: on fascia
LEDs, in the Serial Monitor, or straight into **DEJA as sensors**, which
DEJA blocks, signals and triggers can use. Each block can use whichever
sensor suits that spot.

| Sensor (`type`) | What it is | Good for |
|---|---|---|
| `DETECT_DIGITAL` | any pin that goes LOW with a train: IR obstacle module (FC-51 / TCRT5000), reed switch + magnet under a car, the output of a commercial DCC detector | spot detection, or a whole block with a ready-made detector |
| `DETECT_CURRENT` | an **ACS712** module (or a current transformer) in the block's feeder wire, on an analog pin | whole-block detection of anything drawing power |
| `DETECT_LIGHT` | an **LDR** between the ties, read on an analog pin | cheap spot detection: a train's shadow darkens it |

## 🧱 Setting up blocks

Each row in `BLOCKS` is one block. Its row number (0, 1, 2…) is its DEJA
sensor index:

```cpp
const Block BLOCKS[] = {
  { "Main east",  2,  DETECT_DIGITAL, 8 },   // name, sensor pin, type, fascia LED pin
  { "Main west",  3,  DETECT_DIGITAL, 9 },
  { "Siding",     A0, DETECT_CURRENT, 10 },
  { "Yard lead",  A1, DETECT_LIGHT,   11 },  // NO_PIN = no LED
};
```

A Nano has 8 analog pins (A0 to A7, but A6 and A7 are analog only) and
about a dozen digital ones, so about a dozen blocks with LEDs. Two timings
smooth things out:

- `OCCUPIED_AFTER_MS` (100 ms): how long the sensor must see a train first.
  This ignores glitches.
- `CLEAR_AFTER_MS` (3 s): how long it must see nothing before the block
  clears. This bridges dirty track, the gaps between cars, and spot sensors
  that only see the train between its wheels.

## 🔌 Wiring

| Sensor | Wiring |
|---|---|
| IR module | `VCC` → 5V, `GND` → GND, `OUT` → the pin. Point it up between the ties and turn the pot until it sees a car's underside but not the ballast. |
| Reed switch | one leg → the pin, other leg → GND (the internal pull-up does the rest) |
| Detector board | its output → the pin, grounds joined. Set `DIGITAL_ACTIVE_LOW false` if it goes HIGH when occupied. |
| ACS712 (5 A) | **in series with one feeder wire** to the block: `IP+` and `IP−` in the wire, `VCC` → 5V, `GND` → GND, `OUT` → an analog pin |
| LDR | LDR from the analog pin to GND, and a **10 kΩ** resistor from the pin to 5V |
| Fascia LED | the pin → 1 kΩ → LED → GND |

**About current sensing:** an ACS712 easily sees a loco with its motor or
lights on. It **won't** see resistor wheelsets on cars (a few mA), and a
stopped DCC loco with its lights off may sit near the threshold. For
detecting cars, use a current transformer with many turns, or a commercial
detector on a `DETECT_DIGITAL` pin. Plain DC layouts work too: the sketch
counts the shift from the sensor's zero point, as well as the DCC swing.

**ESP32:** the ADC is 12-bit (readings to 4095), so raise the thresholds
about 4×. Use ADC1 pins (32 to 39) for analog sensors, because ADC2 stops
working while WiFi is on. Give the ACS712's 5 V output a divider (10 kΩ /
20 kΩ) so it stays within 3.3 V.

## 🎚️ Setting the thresholds

Set `SERIAL_MODE SERIAL_TEXT`, open the Serial Monitor at 115200 and type
**`c`** then Enter. It prints every block's live reading twice a second:

```
0  Main east  clear
2  Siding  clear  level 4
3  Yard lead  clear  level 312  room 309
```

- **Current:** note the `level` with the block empty, then with a loco
  sitting in it. Set `CURRENT_THRESHOLD` about halfway between.
- **Light:** `room` follows the room light slowly, and a shadow must be
  `LIGHT_MARGIN` darker than that. Lower the margin if trains aren't seen.
  Raise it if people walking past trip it.

Type `s` for a one-off status, `c` again to stop.

## 💬 Outputs

| `SERIAL_MODE` | You get |
|---|---|
| `SERIAL_TEXT` | `🟥 Main east occupied` / `🟩 Main east clear` |
| `SERIAL_JSON` | `{"block":0,"name":"Main east","occupied":true}` |
| `SERIAL_DEJA` | DEJA USB sensor messages (see below) |

### 🔌 DEJA over USB

Set `SERIAL_MODE SERIAL_DEJA` and plug the Nano into the DEJA server.

1. In DEJA, add a device with id **`blocks`** (or whatever `DEVICE_ID` you
   set), on its USB port.
2. Add one **sensor** per block on that device, with index 0, 1, 2… in the
   same order as `BLOCKS`.
3. Use those sensors in DEJA **blocks** (a block can list several sensors),
   effects and triggers.

The sketch sends DEJA's own messages: `{ "sensor": 0, "state": 1 }` on each
change, `connected` at start-up, `alive` every minute, and `pong` to the
server's `ping`.

### 📡 DEJA over WiFi (ESP32)

Set `USE_MQTT true` (and fill in `secrets.h` from `secrets.example.h`). The
board becomes a DEJA MQTT device. It publishes
`{"device":"blocks","sensor":0,"state":1}` to
`DEJA/<LAYOUT_ID>/<DEVICE_ID>/messages`, says `connected` when it joins,
re-sends every block so DEJA catches up, and leaves an `offline` last will.
Add it in DEJA as an MQTT device with the same id.

## 📤 Upload

No extra libraries for the Nano. The ESP32 MQTT build needs
**PubSubClient**.

```sh
arduino-cli compile --upload --fqbn arduino:avr:nano -p /dev/cu.usbserial-XXXX examples/block-detector
```

🧪 Every build in [`ci.json`](ci.json) compiles, and the logic was simulated
on a computer: wheel flicker held as one occupancy, a 30 ms glitch ignored,
the light sensor following the room and catching a shadow, and current
detection. Not yet tried with real sensors.
