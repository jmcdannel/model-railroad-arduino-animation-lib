---
name: Layout Effects
description: A craftsman kit's instruction sheet for wiring an Arduino Nano into a model railroad scene.
colors:
  ink: "#141414"
  mute: "#5b5b58"
  faint: "#8f8e88"
  rule: "#e2e1dc"
  edge: "#cfcec8"
  paper: "#ffffff"
  shade: "#f6f5f1"
  kit-yellow: "#f2b705"
  kit-yellow-soft: "#fdf3cf"
  pin-blue: "#2d5da8"
  pin-blue-soft: "#e6edf8"
  ok: "#1d7a46"
  ok-soft: "#e5f2ea"
  bad: "#b8322a"
  bad-soft: "#fbe9e7"
typography:
  display:
    fontFamily: "Barlow Semi Condensed, Arial Narrow, sans-serif"
    fontSize: "40px"
    fontWeight: 600
    lineHeight: 1.1
  headline:
    fontFamily: "Barlow Semi Condensed, Arial Narrow, sans-serif"
    fontSize: "26px"
    fontWeight: 600
    lineHeight: 1.15
  title:
    fontFamily: "Barlow Semi Condensed, Arial Narrow, sans-serif"
    fontSize: "20px"
    fontWeight: 600
    lineHeight: 1.2
  body:
    fontFamily: "Atkinson Hyperlegible, system-ui, sans-serif"
    fontSize: "16px"
    fontWeight: 400
    lineHeight: 1.5
  lede:
    fontFamily: "Atkinson Hyperlegible, system-ui, sans-serif"
    fontSize: "18px"
    fontWeight: 400
    lineHeight: 1.5
  label:
    fontFamily: "Atkinson Hyperlegible, system-ui, sans-serif"
    fontSize: "14px"
    fontWeight: 400
    lineHeight: 1.3
  key:
    fontFamily: "Barlow Semi Condensed, Arial Narrow, sans-serif"
    fontSize: "14px"
    fontWeight: 600
    lineHeight: 1
  pin:
    fontFamily: "Barlow Semi Condensed, Arial Narrow, sans-serif"
    fontSize: "17px"
    fontWeight: 600
    lineHeight: 1
  button:
    fontFamily: "Barlow Semi Condensed, Arial Narrow, sans-serif"
    fontSize: "19px"
    fontWeight: 600
    lineHeight: 1
  code:
    fontFamily: "ui-monospace, SF Mono, Menlo, monospace"
    fontSize: "13px"
    fontWeight: 400
    lineHeight: 1.55
rounded:
  key: "3px"
  md: "6px"
  frame: "8px"
  full: "50%"
spacing:
  xs: "4px"
  sm: "8px"
  md: "12px"
  lg: "16px"
  xl: "24px"
  xxl: "32px"
  sheet-x: "56px"
  measure: "62ch"
components:
  button:
    backgroundColor: "{colors.paper}"
    textColor: "{colors.ink}"
    typography: "{typography.button}"
    rounded: "{rounded.md}"
    padding: "0 22px"
    height: "48px"
  button-hover:
    backgroundColor: "{colors.shade}"
  button-primary:
    backgroundColor: "{colors.kit-yellow}"
    textColor: "{colors.ink}"
    typography: "{typography.button}"
    rounded: "{rounded.md}"
    padding: "0 26px"
    height: "48px"
  button-small:
    backgroundColor: "{colors.paper}"
    textColor: "{colors.ink}"
    rounded: "{rounded.md}"
    padding: "0 16px"
    height: "44px"
  button-disabled:
    backgroundColor: "{colors.paper}"
    textColor: "{colors.faint}"
  key-badge:
    backgroundColor: "{colors.ink}"
    textColor: "{colors.paper}"
    typography: "{typography.key}"
    rounded: "{rounded.key}"
    height: "22px"
    padding: "0 4px"
  step-number:
    backgroundColor: "{colors.paper}"
    textColor: "{colors.ink}"
    rounded: "{rounded.full}"
    size: "38px"
  step-number-done:
    backgroundColor: "{colors.ink}"
    textColor: "{colors.paper}"
  step-tab-current-mark:
    backgroundColor: "{colors.kit-yellow}"
    textColor: "{colors.ink}"
    rounded: "{rounded.full}"
    size: "26px"
  choice:
    backgroundColor: "{colors.paper}"
    textColor: "{colors.ink}"
    rounded: "{rounded.md}"
    padding: "12px 14px"
  choice-selected:
    backgroundColor: "{colors.pin-blue-soft}"
  done-toggle:
    backgroundColor: "{colors.paper}"
    textColor: "{colors.ink}"
    rounded: "{rounded.md}"
    padding: "0 14px"
    height: "44px"
  done-toggle-checked:
    textColor: "{colors.pin-blue}"
  notice:
    backgroundColor: "{colors.paper}"
    textColor: "{colors.ink}"
    rounded: "{rounded.md}"
    padding: "12px 16px"
  notice-bad:
    backgroundColor: "{colors.bad-soft}"
  input:
    backgroundColor: "{colors.paper}"
    textColor: "{colors.ink}"
    typography: "{typography.body}"
    rounded: "{rounded.md}"
    padding: "7px 10px"
    height: "42px"
  code-block:
    backgroundColor: "{colors.shade}"
    textColor: "{colors.ink}"
    typography: "{typography.code}"
    rounded: "{rounded.md}"
    padding: "14px 16px"
  parts-tree:
    backgroundColor: "{colors.paper}"
    rounded: "{rounded.frame}"
    padding: "20px 14px 12px"
---

# Design System: Layout Effects

## Overview

**Creative North Star: "The Kit Instruction Sheet"**

The configurator is printed matter from a craftsman kit box: a white sheet ruled in ink, a lettered parts tree drawn in line art, and numbered steps that call each part by its letter and each pin by its place on the real Nano. Every screen is a sheet you work down, not a dashboard you configure. Structure comes from 2px ink rules and hairlines, never from cards, fills or shadows.

Density is generous but practical: one current step on the left as a sheet, the board itself ("Your Nano") on the right behind a heavy ink rule. Color is scarce and every hue has a job. Kit-box yellow says "you are here" and "do this next"; blue says "pin" and "selected"; green and red speak only about status. Everything else is ink on paper.

The letters are the system's spine. The same black lettered key appears in the parts tree, in the wiring steps and beside pins in the use list, and hovering or focusing a step lights its parts and pins together. New surfaces should extend that linkage rather than invent parallel labeling.

**Key Characteristics:**
- White sheet, ink (#141414) rules at 2px for structure, warm hairlines for rows.
- Lettered black key badges that link parts, steps and pins.
- Circled step numbers that fill ink when done; dashed when optional.
- Line-art parts on a sprue runner; the Nano drawn in its real pin layout.
- Condensed industrial headings over a highly legible body face.
- Flat: no shadows anywhere.

## Colors

Ink on paper with two working hues, yellow for place and action, blue for pins and selection, plus a pair of status colors that never decorate.

### Primary
- **Kit-Box Yellow** (kit-yellow): the current step's tab mark and the one primary action on the sheet (Next, or Install on the Install step, where Next drops to a plain button). Also the brand mark's fill and the ring around a lit pin on the pin map.
- **Lit Yellow** (kit-yellow-soft): the "lit" wash on a wiring step row and on its parts in the tree when the step is hovered or focused; text selection.

### Secondary
- **Pin Blue** (pin-blue): pin names (D9, GND), pins in use on the pin map, chosen state (checked Done, selected choice border), links and link-buttons, the focus ring, checkbox and radio accent, progress and budget meter fills.
- **Selection Wash** (pin-blue-soft): background of a selected choice.

### Tertiary
- **Pass Green** (ok) / **Pass Wash** (ok-soft): status only, e.g. the "No problems" check icon and "On this board" mark.
- **Fault Red** (bad) / **Fault Wash** (bad-soft): status only, e.g. pin clashes on the map and in the pin select, over-budget meters, invalid fields, blocking notices.

### Neutral
- **Ink** (ink): text, 2px structural rules, the parts tree frame, key badges, filled step numbers, button borders.
- **Graphite** (mute): secondary text, ledes, hints, notes, pin map labels, done-step text.
- **Pencil** (faint): disabled button text only.
- **Hairline** (rule): row dividers between wiring steps, catalog items, checklists; empty meter track.
- **Edge** (edge): borders of quieter controls (Done toggle, choices, board name field, code block).
- **Paper** (paper): the sheet.
- **Bench Grey** (shade): code blocks and the plain button's hover.

### Dark mode 🌙
Follows the system setting (`prefers-color-scheme`). It is the same sheet drawn in light ink on graphite, not a new look. Every role keeps its token, and only the values change:

| Token | Light | Dark |
|---|---|---|
| ink | #141414 | #ecebe6 |
| mute | #5b5b58 | #a9a8a1 |
| faint | #8f8e88 | #7a7973 |
| rule | #e2e1dc | #33322e |
| edge | #cfcec8 | #4a4944 |
| paper | #ffffff | #1c1c1a |
| shade | #f6f5f1 | #262623 |
| yellow-soft | #fdf3cf | #3b3216 |
| blue / blue-soft | #2d5da8 / #e6edf8 | #8fb1ea / #1f2b40 |
| ok / ok-soft | #1d7a46 / #e5f2ea | #62c48f / #173020 |
| bad / bad-soft | #b8322a / #fbe9e7 | #f0796d / #3a1d1a |

- **Yellow** (#f2b705) is the same in both. Text on it is always `--on-yellow` (#141414).
- Anything filled with ink (key badges, done step numbers) uses paper for its text, so it inverts with the theme.
- Part drawings are line art in `currentColor`, with the classes `.pf` (paper), `.pi` (ink) and `.pb` (blue-soft). Real part colors stay fixed: LED tints, strip tints, resistor bands, and the resistor body (`--resistor`, so the bands read on dark).
- Never write a literal color in markup or SVG. Add a token instead.

### Named Rules
**The Yellow Means Here Rule.** Full-strength yellow appears on at most one action per sheet plus the current step mark. If a second yellow button shows up, one of them is not primary.

**The Status Is Not Decoration Rule.** Green and red appear only when they report a check result. Never use them for accents, categories or illustration.

**The Blue Is a Pin Rule.** Blue marks pins, selection, links and focus. It is never a fill for a panel or a heading.

## Typography

**Display Font:** Barlow Semi Condensed 600 (with Arial Narrow, sans-serif)
**Body Font:** Atkinson Hyperlegible 400/700 (with system-ui, sans-serif)
**Label/Mono Font:** ui-monospace, SF Mono, Menlo (code only)

**Character:** A condensed, workshop-stencil heading face carries titles, step numbers, key letters, pin names and buttons; a face built for low-vision legibility carries every sentence a newcomer has to read.

### Hierarchy
- **Display** (600, 40px, 32px under 640px, 1.1): the sheet title, one per step.
- **Headline** (600, 26px to 28px, 1.15): section heads on a sheet and the "Your Nano" column head.
- **Title** (600, 19px to 21px, 1.2): wiring step titles, catalog effect names, column sub-heads (Parts, Pin map).
- **Lede** (400, 18px, 1.5, Graphite with an Ink bold lead): one-line effort summary under the title, max 62ch.
- **Body** (400, 16px, 1.5): instructions and hints, max 62ch; notices and tips cap at 72ch.
- **Label** (400, 13px to 15px): notes, field help, part names, meter captions.
- **Key / Pin / Button** (Barlow 600, 14px / 17px / 19px): lettered keys, pin names, button text.
- **Code** (13px, 1.55): generated sketches and commands only.

### Named Rules
**The Two Voices Rule.** Barlow names things (titles, letters, pins, numbers, actions); Atkinson explains them. Never set a sentence of instruction in Barlow.

**The Monospace Is Code Rule.** Monospace appears only inside code blocks. Pin names are Barlow in blue, not monospace.

## Layout

A two-column board: the current sheet on the left (flexible, padding 40px 56px 28px) and "Your Nano" on the right at a fixed 568px behind a 2px ink rule, sticky to the viewport. The top bar is 64px tall, closed by a 2px ink rule, with the step tabs pushed right.

Rows, not boxes: wiring steps, catalog entries and checklists are list rows separated by hairlines (16px vertical padding), each wiring row a three-column grid of number (56px), path, Done toggle. Reading text is capped at 62ch.

The step footer (Back, progress text, primary action) is sticky to the bottom of the sheet behind a 2px ink rule, primary action pushed right.

Responsive: at 1080px and below, "Your Nano" drops below the sheet (ink rule moves to its top, no longer sticky), the step tabs wrap to their own full-width row, and a compact parts legend appears above the wiring steps so letters stay readable without scrolling. At 640px and below, side padding becomes 16px, step tab labels hide except the current one, the Done toggle drops beneath the step path, the parts tree goes to two columns and the footer wraps with the progress text on its own line.

Spacing runs on a loose 4px base (4, 8, 12, 16, 24, 32), with 56px sheet gutters on desktop.

Print 🖨️: the Wire it sheet prints as a bench sheet, always in light colors. "Your Nano" comes first (parts tree in five columns, pin map, pin list), then the numbered steps. Pins print as plain text, every step number prints as an empty circle to tick by hand, and rows never split across pages. Navigation, Done toggles, the legend, meters and the footer are left off.

## Elevation & Depth

Flat. There are no shadows. Depth and grouping come from rule weight: 2px to 3px ink for structure (top bar, column divider, footer, parts tree frame, notices at 1.5px), 1px warm hairlines for rows, and soft tonal washes (lit yellow, selection blue, fault red) for state.

### Named Rules
**The Ruled Sheet Rule.** Separate with lines, not lifts. If something needs more emphasis, give it a heavier ink border, not a shadow or a filled card.

## Shapes

Small, consistent corners on a printed-sheet geometry: 6px on buttons, fields, choices, notices and the code block; 3px on key badges and part cells; 8px on the parts tree and legend frames. Circles are reserved for step numbers and pin holes. Dashed borders signal "optional" (step number) or "empty" (empty parts tree). Line-art parts are drawn with real details (resistor color bands, LED legs with a + mark).

## Components

### Buttons
Ink-bordered, square-ish and plain; only one ever wears yellow.
- **Shape:** gently rounded (6px), 1.5px ink border, 48px tall.
- **Plain:** paper fill, ink Barlow 600 at 19px, 22px side padding; hover to Bench Grey.
- **Primary:** Kit-Box Yellow fill, 2px ink border, 26px side padding; hover lightens slightly.
- **Small:** 44px tall, 17px text, 16px padding (catalog Add/Remove, Copy commands).
- **Disabled:** Pencil text, Edge border, paper fill, not-allowed cursor.
- **Link button:** blue underlined text with a 44px hit area (Download sketch).

### Lettered Key Badge (signature)
A 22px ink tile with a white Barlow capital (A, B, C…), 3px corners, set inline in running text and pinned to the top-left of each part cell. One letter per part per board, continuing across effects.

### Step Number (signature)
A 38px circle with a 2px ink border and a Barlow 20px numeral on paper. Fills ink with a white numeral when its Done box is checked (160ms color transition, off under reduced motion). Optional steps use a dashed border. Top-bar step tabs use the same idea at 26px: outline by default, Kit-Box Yellow with an ink border when current, ink-filled with a white check when done.

### Wiring Step Row
Number, title, and an inline path ("Nano D9 → D 220 Ω resistor → A long leg…") where pins are blue Barlow selects and parts are key badges. A Done toggle (44px, Edge border, blue when checked) sits right. Hover or focus lights the row in Lit Yellow and lights its parts and pins in the column.

### Parts Tree (sprue, signature)
A 3px ink frame (8px corners) holding a three-column grid of line-art parts. A 2px ink runner spans each row with a short gate dropping to every part, like a plastic kit sprue. Each part shows its key, its drawing, its name and quantity (×2). Lit parts take the Lit Yellow wash.

### Nano Pin Map (signature)
The Nano drawn in its real two-row header layout with the USB end marked. Pin holes are paper circles with 2px ink strokes; in-use pins fill blue, clashing pins fill red, lit pins take a 4px yellow ring. Labels are Barlow 11.5px Graphite, blue and semibold when in use; PWM pins carry a "~".

### Choices
Full-width radio rows (6px corners, Edge border, 12px 14px padding) with a bold label and a Graphite explanation. Selected: blue border, Selection Wash fill.

### Inputs / Fields
- **Style:** paper fill, 1px grey border, 6px corners, 42px min height, tabular numbers.
- **Focus:** global 2px blue outline, 2px offset.
- **Error:** red border on Fault Wash with a 13px red message below.

### Notices, Tips and Status
Bordered panels (1.5px ink, 6px corners, max 72ch) with an inline Barlow 18px lead word then Atkinson text on the same line. Fault variants switch to a red border on Fault Wash. The pin status panel leads with a line-drawn circle icon in Pass Green or Fault Red.

### Meters
Three budget meters (PWM pins, pins, flash): Graphite caption, Barlow tabular value, 6px blue bar on a hairline track; red when over budget.

## Do's and Don'ts

### Do:
- **Do** refer to every physical part by its lettered key, wherever it appears.
- **Do** separate content with 2px ink rules and 1px hairlines instead of cards.
- **Do** keep exactly one yellow primary action per sheet; demote Next when a step has its own primary (Install).
- **Do** set pin names in blue Barlow and draw the Nano in its real layout.
- **Do** cap reading text at 62ch and notices at 72ch.
- **Do** keep touch targets at 44px or more (Done toggles, small buttons, link buttons).
- **Do** honor reduced motion: the only transitions are short color and width changes, and they switch off.

### Don't:
- **Don't** use shadows or elevated cards; this sheet is flat.
- **Don't** use green or red for anything but check results.
- **Don't** put yellow on a second button, a heading or a panel fill.
- **Don't** set instructions in the condensed heading face or pin names in monospace.
- **Don't** show part or pin choices as a dashboard of bare dropdowns; put them inline in the numbered steps.
