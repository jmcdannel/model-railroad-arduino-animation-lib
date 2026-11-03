---
version: 1
slug: "web-index-html"
primary_target: "web/index.html"
related_targets: ["web/style.css","web/js/app.js"]
---

# Configurator (web/index.html)

## Scope
Mode: Operate. The whole configurator: pick effects, wire them, choose when they run, install, test. Newcomers to Arduino are the primary path; planning a multi-effect board is reachable but secondary.

## Audience, job, constraints
Model railroaders new to Arduino, at the layout or planning at a desk. Job: get a wired, installed, working Nano. Must hand them a per-pin wiring guide, a lettered parts list, and test and troubleshooting steps. A shareable plan was not asked for. Browser install is Chrome/Edge desktop only and covers single effects plus four sets; everything else gets a sketch and commands.

## Direction contract
THESIS: Wiring a Nano reads like a craftsman kit's instruction sheet: a lettered parts tree and numbered steps that call parts by letter. It refuses the category default, a form-heavy dashboard of pin dropdowns and settings.
OWN-WORLD: A white sheet with 2px ink rules; ink #141414, muted #5b5b58, hairlines #e2e1dc; kit-box yellow #f2b705 only on the current step and the one primary action; blue #2d5da8 for pins and selection; black lettered keys (A, B, C…) as inline badges; line-art parts (LEDs, resistors with real color bands, sensors, servos, wire); circled step numbers that fill black when done; Barlow Semi Condensed headings, Atkinson Hyperlegible body.
STORY: A newcomer picks an arc welder, sees exactly which lettered parts to buy, wires each pin by numbered steps that name parts by letter and pins by their place on the real Nano, chooses in plain words when it runs, installs with one button, then follows the test checklist. A planner adds more effects; letters and pins continue across one board and clashes show on the pin map.
FIRST VIEWPORT: Top bar: mark and "Layout Effects", the board's name, five step tabs (Pick effects, Wire it, When it runs, Install, Test) with done checks. Left, about 60%: the current step as a sheet; on Wire it, the title, a one-line effort summary, numbered wiring steps with lettered keys and Done checkboxes, a tip, and Back, progress and the yellow Next button pinned at the bottom. Right, 568px behind a 2px ink rule: Your Nano, with the lettered parts tree, the real-layout Nano pin map with pins in use filled, budget meters (PWM pins, pins, flash) and the problems status. Signature interaction: the letters link the three; focusing or hovering a wiring step lights its parts in the tree and its pins on the map, and checking Done fills its number.
FORM: Kit Instruction Sheet, position 1 on the ordered list (chosen as Impeccable's pick over the assigned Track Plan); seed:d957c4f5128304e2
FINISH: unreviewed and undocumented is unfinished; this build ends with the finish review, the verdict, DESIGN.md, and every shipping raster carrying its provenance

## Unresolved
Pin editing lives inline in the wiring steps. The flash budget is exact for prebuilt sets and estimated otherwise.
