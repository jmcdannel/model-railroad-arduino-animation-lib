# 🪧 Room Code Sign (ESP32-C6 + 1.47" LCD, or any ESP32 + 0.96" OLED)

A little always-on sign for the wall by the layout. Its home page shows the
DEJA room code visitors type to join as a guest, next to a QR of the guest
join link, with a bar that drains as the code counts down to its next
5-minute rotation. Put it next to the "view as guest" poster.

The **BOOT button** pages through four screens; two minutes after the last
press it drifts back to the code, so the wall default is always the code.
Page dots sit top right; a **red dot** before them on every page means every
known track output is off.

The header wears the dejajs.com livery: the DEJA mark and wordmark, the layout
name, the page dots, and the cyan / magenta / lime stripes under it (the site's
colours — `C_CYAN`, `C_MAGENTA`, `C_LIME` at the top of the sketch). `logo.h` is
the mark as a 24×24 RGB565 bitmap, generated from the website's icon by
`make-logo.mjs` (see the comment in it).

| Page | Shows |
|------|-------|
| 1 · Join | QR of the guest join link · the 3-digit room code · countdown bar |
| 2 · Now running | up to four moving locos, fastest first, with roster names and speed % |
| 3 · Track power | one row per track output on each EX-CommandStation: mode + ON / OFF pill |
| 4 · Server | version, uptime, serial + cloud up/down, awake / standby, drivers, broker clients, the sign's WiFi + IP |

Two builds of the same sketch:

- **Waveshare / SpotPear ESP32-C6-LCD-1.47** (172×320 ST7789, onboard RGB LED,
  BOOT button, USB-C). Nothing to wire — the screen is on the board. The default build.
- **Any ESP32 + 0.96" SSD1306 128×64 I2C OLED** (`-DDISPLAY_SSD1306`), e.g. an
  ESP32-DevKitC V4 on a screw-terminal breakout. Too small for a QR, so it
  shows the code page only (no button).

  | OLED pin | ESP32 |
  |----------|-------|
  | GND | GND |
  | VCC | 3V3 |
  | SCL | GPIO 22 (P22) |
  | SDA | GPIO 21 (P21) |

  Pins, I2C address and the status LED pin are at the bottom of `config.h`.
  The sketch looks for the OLED every 5 s, so you can wire it with the board running.

It is **not** a DEJA device. The DEJA server, started with
`ROOM_SIGN_MQTT=true`, publishes two topics on its MQTT broker and the sign
subscribes to both:

| Topic | Payload | When |
|-------|---------|------|
| `DEJA/<layoutId>/roomCode` | `{"code":"123","secondsRemaining":287,"windowSeconds":300}` | every 30 s |
| `DEJA/<layoutId>/roomSign` | `{"join":…,"running":[…],"tracks":[…],"server":{…}}` | on change (checked every second) and every 30 s |

Only the *current* code ever reaches the board — no secret, no clock. If the
server goes quiet (90 s without a message) the digits come down and the pages
say "Server offline", so nothing stale is ever on the wall.

## 🛜 Setup

```sh
cp config.example.h config.h   # then fill in WiFi, the server's LAN IP, layout id
```

On the server machine set `ROOM_SIGN_MQTT=true` where that server reads its
env — `~/.deja/.env` for a `deja start` install, **`apps/server/.env`** for a
source checkout run with `pnpm run start` — and restart it. The server logs
`[ROOM SIGN] Publishing the room code on DEJA/tam/roomCode and the sign feed on DEJA/tam/roomSign`.

## 📤 Build & flash

Libraries (`arduino-cli lib install …`): PubSubClient, ArduinoJson,
Adafruit GFX Library, Adafruit ST7735 and ST7789 Library, U8g2_for_Adafruit_GFX.
Core: esp32 ≥ 3.0 (the C6 needs it). The QR generator ([ricmoo/QRCode](https://github.com/ricmoo/QRCode),
MIT) is vendored as `qrcode_gen.c/.h` because the ESP32 core ships its own
`qrcode.h` that shadows the library.

```sh
# ESP32-C6-LCD-1.47 (default build)
FQBN=esp32:esp32:esp32c6:CDCOnBoot=cdc,FlashSize=8M,PartitionScheme=default_8MB
arduino-cli compile --fqbn $FQBN room-code
arduino-cli upload  --fqbn $FQBN -p /dev/cu.usbmodem2101 room-code
arduino-cli monitor -p /dev/cu.usbmodem2101 -c baudrate=115200

# ESP32 DevKitC + SSD1306 OLED
FQBN=esp32:esp32:esp32:UploadSpeed=460800
FLAGS="--build-property compiler.cpp.extra_flags=-DDISPLAY_SSD1306"
# compile + upload in one go: `arduino-cli upload` can't take build properties
arduino-cli compile --upload -p /dev/cu.usbserial-0001 --fqbn $FQBN $FLAGS room-code
arduino-cli monitor -p /dev/cu.usbserial-0001 -c baudrate=115200
```

Libraries for the OLED build: add `Adafruit SSD1306`.

The serial log is one line per state or page change (`[state]`, `[page]`,
`[feed]`, `[qr]`) plus a `[status]` heartbeat every 10 s.
On the bench, typing `n` in the serial monitor pages like the button does.

## 💡 Status LED

| C6 RGB LED | DevKitC blue LED | Meaning |
|------------|------------------|---------|
| blue blink | fast blink | joining WiFi |
| amber blink | slow blink | looking for the DEJA server |
| amber | on | connected, no code yet |
| dim green | off | code live |
| amber | on | code just rotated, next one arriving |
| red | double blink | server offline (no message for 90 s, or the code ran out) |

## 🎛️ Tuning

- `ROTATION` — 1 or 3 for landscape (which side the USB-C is on), 0/2 portrait
- `INVERT_COLORS` — flip if the panel looks like a negative
- `BACKLIGHT`, `LED_BRIGHTNESS` — 0–255; set the LED to 0 to silence it
- `OFFLINE_AFTER_MS` — how long without a message before "Server offline"
- `PAGE_HOME_AFTER_MS` — idle time before the sign returns to the Join page
- Colours at the top of the sketch (RGB565): the ground and the three livery colours, red for alarms
