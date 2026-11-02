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

## ✅ Phase 2: web configurator and installer (built)

One page on GitHub Pages (`web/`, deployed by `.github/workflows/pages.yml`).
You pick effects, pins, start modes and settings, then either:

- **⚡ Install to Nano** (Chrome or Edge on desktop): the page downloads a
  prebuilt firmware, writes your settings into it, and flashes it over USB
  with Web Serial (STK500, new or old bootloader).
- **📋 Build it yourself** (any browser): the page generates a readable
  sketch and the `arduino-cli` commands to build it, for any combination.

**How it fits on a Nano.** Phase 1 measured all twelve effects at 97% of
flash, so there's no single "everything" firmware. Instead:

- **Prebuilt firmwares** for every single effect plus four themed sets
  (`web/js/bundles.js`), the biggest 13 KB.
- **A settings block in flash.** Each firmware keeps every pin, start mode
  and setting in one `SETTINGS` block in flash, marked `MRACFG`. The page
  writes your values into that block before flashing. That replaces the
  EEPROM plan: there's no settings protocol on the Arduino, and nothing
  extra to keep in RAM.
- **One generator.** It writes both the prebuilt firmwares (CI) and the
  copy-and-paste sketches (page) from a schema read straight from the
  `Config` structs in `src/`, so the page can't drift from the code.

**Checked in CI:**
- the published blocks match the page's encoder byte for byte,
- randomized settings (floats, enums, negative numbers) match byte for byte,
- every copy-and-paste sketch compiles,
- the flasher passes against a fake bootloader (`test/`).

**Still to do:**
- 🔌 Flash a real Nano from the page, on both a new-bootloader Nano and an
  old-bootloader clone. The protocol is tested, but the reset timing over
  Web Serial needs a real board.
- More themed sets, if people ask for particular combinations.

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
