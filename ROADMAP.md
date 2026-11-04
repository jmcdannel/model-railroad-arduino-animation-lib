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

## 🧰 Layout tools (in progress)

The collection is growing past animation into a model railroad Arduino kit
(a repo rename may follow). Tools are standalone sketches in `examples/`,
one file each with numbered settings blocks at the top, and every one can
be used more than one way: on its own with a display, from the Serial
Monitor, plugged into DEJA, or over MQTT. Simple enough to edit by hand.

- ✅ **🚄 Speedometer:** two IR sensors at a known gap → scale speed.
  TM1637 or OLED, serial text or JSON, DEJA USB sensors, MQTT on an ESP32.
- ✅ **🎛️ Servo tuner:** a Nano (or PCA9685 board) on the bench: center a
  servo, nudge it, find and save both end points, test the throw, then print
  the numbers to paste into DEJA (same 0–180 → 650–2350 µs mapping as
  DEJA's own firmware) or into the crossing-gates sketch. Serial keys, or a
  knob and a button with no computer.
- ✅ **🪧 DEJA room sign:** an ESP32 with a screen showing the guest room code
  and a QR to join, plus status pages (locos running, track power, server
  health). Reads the feeds the DEJA server already publishes on MQTT
  (`ROOM_SIGN_MQTT=true`). A generic version of the sign on Tamarack
  Junction, plus a status-monitor mode with no room code.

Ideas for later: block occupancy detector (current sense) as DEJA sensors,
turnout panel with a servo, frog relay and fascia LEDs, ABS signal heads
driven by DEJA blocks, a fast-clock display, a sound trigger box.

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
