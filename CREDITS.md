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

## 🧩 Libraries used (not included)

- [Servo](https://github.com/arduino-libraries/Servo) by Arduino: crossing
  gates (`src/MRA_CrossingGates.h`). Install with `arduino-cli lib install Servo`.
- SoftwareSerial, part of the Arduino AVR core: sound (`src/MRA_Sound.h`).

Both are used through their normal `#include`, not copied into this repo,
and keep their own licenses.

