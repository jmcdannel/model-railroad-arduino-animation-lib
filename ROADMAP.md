# 🗺️ Roadmap

Where this collection is headed. Each phase ships as its own PR(s), and
nothing in a later phase starts until the earlier one works on real
hardware.

## ✅ Today

Six standalone Nano sketches (welder, campfire, TV glow, lightning, grade
crossing, traffic light). Each runs from power-up, and each has an optional
D12 trigger input. Together they'd use about 10 KB of flash, about a third
of a Nano.

| Effect | Flash | RAM |
|---|---|---|
| 🔥 Welder | 4.1 KB | 65 B |
| 🔥 Campfire | 2.6 KB | 72 B |
| 📺 TV glow | 3.0 KB | 63 B |
| ⛈️ Lightning | 3.0 KB | 40 B |
| 🚦 Crossing | 2.2 KB | 62 B |
| 🚥 Traffic light | 1.3 KB | 52 B |

## 1️⃣ Effect modules + six new effects

Turn every effect into a self-contained module with the same tiny interface
(`begin()`, `update(now, on)`), so each one works both as a standalone
sketch **and** inside a multi-effect sketch.

- **Start modes, shared by every effect:**
  - **Power-up:** runs on its own, no wiring
  - **While triggered:** runs while a pin is asserted, plus an optional run-on
    time. Use it for a DEJA.js IO pin, a relay or toggle switch, or an IR/PIR
    sensor.
  - **Toggle:** a momentary button press starts it, the next press stops it
- **Six new effects:**
  - 🪧 **Neon sign:** buzzing strike-up, spell-out / flash patterns, an
    optional sputtering "broken" letter
  - 💡 **Fluorescent startup:** staggered tubes that blink, glow and strike,
    plus an optional bad tube
  - 🚨 **Rotating beacon:** amber rotator, airport (white/green) or tower
    obstruction light, with spin-up and spin-down
  - 🚧 **Servo crossing gates:** flashers, a delay, eased lowering with a
    bounce, raise, and a bell output
  - 🔔 **Sound via DFPlayer Mini:** loop a crossing bell while triggered, or
    play thunder after each lightning strike with a distance delay
  - 🌅 **Day-night room lighting:** dawn, day, dusk and night fades on LED
    strips (through MOSFETs), with a "night" output that can switch other
    effects on at dusk
- **Pick-your-effects sketch:** a `config.h` where you switch effects on and
  give each its own pins and trigger. Only the chosen effects are compiled
  in.
- **Effects that chain together:** lightning's thunder pulse → sound,
  crossing gates' bell pin → sound, day-night's night output → neon sign.

**Known limits:** the Nano has 6 PWM pins (D3, D5, D6, D9, D10, D11) and the
Servo library takes PWM away from D9/D10. Multi-effect builds need a pin
plan; the configurator (below) will check it for you.

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

**To prove first:** that all effects fit in one firmware alongside the
settings code (estimated 22–26 KB of the 30 KB), and the bootloader choice
for older Nanos and clones (Nano vs. "old bootloader" baud rates).

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
