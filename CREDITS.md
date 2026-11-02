# 🙏 Credits & Third-Party Code

Most of this collection is original work under the repo's [MIT license](LICENSE).
The pieces below came from, or were shaped by, other people's work.

## 📦 Vendored code (copied into this repo)

### QR code generator — `room-code/qrcode_gen.c`, `room-code/qrcode_gen.h`

- **Source:** [ricmoo/QRCode](https://github.com/ricmoo/QRCode) by Richard Moore
- **Derived from:** [Project Nayuki's QR Code generator library](https://www.nayuki.io/page/qr-code-generator-library)
- **License:** MIT. Copyright (c) 2017 Richard Moore, Copyright (c) 2017 Project Nayuki.
  The full license text is kept at the top of both files.
- **Changes:** renamed from `qrcode.c/.h` to `qrcode_gen.c/.h` (with the
  include guard and `#include` updated to match) because the ESP32 Arduino
  core ships its own `qrcode.h` that shadows the library. No other changes.

## 💡 Inspiration (ideas, not code)

### Arc welder — `welder/`

Based on the classic model-railroad arc-welder sketches:

- Steve Spence's original arc-welder sketch
- Paul & David Bradt's version in *Model Railroader* (SN230)
- The Cornhill & Atherton finite-state-machine build on *Model Railroad Hobbyist*

`welder.ino` is a from-scratch rewrite (PWM intensity, burst structure,
thermal red glow), but the effect owes them its shape.

## 🧩 Libraries used (not included; install separately)

`room-code` builds against these Arduino libraries, each under its own license:

- [PubSubClient](https://github.com/knolleary/pubsubclient) by Nick O'Leary
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson) by Benoît Blanchon
- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library),
  [Adafruit ST7735 and ST7789 Library](https://github.com/adafruit/Adafruit-ST7735-Library),
  [Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306) by Adafruit
- [U8g2_for_Adafruit_GFX](https://github.com/olikraus/U8g2_for_Adafruit_GFX) by Oliver Kraus

## 🚂 Artwork

`room-code/logo.h` is the [DEJA.js](https://github.com/trackandtrestle/DEJA.js)
mark from Track & Trestle, converted to a 24×24 bitmap by `make-logo.mjs`.
