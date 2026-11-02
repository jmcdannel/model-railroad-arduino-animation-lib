# 🚂 Model Railroad Arduino Animation Lib

Lighting, motion and sound effects that bring a model railroad to life. Each
effect is a small module that runs on its own Arduino Nano **or** alongside
others on one board, and each one can start at power-up, follow a DEJA.js
IO pin or sensor, or toggle from a button.

| Effect | What it does | Status |
|--------|--------------|--------|
| [🔥 Arc welder](examples/welder/) | blue/white arc flicker, red cooling glow | ✅ on the layout (older standalone build) |
| [🔥 Campfire](examples/campfire/) | two flame LEDs + ember, gusts and lulls | 🧪 compiled + simulated |
| [📺 TV glow](examples/tv-glow/) | scene cuts, flashes, channel surfing | 🧪 compiled + simulated |
| [⛈️ Lightning storm](examples/lightning/) | strike clusters, active/quiet spells, thunder output | 🧪 compiled + simulated |
| [🚦 Grade crossing](examples/crossing/) | 50 fpm flashers, 1–2 IR sensors and/or button | 🧪 compiled + simulated |
| [🚥 Traffic light](examples/traffic-light/) | single head or synced 4-way, night flash | 🧪 compiled + simulated |
| [🪧 Neon sign](examples/neon-sign/) 🆕 | buzzy strike-up, spell-and-flash, a failing letter | 🧪 compiled + simulated |
| [💡 Fluorescent startup](examples/fluorescent/) 🆕 | staggered tubes that glow, blink and strike | 🧪 compiled + simulated |
| [🚨 Rotating beacon](examples/beacon/) 🆕 | rotator, airport white/green, tower light | 🧪 compiled + simulated |
| [🚧 Crossing gates](examples/crossing-gates/) 🆕 | servo gates with delay, easing, bounce, bell output | 🧪 compiled + simulated |
| [🔔 Sound](examples/sound/) 🆕 | DFPlayer Mini: crossing bell, thunder, ambient | 🧪 compiled + simulated |
| [🌅 Day-night lighting](examples/day-night/) 🆕 | dawn → day → dusk → night on LED strips, night output | 🧪 compiled + simulated |
| [🧩 Multi-effect](examples/multi-effect/) 🆕 | pick effects and pins in `config.h`, one trigger each | 🧪 compiled + simulated |

🧪 = compiles for the Nano and behaves correctly in a host simulation, but
hasn't been flashed to a real board yet. The six original effects were
ported to modules with identical output to the old sketches (checked over
30 simulated minutes each).

## 🧰 How it's built

```
src/            the effects: MRA_Welder.h, MRA_Neon.h, … (one header each)
  MRA_Trigger.h   start modes shared by every effect
examples/       one ready-to-flash sketch per effect, plus multi-effect
```

Every effect has the same two calls, `begin()` and `update(now, on)`, so a
sketch is just a few lines:

```cpp
#include <MRA_Welder.h>
using namespace mra;

Welder  welder(9, 10, 11);                 // white, blue, red
Trigger trigger(WHILE_TRIGGERED, 12);      // or POWER_UP, or TOGGLE

void setup() { welder.begin(); trigger.begin(); }
void loop()  { uint32_t now = millis(); welder.update(now, trigger.update(now)); }
```

Upload from the repo folder without installing anything:

```sh
arduino-cli compile --fqbn arduino:avr:nano --library . examples/welder
arduino-cli upload  --fqbn arduino:avr:nano -p /dev/cu.usbserial-XXXX examples/welder
```

Or install it as a library (Arduino IDE: download the ZIP, then *Sketch →
Include Library → Add .ZIP Library*) and open the effects from *File →
Examples → Model Railroad Animation*. Crossing gates also need the Servo
library (`arduino-cli lib install Servo`).

## 🎛️ Start modes

| Mode | Wiring | Use it for |
|---|---|---|
| `POWER_UP` | none | runs from power-up |
| `WHILE_TRIGGERED` | pin ← DEJA IO pin, relay, switch, sensor | runs while the pin is asserted, plus an optional run-on time (`60000` = a button press runs 1 minute) |
| `TOGGLE` | pin ← momentary button | press on, press off |

Polarity: `ACTIVE_HIGH` (default) for a DEJA IO pin or PIR sensor, where
HIGH = on and an unwired pin reads HIGH, so the board still runs on its
own. `ACTIVE_LOW` for a button, relay contact or IR obstacle module that
pulls the pin to GND.

## 🔗 Triggering effects from DEJA.js

Run one wire from a DEJA IO pin to the effect's trigger pin (D12 in the
examples) and one from the IO board's GND to the Nano's GND, then use
`Trigger trigger(WHILE_TRIGGERED, 12);`. The Nano's internal pull-up does
the rest.

- **DEJA side:** add the IO pin to the device's output pins in Throttle and
  create a `pin` (or `light` / `relay`) effect on that device. DEJA "on"
  drives the pin HIGH, which runs the effect. No DEJA changes needed.
- **Mega (5 V):** direct wire, no caveats.
- **ESP32 / Pico W (3.3 V):** direct wire works. 3.3 V reads as HIGH on the
  Nano with a thin margin, so keep the wire short. If an effect ever blinks
  when it should be on, use `ACTIVE_LOW` and invert the effect in DEJA: a
  0 V "on" is unambiguous.
- **Relay module you already own:** wire its contact between the trigger
  pin and GND and use `ACTIVE_LOW`.
- **Unplugged** reads HIGH, so with `ACTIVE_HIGH` a board keeps running
  standalone. The exceptions are the crossing and gates, where HIGH means
  *train!*, so their trigger is off unless you set `USE_TRIGGER = true`.

| Effect | Off | On |
|--------|-----|----|
| 🔥 Welder | no new jobs, glow cools down | welds |
| 🔥 Campfire | fades out | burns |
| 📺 TV glow | fades to black | plays |
| ⛈️ Lightning | dark | storms |
| 🚦 Crossing / 🚧 gates | sensors + button only | fires the crossing (+ hold) |
| 🚥 Traffic light | night-flash (or dark) | normal cycle via all-red |
| 🪧 Neon | tubes go out | strike-up, then the pattern |
| 💡 Fluorescent | all tubes out | startup sequence |
| 🚨 Beacon | lamp fades, motor coasts down | spins up |
| 🔔 Sound | loop fades out | plays (loop / one clip / ambient) |
| 🌅 Day-night | fade out + pause (or sunset → night) | cycle runs (or sunrise → day) |

## 🔌 Chaining boards

Some effects have outputs made to feed another board's trigger pin (plus a
shared GND):

- ⛈️ lightning **thunder out (D2)** → 🔔 sound **D12** (THUNDER preset)
- 🚧 gates **bell out (D7)** → 🔔 sound **D12** (BELL preset)
- 🌅 day-night **night out (D2)** → 🪧 neon **D12**, or building lights

On a single board, the [multi-effect](examples/multi-effect/) sketch links
them in software instead.

## 🗺️ What's next

A web page that configures your board and either installs it straight
from the browser or gives you the `arduino-cli` commands to copy. See
[ROADMAP.md](ROADMAP.md).

## 🤝 Contributing

New effects and fixes are welcome: see [CONTRIBUTING.md](CONTRIBUTING.md).
🔐 Never commit WiFi passwords or keys; [SECURITY.md](SECURITY.md) explains
what to do if one slips out.

## 🙏 Credits

The arc welder is inspired by the classic Steve Spence,
Bradt (*Model Railroader*) and Cornhill & Atherton (*MRH*) welder sketches.
Full details are in [CREDITS.md](CREDITS.md).

## 📄 License

[MIT](LICENSE) © 2026 Josh McDannel. Third-party code keeps its own license.
