# 🗺️ Roadmap

Where this collection is headed. Each phase ships as its own PR(s), and
nothing in a later phase starts until the earlier one works on real
hardware.

## ✅ Phase 1: effect modules + six new effects (built)

Every effect is now a module in `src/` with the same tiny interface
(`begin()`, `update(now, on)`), plus a ready-to-flash example. The six
original sketches were ported with identical output (checked against the
old sketches over 30 simulated minutes each).

- **Start modes, shared by every effect** (`MRA_Trigger.h`): power-up,
  while triggered (with optional run-on), toggle; active-high or active-low.
- **Six new effects:** 🪧 neon sign, 💡 fluorescent startup, 🚨 rotating
  beacon, 🚧 servo crossing gates, 🔔 DFPlayer sound, 🌅 day-night lighting.
- **Multi-effect sketch:** a `config.h` that switches effects on, gives each
  its pins and trigger, links them (thunder after lightning, bell with the
  gates, neon at dusk) and checks the pin plan at startup.

**Still to do:** flash each new effect to real hardware and tune it by eye.
Until then they're marked 🧪 in the README.

| Effect | Flash | RAM | | Effect | Flash | RAM |
|---|---|---|---|---|---|---|
| 🔥 Welder | 5.3 KB | 160 B | | 🪧 Neon | 4.9 KB | 155 B |
| 🔥 Campfire | 3.6 KB | 130 B | | 💡 Fluorescent | 3.6 KB | 124 B |
| 📺 TV glow | 4.2 KB | 119 B | | 🚨 Beacon | 4.6 KB | 71 B |
| ⛈️ Lightning | 4.4 KB | 126 B | | 🚧 Crossing gates | 7.8 KB | 221 B |
| 🚦 Crossing | 2.9 KB | 122 B | | 🔔 Sound | 5.3 KB | 217 B |
| 🚥 Traffic light | 2.3 KB | 71 B | | 🌅 Day-night | 4.4 KB | 113 B |

(Standalone examples; Arduino core included. Settings live in RAM now, which
costs about 1 KB of flash per effect versus the old constants, and is what
lets a web page change them later.)

## 2️⃣ Web configurator and installer

One page on GitHub Pages. You pick effects, pins, start modes and tuning,
then choose how to get it onto the board:

- **⚡ Install to Nano from the browser** (Chrome or Edge on desktop): the page
  flashes a prebuilt "universal" firmware over USB with Web Serial, then
  sends your settings to the Nano, which stores them in EEPROM. You can
  re-configure later without reflashing, and you never install the Arduino
  IDE.
- **📋 Copy & paste commands** (any browser, any OS): the page generates your
  `config.h` and the exact `arduino-cli` commands to compile and upload it.

Both come from the same choices on the same page. The page also checks the
pin plan (PWM pins, Servo timer conflicts, pins used twice) before you
install.

**Measured in Phase 1:** all twelve effects in one sketch take 29.9 KB of
the Nano's 30 KB (97%) and 79% of RAM, before any settings code. So one
universal firmware with everything won't fit as-is. Options to weigh:
several prebuilt firmwares (e.g. "lights", "crossing + sound", "scenery"),
building the firmware on demand in the cloud, or slimming the effects
(dropping float math). Also to settle: the bootloader choice for older
Nanos and clones (Nano vs. "old bootloader" baud rates).

## 3️⃣ Share it

- 📦 List it in the Arduino Library Manager, so it appears in the IDE's
  library search
- 📸 Photos and short videos of each effect on a layout, in each README

## 💭 Ideas, not scheduled

- More fading pins without extra parts (software PWM), or a PCA9685 board
  for 16 smooth channels plus servos
- DCC-triggered effects: a Nano listening to accessory addresses (NmraDcc)
- ESP32 builds of the effects (WiFi/MQTT control from DEJA.js)
- More effects: welding sparks on a second LED, arc lamp flicker, a moving
  car's headlights, a fire with smoke (fan + ultrasonic mister), a station
  departure board

Have an idea? Open an **Effect idea** issue.
