# 🚂 Model Railroad Arduino Animation Lib

Small Arduino sketches that bring a model railroad to life. One folder per
effect, each with a sketch, wiring notes and tuning knobs. All are designed
to run from power-up with no buttons, no libraries and no fuss.

| Effect | Board | Status |
|--------|-------|--------|
| [🔥 Arc welder](welder/) — blue/white arc flicker, red cooling glow | Nano | ✅ running on the layout (pre-trigger build) |
| [🔥 Campfire](campfire/) — two flame LEDs + ember, gusts and lulls | Nano | ✅ compiled + simulated, D12 trigger |
| [📺 TV glow](tv-glow/) — scene cuts, flashes, channel surfing | Nano | ✅ compiled + simulated, D12 trigger |
| [⛈️ Lightning storm](lightning/) — strike clusters, active/quiet spells, thunder trigger out | Nano | ✅ compiled + simulated, D12 trigger |
| [🚦 Grade crossing](crossing/) — 50 fpm flashers, 1–2 IR sensors and/or button | Nano | ✅ compiled + simulated, D12 trigger |
| [🚥 Traffic light](traffic-light/) — single head or synced 4-way, night flash mode | Nano | ✅ compiled + simulated, D12 trigger |
| [🪧 Room code sign](room-code/) — join QR + DEJA room code, now running, track power, server pages (BOOT button), status LED (WiFi + MQTT) | ESP32-C6-LCD-1.47 · ESP32 + 0.96" OLED (code page only) | ✅ live on the layout (C6); OLED build flashed |

Every sketch: one folder, one `.ino`, a README with a wiring table, tuning
constants at the top, non-blocking `millis()` timing. Verified with
`arduino-cli compile --fqbn arduino:avr:nano <folder>`.

## 🔗 Triggering effects from DEJA.js

Every sketch has an optional trigger input on **D12** (off by default). Set
`USE_TRIGGER = true` in the sketch, then run one wire from a DEJA IO pin to
D12 and one from the IO board's GND to the Nano's GND. That's the whole
circuit: the Nano's internal pull-up does the rest.

- **DEJA side:** add the IO pin to the device's output pins in Throttle and
  create a `pin` (or `light` / `relay`) effect on that device. DEJA "on"
  drives the pin HIGH, which runs the effect. No DEJA changes needed.
- **Mega (5 V):** direct wire, no caveats.
- **ESP32 / Pico W (3.3 V):** direct wire works. 3.3 V reads as HIGH on the
  Nano with a thin margin, so keep the wire short. If an effect ever blinks
  when it should be on, set `TRIGGER_ACTIVE_LOW = true` and invert the
  effect in DEJA: a 0 V "on" is unambiguous.
- **Relay module you already own:** wire its contact between D12 and GND
  and set `TRIGGER_ACTIVE_LOW = true`. Isolation without the reboot delay
  of switching the Nano's power.
- **Unplugged D12** reads HIGH, so a board keeps running standalone. The
  one exception is the crossing, where HIGH means *flashers on*.

| Effect | Trigger off | Trigger on |
|--------|-------------|------------|
| 🔥 Welder | no new jobs, glow cools down | welds |
| 🔥 Campfire | fades out | burns |
| 📺 TV glow | fades to black | plays |
| ⛈️ Lightning | dark | storms |
| 🚦 Crossing | sensors + button only | flashers run while asserted + hold |
| 🚥 Traffic light | night-flash (or dark) | normal cycle via all-red |

Ideas on deck: neon sign, fluorescent startup, rotating beacon, servo
crossing gates, crossing bell / thunder via DFPlayer, day-night room lighting.

## 🙏 Credits

The QR generator in `room-code/` is vendored from
[ricmoo/QRCode](https://github.com/ricmoo/QRCode) (MIT, Richard Moore and
Project Nayuki). The arc welder is inspired by the classic Steve Spence,
Bradt (*Model Railroader*) and Cornhill & Atherton (*MRH*) welder sketches.
Full details are in [CREDITS.md](CREDITS.md).

## 📄 License

[MIT](LICENSE) © 2026 Josh McDannel. Third-party code keeps its own license.
