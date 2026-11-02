# Product

<!-- impeccable:product-schema 1 -->

## Platform

web

## Users

Model railroad hobbyists who are new to Arduino: they can wire an LED and a
resistor, but have never opened the Arduino IDE or written a sketch. They
want a scene on their layout to come alive (a welder in the shop, a crossing
that flashes for trains, a town that lights up at dusk) without learning to
program.

They use the configurator in two situations, about equally:

- **At the layout or workbench**, with a laptop beside the scene and a Nano
  on a USB cable, switching between the page and the wiring.
- **Planning at a desk**, working out which effects go on which board and
  how to wire them before touching hardware; installing may happen later,
  on another computer.

Experienced Arduino users and DEJA.js layout owners use it too, and must not
be slowed down, but the newcomer sets the bar for what gets explained.

## Product Purpose

An open-source library of lighting, motion and sound effects for model
railroads that run on an Arduino Nano, plus a web configurator that turns a
few choices into a working board. Success is a newcomer getting an effect
running on their layout on the first try: the right pins, wired safely,
installed without a toolchain.

## Positioning

- Effects are small modules that run alone or several to one Nano, each with
  its own start mode: at power-up, while a pin is triggered (a DEJA.js IO
  pin, a relay, a sensor), or toggled from a button.
- The configurator installs straight from the browser: it writes the user's
  pins and settings into a prebuilt firmware and flashes it over USB, so no
  Arduino IDE is needed. Any combination it can't install comes with a
  generated sketch and copy-and-paste commands instead.
- The page checks the pin plan before anything is flashed (pins used twice,
  fades on pins that can't fade, servo timer conflicts).

## Operating Context

- Hardware: Arduino Nano (genuine with the new bootloader, or clones with the
  old one), LEDs and resistors, IR obstacle sensors, buttons, hobby servos,
  DFPlayer Mini sound modules, LED strips through MOSFETs.
- Triggers often come from DEJA.js, a model railroad control system: a DEJA
  IO pin wired to the Nano drives an effect on or off.
- The page is static on GitHub Pages; the source, wiring guides and example
  sketches live in the GitHub repo.

## Capabilities and Constraints

- Arduino Nano limits: 30 KB of usable flash, 2 KB of RAM, six PWM pins (D3,
  D5, D6, D9, D10, D11). The Servo library takes PWM away from D9 and D10.
  All twelve effects together nearly fill a Nano, so browser install covers
  single effects and a few prebuilt sets.
- Browser install needs Web Serial: Chrome or Edge on a desktop. Every other
  browser gets the commands and sketch download.
- Terminology: "effect", "start mode" (power-up / while triggered / toggle),
  "trigger pin", "pin plan", "bootloader (new / old)".
- Effects other than the arc welder have been compiled and simulated but not
  yet run on real hardware; installing from the page has not yet been tested
  on a real Nano.

## Brand Commitments

An independent open-source project under its own name, "Model Railroad
Arduino Animation Lib" (configurator page: "Layout effects configurator").
DEJA.js is one supported way to trigger effects, not the brand. MIT
licensed; borrowed ideas and code are credited in CREDITS.md.

## Evidence on Hand

- The arc welder runs on the owner's layout (an older standalone build).
- Twelve effects with wiring guides and example sketches in `examples/`.
- No photos, videos, testimonials or user counts yet. Don't invent any.

## Product Principles

- **First try, first board.** Defaults and explanations are aimed at someone
  wiring their first Nano; nothing assumes Arduino knowledge.
- **Check before you flash.** Catch wiring and pin mistakes on the page,
  before they become a confusing board.
- **Never a dead end.** When one-click install can't do something, say why
  and hand over a path that works (sketch and commands).
- **The layout is the point.** Effects are described by what they look like
  on a railroad, not by how they're coded.
- **Open and honest.** Say what's tested and what isn't, and credit others'
  work.
