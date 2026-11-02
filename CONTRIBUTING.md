# 🤝 Contributing

Thanks for helping bring more layouts to life! New effects, fixes, better
wiring notes and photos of these running on your railroad are all welcome.
Where the project is headed is in [ROADMAP.md](ROADMAP.md).

Be kind and assume good intent. This is a hobby project, and everyone here
is building a railroad for fun.

## 🧰 Setup

1. Install [arduino-cli](https://arduino.github.io/arduino-cli/latest/installation/)
   (or use the Arduino IDE) and the AVR core:
   ```sh
   arduino-cli core install arduino:avr
   ```
2. Install [gitleaks](https://github.com/gitleaks/gitleaks#installing)
   (`brew install gitleaks` on a Mac) and turn on the repo's pre-commit hook.
   It stops a commit that contains a password or key:
   ```sh
   git config core.hooksPath .githooks
   ```
3. Check that everything compiles (from the repo root):
   ```sh
   arduino-cli lib install Servo     # for the crossing gates
   arduino-cli compile --fqbn arduino:avr:nano --library . examples/welder
   ```

## 🔐 Secrets: the one hard rule

**Never commit a WiFi password, network name, IP address, API key, token or
any other credential.** Not even "just for a minute" in a branch: GitHub
keeps everything, and this repo is public.

- Put credentials in `arduino_secrets.h` next to your sketch (that name is
  gitignored, and it's the Arduino convention). Commit an
  `arduino_secrets.example.h` with placeholders like `"your-password"`.
- Three layers check for leaks: the pre-commit hook (gitleaks), GitHub push
  protection, and a required **Secret scan** CI check that scans every
  commit on every PR. Custom rules in [`.gitleaks.toml`](.gitleaks.toml)
  catch Arduino-style `#define WIFI_PASSWORD "…"` and
  `const char* ssid = "…"` that the stock rules miss.
- **If a secret does get pushed:** change the password or revoke the key
  *first*. Deleting the commit doesn't un-leak it. Then follow
  [SECURITY.md](SECURITY.md).

## 🚂 Adding an effect

An effect is a **module** in `src/` plus a ready-to-flash **example**:

```
src/MRA_MyEffect.h            ← the effect: a class in namespace mra
examples/my-effect/
  my-effect.ino               ← folder and sketch names must match (Arduino requires it)
  README.md                   ← what it looks like, wiring table, start mode, tuning
```

The module's shape, so every effect works alone and alongside others:

```cpp
class MyEffect {
 public:
  struct Config { uint16_t stepMs = 500; /* … tuning, with defaults */ } cfg;
  MyEffect(uint8_t pinA = 9, uint8_t pinB = NO_PIN);  // pins in, NO_PIN for optional ones
  void begin();                         // pinMode, initial state; reads cfg
  void update(uint32_t now, bool on);   // call every loop(); never blocks
};
```

Conventions:

- **Never block:** `millis()` state machines, no `delay()`, so it can share
  a Nano with other effects. Throttle with a tick (`if (now - lastTick_ <
  TICK_MS) return;`) when you don't need every millisecond.
- **`on` means running.** Decide what *off* looks like (fade out, finish
  the current cycle, coast down…), document it in the header comment, and
  restart cleanly when `on` comes back.
- **Tuning goes in `Config`**, grouped and commented, with sensible
  defaults. Examples expose the useful ones in a `tune()` block at the top.
- **Start modes come free:** the example uses `Trigger` (POWER_UP,
  WHILE_TRIGGERED, TOGGLE). Don't read the trigger pin in the module.
- **Mind 16-bit `int`:** on the Nano `int` is 16 bits, so use `long` /
  `uint32_t` for anything that can pass 32,767 (times, products).
- **Libraries:** none if at all possible. If you need one, include it only
  from your effect's header (not `ModelRailroadAnimation.h`), say so at the
  top of the README with the install command, and add it to
  `library.properties` `depends=` and the CI install step.
- **README wiring table:** pin → resistor → part, plus a tip on placing the
  LEDs so the effect reads well on a layout. Add a row to the effects table
  in the top-level README and an entry in `examples/multi-effect/`.
- **Keep it small:** a Nano has 30 KB of flash and 2 KB of RAM, and all
  twelve current effects together already use 97% of the flash. CI reports
  sizes on every PR.

Compile from the repo root without installing anything:

```sh
arduino-cli compile --fqbn arduino:avr:nano --library . examples/my-effect
```

## 🙏 Borrowed code and ideas

Credit generously, and copy carefully.

- **Inspired by someone's sketch or article?** Great: write your own version
  and credit them in [CREDITS.md](CREDITS.md) and the effect's README.
- **Copying code?** Only from code under a license that's compatible with
  this repo's MIT license: MIT, BSD, zlib, Apache-2.0, or public domain /
  CC0. Keep its copyright header, note any changes, and add it to
  CREDITS.md under "Vendored code".
- **Not OK to copy:** GPL/LGPL code, code with no license at all (that means
  "all rights reserved"), or listings from magazines and books. Use those as
  inspiration only, and write your own code.

## 🔀 Pull requests

1. Branch from `main` (`git switch -c add-neon-sign`) and open a PR. The PR
   template has a short checklist.
2. Two checks must pass: **Secret scan** and **Compile sketches**. The
   compile job posts a flash/RAM size table on the PR.
3. PRs are **squash-merged**, so one PR becomes one commit on `main`. Make
   the PR title a good commit message. Emoji are welcome 🚦.
4. `main` is protected: no direct pushes, no force-pushes, linear history,
   signed commits only (GitHub signs the squash commit for you).
5. Say whether you tested on real hardware or only compiled. "Compile-only"
   is fine, just label it.

## 🐛 Issues

Use the templates: **Bug report** for something broken, **Effect idea** for
something new. For a security problem or a leaked secret, report it
privately (see [SECURITY.md](SECURITY.md)), not in a public issue.

By contributing, you agree your work is released under the repo's
[MIT license](LICENSE).
