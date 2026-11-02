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
3. Check that everything compiles:
   ```sh
   arduino-cli compile --fqbn arduino:avr:nano welder
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

Each effect is one folder with one sketch, and anyone should be able to
flash it without reading anything else.

```
my-effect/
  my-effect.ino   ← folder and sketch names must match (Arduino requires it)
  README.md       ← what it looks like, wiring table, tuning, trigger behavior
```

Conventions, so every effect feels the same:

- **Runs from power-up** with no buttons and no libraries if at all
  possible. If you need a library (Servo, a sound module…), say so at the
  top of the README with the exact `arduino-cli lib install` command.
- **Tuning constants at the top** of the sketch, grouped and commented, with
  the pins first.
- **Non-blocking timing:** `millis()` state machines, no `delay()` in
  `loop()`, so it can share a Nano with other effects later.
- **Optional trigger input on D12**, off by default, following the existing
  sketches (`USE_TRIGGER`, `TRIGGER_ACTIVE_LOW`, `INPUT_PULLUP`, debounced).
  Say what "trigger off" does in the README.
- **README wiring table:** pin → resistor → part, plus a tip on placing the
  LEDs so the effect reads well on a layout.
- **Add a row** to the effects table in the top-level README.
- **Keep it small:** a Nano has 30 KB of flash and 2 KB of RAM, and effects
  will be combined on one board. CI reports sizes on every PR.

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
