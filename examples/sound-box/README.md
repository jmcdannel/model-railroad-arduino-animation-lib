# 🔊 Sound Trigger Box (Arduino Nano or ESP32 + DFPlayer Mini)

One MP3 player, many ways to set it off:

- 🔘 **Fascia buttons**: a horn, a station announcement, yard sounds on and off
- 🔌 **Trigger inputs**: the crossing gates' bell output, a sensor, a relay
  contact
- 🚂 **DEJA**: a `pin` effect on this device works exactly like that pin
  being wired, so DEJA effects, routes and macros can play sounds
- 💬 **The Serial Monitor**: `p3` plays track 3, `l3` loops it, `s` stops

For a single always-on sound tied to one effect (thunder after lightning, a
bell with the gates), the [sound effect](../sound/) is simpler. This box is
for a panel of them.

## 🔘 Triggers

Each row is one trigger:

```cpp
const Trigger TRIGGERS[] = {
  { 2,  TO_GND,    1, PLAY_ONCE },      // pin, wiring, track, mode
  { 3,  TO_GND,    2, PLAY_ONCE },
  { 4,  TO_GND,    3, PLAY_TOGGLE },
  { 12, DEJA_ONLY, 4, PLAY_WHILE_ON },
};
```

| Mode | Does |
|---|---|
| `PLAY_ONCE` | plays the clip once per press (press again to restart it) |
| `PLAY_WHILE_ON` | loops while the input is on, stops when it goes off: a bell while the gates are down |
| `PLAY_TOGGLE` | press to start a loop, press again to stop it: background sounds |

| Wiring | For |
|---|---|
| `TO_GND` | a button, relay contact or IR module that pulls the pin to GND (no resistor needed) |
| `HIGH_IS_ON` | a logic output: a DEJA IO pin, the [crossing gates'](../crossing-gates/) bell out, a PIR sensor |
| `DEJA_ONLY` | nothing wired. Only DEJA or the Serial Monitor sets it. |

The player plays **one sound at a time**, and the newest trigger wins. With
the **BUSY** pin wired, a loop that a one-shot interrupted comes back when
the one-shot ends: the horn sounds over the bell, then the bell carries on.

## 🔌 Wiring (Nano)

| From | To | Notes |
|---|---|---|
| D11 | 1 kΩ → DFPlayer `RX` | the resistor stops 5 V hiss |
| D10 | DFPlayer `BUSY` | optional; needed for loops to come back |
| 5V / GND | DFPlayer `VCC` / `GND` | |
| DFPlayer `SPK_1` / `SPK_2` | 8 Ω speaker, up to 3 W | |
| D2, D3, D4 | buttons → GND | |
| A0 | 10 kΩ pot wiper (ends to 5V and GND) | optional volume knob, `VOLUME_KNOB_PIN A0` |

**ESP32:** GPIO 17 → DFPlayer `RX` (no resistor needed at 3.3 V), BUSY → 16,
DFPlayer `VCC` → 5V (VIN). Set `PLAYER_TX_PIN 17` and `BUSY_PIN 16`.

**SD card:** make a folder named **`mp3`** holding `0001.mp3`, `0002.mp3`,
and so on. The number is the track, and anything after it is ignored, so
`0001_horn.mp3` works.

## 🚂 With DEJA

**Over USB:** set `SERIAL_MODE SERIAL_DEJA`, plug the Nano into the DEJA
server, and add a device with id **`sound-box`** (or your `DEVICE_ID`) on
that port.

**Over WiFi (ESP32):** set `USE_MQTT true` and `LAYOUT_ID`, fill in
`secrets.h` (copy `secrets.example.h`), and add the device in DEJA as an MQTT
device with the same id. It listens on `DEJA/<LAYOUT_ID>/<DEVICE_ID>`.

Then add **`pin` effects** on that device:

- **A pin that's in `TRIGGERS`** acts as if it were wired. Pin 12 "on" rings
  the bell above until it goes "off".
- **Any other pin number** plays that track once. Pin 7 "on" plays
  `0007.mp3`, and "off" stops it if it's still playing.

So a DEJA route or macro can sound a horn, start a bell, or play an
announcement. The sketch answers DEJA's `ping` and sends `connected` and
`alive`, like DEJA's own boards, and ignores servo and turnout commands.

## 💬 Serial Monitor

At 115200 with `SERIAL_TEXT`: `p3` plays track 3, `l3` loops track 3, `s`
stops everything, `v20` sets the volume (0–30), `?` shows help. Each sound
prints a line (`▶️ playing track 1`).

## 📤 Upload

Libraries: **ArduinoJson** (for DEJA's messages), and **PubSubClient** for the
ESP32 MQTT build. SoftwareSerial comes with the Nano's core.

```sh
arduino-cli compile --upload --fqbn arduino:avr:nano -p /dev/cu.usbserial-XXXX examples/sound-box
```

🧪 Every build in [`ci.json`](ci.json) compiles, and the logic was simulated
on a computer with real DEJA frames: a bell loop from DEJA, the horn
interrupting it and the bell coming back, an unlisted pin playing its track,
toggle, volume and stop, all with valid DFPlayer checksums. Not yet tried
with a real DFPlayer.
