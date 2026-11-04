# 🙏 Credits & Third-Party Code

Most of this collection is original work under the repo's [MIT license](LICENSE).
The pieces below were shaped by other people's work.

## 💡 Inspiration (ideas, not code)

### Arc welder — `src/MRA_Welder.h`, `examples/welder/`

Based on the classic model-railroad arc-welder sketches:

- Steve Spence's original arc-welder sketch
- Paul & David Bradt's version in *Model Railroader* (SN230)
- The Cornhill & Atherton finite-state-machine build on *Model Railroad Hobbyist*

The welder is a from-scratch rewrite (PWM intensity, burst structure,
thermal red glow), but the effect owes them its shape.

## 📚 References

- **DFPlayer Mini** serial protocol (`src/MRA_Sound.h`): the 10-byte command
  frame and command numbers come from DFRobot's DFPlayer Mini manual. The
  code is our own; DFRobot's library isn't used.
- **Prototype behavior** (crossing flash rate, gate timing, airport and
  obstruction beacon rates) follows common North American practice; the
  sketches note the ranges they use.

## 📦 Included third-party code

- **QR code generator** (`examples/deja-room-sign/qrcode_gen.c`, `.h`):
  [ricmoo/QRCode](https://github.com/ricmoo/QRCode), MIT, © 2017 Richard
  Moore and Project Nayuki. The license is kept in the file header. It's
  copied in because the ESP32 core's own `qrcode.h` hides the library's.
- **DEJA.js mark** (`examples/deja-room-sign/logo.h`): the DEJA.js icon, by
  the same author, as a 24×24 bitmap.

## 🧩 Libraries used (not included)

- [Servo](https://github.com/arduino-libraries/Servo) by Arduino: crossing
  gates (`src/MRA_CrossingGates.h`). Install with `arduino-cli lib install Servo`.
- SoftwareSerial, part of the Arduino AVR core: sound (`src/MRA_Sound.h`).

- Layout tools: [TM1637](https://github.com/avishorp/TM1637) (Avishay
  Orpaz), [SSD1306Ascii](https://github.com/greiman/SSD1306Ascii) (Bill
  Greiman), [PubSubClient](https://github.com/knolleary/pubsubclient) (Nick
  O'Leary), [Adafruit PWM Servo Driver](https://github.com/adafruit/Adafruit-PWM-Servo-Driver-Library),
  [ArduinoJson](https://arduinojson.org) (Benoît Blanchon), Adafruit GFX,
  ST7789 and SSD1306, and [U8g2_for_Adafruit_GFX](https://github.com/olikraus/U8g2_for_Adafruit_GFX)
  (Oliver Kraus).

All are used through their normal `#include`, not copied into this repo,
and keep their own licenses.

