# 🪧 DEJA Room Sign + Status Monitor (ESP32)

A little always-on sign for the wall by the layout. Its home page shows the
**DEJA room code** that guests and members type to join, next to a QR of the
guest join link, with a bar that drains as the code counts down to its next
5-minute rotation. A button pages through **layout status**: the locos
running, track power, and the DEJA server's health.

Set `SHOW_ROOM_CODE false` and it's a **status monitor** instead: the same
status pages, with no code and no QR. Good for the crew lounge, or for a
layout that doesn't take guests.

| Page | Shows |
|------|-------|
| 1 · Join | QR of the guest join link · the 3-digit room code · countdown bar |
| 2 · Now running | up to four moving locos, fastest first, with roster names and speed % |
| 3 · Track power | one row per track output on each EX-CommandStation: mode + ON / OFF pill |
| 4 · Server | version, uptime, serial + cloud up/down, awake / standby, drivers, broker clients, the sign's WiFi + IP |

Two minutes after the last press it goes back to the first page, so the wall
default is always the code (or now-running, as a status monitor). A **red
dot** in the header on every page means every known track output is off.

## 🖥️ Two boards

- **Waveshare / SpotPear ESP32-C6-LCD-1.47** (172×320 colour screen, RGB
  LED, BOOT button, USB-C): nothing to wire, the screen is on the board.
  The BOOT button pages. `BOARD BOARD_C6_LCD`, the default.
- **Any ESP32 + 0.96" SSD1306 128×64 I2C OLED** (`BOARD BOARD_OLED`), e.g.
  an ESP32-DevKitC. Too small for a QR or pages, so it shows the room code,
  or as a status monitor one screen: track power, how many locos are running
  and how many drivers, and the server version and uptime.

  | OLED pin | ESP32 |
  |----------|-------|
  | GND | GND |
  | VCC | 3V3 |
  | SCL | GPIO 22 |
  | SDA | GPIO 21 |

  The sketch looks for the OLED every 5 s, so you can wire it with the board
  running.

## 📡 How it gets the code

It is **not** a DEJA device, so there's nothing to register in DEJA. The DEJA
server (**v1.23 or later**), started with `ROOM_SIGN_MQTT=true`, publishes
two topics on its built-in MQTT broker, and the sign subscribes to them:

| Topic | Payload | When |
|-------|---------|------|
| `DEJA/<layoutId>/roomCode` | `{"code":"123","secondsRemaining":287,"windowSeconds":300}` | every 30 s |
| `DEJA/<layoutId>/roomSign` | `{"join":…,"running":[…],"tracks":[…],"server":{…}}` | on change and every 30 s |

Only the *current* code ever reaches the board: no secret and no clock. If
the server goes quiet (90 s without a message), the digits come down and the
pages say "Server offline", so nothing stale is ever on the wall.

These are ordinary MQTT topics, so anything else on your network (Home
Assistant, Node-RED, your own sketch) can read them too.

## 🛜 Setup

1. On the DEJA server machine, set `ROOM_SIGN_MQTT=true` where the server
   reads its environment (`~/.deja/.env` for a `deja start` install,
   `apps/server/.env` for a source checkout) and restart it. The log says
   `[ROOM SIGN] Publishing the room code on DEJA/<id>/roomCode …`.
2. At the top of `deja-room-sign.ino`, set `LAYOUT_ID` (the layout's id in
   DEJA) and `LAYOUT_NAME`, and pick `BOARD` and `SHOW_ROOM_CODE`.
3. Copy `secrets.example.h` to `secrets.h` and fill in your WiFi and the
   server's LAN IP. `secrets.h` is git-ignored.

## 📤 Build & flash

Core: **esp32 3.0 or later** (the C6 needs it). Libraries: **PubSubClient**,
**ArduinoJson**, **Adafruit GFX Library**, **Adafruit ST7735 and ST7789
Library**, **U8g2_for_Adafruit_GFX**, plus **Adafruit SSD1306** for the OLED
board. The QR generator ([ricmoo/QRCode](https://github.com/ricmoo/QRCode),
MIT) is included as `qrcode_gen.c/.h`, because the ESP32 core ships its own
`qrcode.h` that hides the library's.

```sh
# ESP32-C6-LCD-1.47
FQBN=esp32:esp32:esp32c6:CDCOnBoot=cdc,FlashSize=8M,PartitionScheme=default_8MB
arduino-cli compile --upload --fqbn $FQBN -p /dev/cu.usbmodem2101 examples/deja-room-sign

# ESP32 DevKitC + OLED (after setting BOARD BOARD_OLED)
arduino-cli compile --upload --fqbn esp32:esp32:esp32 -p /dev/cu.usbserial-0001 examples/deja-room-sign

arduino-cli monitor -p <port> -c baudrate=115200
```

The serial log has one line per state or page change (`[state]`, `[page]`,
`[feed]`, `[qr]`), plus a `[status]` heartbeat every 10 s. On the bench,
typing `n` in the serial monitor pages like the button does.

## 💡 Status LED

| C6 RGB LED | DevKitC blue LED | Meaning |
|------------|------------------|---------|
| blue blink | fast blink | joining WiFi |
| amber blink | slow blink | looking for the DEJA server |
| amber | on | connected, no code yet |
| dim green | off | code (or status feed) live |
| amber | on | code just rotated, next one arriving |
| red | double blink | server offline (no message for 90 s, or the code ran out) |

## 🎛️ Tuning

Near the top of the C6 section of the sketch:

- `ROTATION`: 1 or 3 for landscape (which side the USB-C is on), 0 or 2 for portrait
- `INVERT_COLORS`: flip it if the panel looks like a negative
- `BACKLIGHT`, `LED_BRIGHTNESS`: 0–255. Set the LED to 0 to turn it off.
- `OFFLINE_AFTER_MS`: how long without a message before "Server offline"
- `PAGE_HOME_AFTER_MS`: idle time before the sign returns to the first page
- Colours (RGB565): the ground, the dejajs.com livery (`C_CYAN`,
  `C_MAGENTA`, `C_LIME`), and red for alarms

`logo.h` is the DEJA mark as a 24×24 RGB565 bitmap, made from the website's
icon by `make-logo.mjs` (see the comment in it).

✅ Ported from the C6 room-code sign that runs on the wall at Tamarack
Junction. The drawing and MQTT code are unchanged. This version adds the
settings block, `secrets.h`, and the status-monitor mode. Every build in
[`ci.json`](ci.json) compiles; the status-monitor mode and the OLED status
screen are 🧪 not yet tried on a board.
