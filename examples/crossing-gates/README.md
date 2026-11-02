# 🚧 Crossing Gates (Arduino Nano)

The [grade crossing flashers](../crossing/) plus one or two servo-driven
gate arms and a bell output. A train hits a sensor: the flashers start and
the bell rings, the gates wait 3 s, lower slowly and land with a small
bounce. When the train has cleared, the gates rise, and the flashers stop
once they're up. If another train arrives while they're rising, they come
straight back down.

Needs the **Servo** library: `arduino-cli lib install Servo` (the Arduino
IDE usually has it already).

## 🔌 Wiring

| Nano pin | Goes to | Notes |
|----------|---------|-------|
| D5  | 220 Ω → red LED | left lamp |
| D6  | 220 Ω → red LED | right lamp |
| D9  | gate 1 servo signal (orange/yellow) | |
| D10 | gate 2 servo signal | mirrored, for the other side of the road; `NO_PIN` for one gate |
| D2  | IR sensor A `OUT` | FC-51 / TCRT5000 module: VCC → 5V, GND → GND |
| D3  | IR sensor B `OUT` | `NO_PIN` for a single sensor at the crossing |
| D4  | pushbutton → GND | internal pull-up, no resistor needed |
| D7  | bell output | HIGH while the bell should ring |

**Servo power:** red → 5V, brown/black → GND. Two micro servos (SG90) run
fine from the Nano's 5V on a good USB supply; bigger ones need their own
5 V supply with its GND joined to the Nano's. A 470 µF capacitor across the
servo supply stops resets when they start moving.

**Why D5/D6 for the lamps?** The Servo library takes over Timer1, so D9 and
D10 can't fade LEDs anymore. The servos sit there instead.

**Bell sound:** wire D7 to D12 of a Nano running the [sound](../sound/)
example (BELL preset), plus GND to GND.

## 📤 Upload

From the repo folder (no install needed):

```sh
arduino-cli lib install Servo
arduino-cli compile --fqbn arduino:avr:nano --library . examples/crossing-gates
arduino-cli upload  --fqbn arduino:avr:nano -p /dev/cu.usbserial-XXXX examples/crossing-gates
# clone with old bootloader: --fqbn arduino:avr:nano:cpu=atmega328old
```

Or install the library once and open it from *File → Examples → Model
Railroad Animation*. Arduino IDE: download the repo as a ZIP, then *Sketch →
Include Library → Add .ZIP Library*. arduino-cli:
`arduino-cli config set library.enable_unsafe_install true`, then
`arduino-cli lib install --git-url https://github.com/jmcdannel/model-railroad-arduino-animation-lib.git`.
The example is *crossing-gates*.

## 🎛️ Start mode

Like the plain crossing, it's armed from power-up: the sensors and button
run it. D12 is an extra way to fire it (a DEJA.js IO pin, a detection
block): set `USE_TRIGGER = true` and the gates come down while D12 is HIGH.
It's off by default because an unwired D12 reads HIGH, which would mean
"train!".

## 🎬 What it does

```
train ──▶ flashers + bell ──▶ 3 s ──▶ gates lower (7 s, eased) ──▶ bounce ──▶ down
clear + 3 s ──▶ gates rise (5 s) ──▶ up ──▶ flashers off
```

The servos are switched off between moves so they don't buzz or creep; the
arm's own friction holds it.

## 🎛️ Tuning

Set these in the sketch's `tune()` block. Gate settings are `g.…`
([`src/MRA_CrossingGates.h`](../../src/MRA_CrossingGates.h)), crossing
settings are `c.…` ([`src/MRA_Crossing.h`](../../src/MRA_Crossing.h)).

- `g.upDeg`, `g.downDeg` — **set these first** to match your servo horns
- `g.mirrorGate2` — gate 2 faces the other way (default true)
- `g.delayMs` — flashers before the gates move (prototype: at least 3 s)
- `g.lowerMs`, `g.raiseMs` — how long the arms take
- `g.bounceDeg`, `g.bounceMs` — the landing bounce (0 = none)
- `g.bellUntilDown` — bell stops when the gates are down (false = rings until they're up)
- `g.detachMs` — how long the servos stay powered after a move
- `g.minUs`, `g.maxUs` — servo pulse range, if yours doesn't reach 0–180°
- `c.holdMs`, `c.flashesPerMin`, … — see the [crossing](../crossing/) README
