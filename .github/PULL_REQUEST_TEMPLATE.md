## 🚂 What does this change?

<!-- One or two sentences. For a new effect: what it looks like on the layout. -->

## 🔌 Hardware

<!-- Board, LEDs/servos/modules, and the wiring if it changed. -->

## ✅ Checklist

- [ ] Compiles: `arduino-cli compile --fqbn arduino:avr:nano --library . examples/<effect>`
- [ ] Tested on real hardware (say which board), or marked "compile-only" above
- [ ] New/changed effect: module in `src/`, example + README with a wiring table, a row in the top-level README, an entry in `examples/multi-effect/`
- [ ] Tuning in `Config`, non-blocking `update(now, on)` (no `delay()`), off behavior documented
- [ ] Borrowed code or ideas are credited in `CREDITS.md`, and any copied code is under a compatible license (see CONTRIBUTING.md)
- [ ] 🔐 No WiFi passwords, IP addresses, tokens or other secrets (use `arduino_secrets.h`, which is gitignored)
