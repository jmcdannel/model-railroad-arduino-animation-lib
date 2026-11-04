# 🎛️ Servo Tuner (Arduino Nano)

A bench tool for turnout and crossing-gate servos. It does four jobs:

- **Centers every servo at power-up**, so you can fit the horn straight
  before mounting.
- **Finds the end points**: nudge a degree at a time until the points close
  (or the gate sits level), and mark that spot.
- **Checks the throw**: glide or swing between the two ends, slowly, before
  you glue anything down.
- **Prints the numbers**, ready to paste into a DEJA turnout or the
  [crossing-gates](../crossing-gates/) sketch, and saves them in EEPROM.

Angles map to pulses the same way as DEJA's own firmware (0° = 650 µs,
180° = 2350 µs), so an angle you find here goes straight into DEJA. That holds
whether the servo is on a Nano pin or a PCA9685 board.

## 🔌 Wiring

| From | To | Notes |
|---|---|---|
| Servo signal (orange/yellow) | D9, D10 … | list them in `SERVO_PINS` |
| Servo + (red) | **a separate 5 V supply** | a moving servo browns out the Nano's 5V pin |
| Servo − (brown/black) | supply GND **and** Nano GND | the grounds must be shared |
| 10 kΩ knob | ends → 5V / GND, wiper → A0 | optional |
| Button | D2 → button → GND | optional |
| Mark button | D3 → button → GND | optional |

**PCA9685 board** (16 servos, what DEJA uses): `SDA` → A4, `SCL` → A5,
`VCC` → 5V (logic), servo power into the board's V+ terminal. Set
`DRIVER DRIVER_PCA9685`.

## 💬 With the Serial Monitor

Open it at **115200**, with *Newline* or *Both NL & CR*. Type a command and
press Enter.

| Command | Does |
|---|---|
| `s3` | select servo 3 |
| `90` | go to 90° |
| `+` `-` | nudge 1° (`+++` = 3°) |
| `]` `[` | nudge 10° |
| `c` | center (90°) |
| `a` / `b` | mark this spot as **end A** (straight / gate up) or **end B** (divergent / gate down) |
| `t` | glide to the other end |
| `w` | swing A ↔ B, pausing at each end, until the next command |
| `l` | limp: stop driving the servo so you can move it by hand |
| `p` | print every servo's numbers |
| `e` | save the end points to EEPROM |
| `?` | help |

A typical session:

```
>> +++
Servo 0: 93° (1528 µs)
>> a
📍 Servo 0 end A = 93°  (e to save)
>> 62
Servo 0: 62° (1236 µs)
>> b
📍 Servo 0 end B = 62°  (e to save)
>> w
🔁 Swinging. Any command stops it.
>> p
▶ Servo 0 (pin 9)  A 93°  B 62°
    DEJA turnout   → straight: 93  divergent: 62
    crossing gates → c.upDeg = 93; c.downDeg = 62; c.minUs = 650; c.maxUs = 2350;
>> e
💾 Saved. The end points survive power-off.
```

## 🎚️ Without a computer

Fit a knob and a button (and a mark button if you like), and set
`KNOB_PIN`, `BUTTON_PIN` and `MARK_PIN`.

- **Button:** cycles the selected servo through center → knob → swing.
  **Hold it for 1 s** to select the next servo.
- **Knob:** in knob mode, the servo follows it.
- **Mark button:** saves where the knob has it as end A, then end B on the
  next press (and saves to EEPROM). Swing mode then checks the throw.

At power-up every servo goes to center, so the board doubles as a
servo-centering jig. Set `CENTER_ON_BOOT = false` to go to each servo's
saved end A instead.

## 🎛️ Settings

| Block | Setting | Default |
|---|---|---|
| 1. Servos | `DRIVER`: `DRIVER_PINS` or `DRIVER_PCA9685` | pins |
| | `SERVO_PINS`, or `PCA9685_ADDR` + `PCA9685_COUNT` | 9, 10 / 0x40, 16 |
| 2. Pulses | `PULSE_MIN_US`, `PULSE_MAX_US` | 650, 2350 (DEJA) |
| | `CENTER_DEG`, `CENTER_ON_BOOT`, `SLOW_DEG_PER_S` | 90, true, 60 |
| 3. Controls | `KNOB_PIN`, `BUTTON_PIN`, `MARK_PIN` | -1 (none) |

## 📤 Upload

The PCA9685 build needs the **Adafruit PWM Servo Driver Library**. Pins use
the built-in Servo library.

```sh
arduino-cli compile --fqbn arduino:avr:nano examples/servo-tuner
arduino-cli upload  --fqbn arduino:avr:nano -p /dev/cu.usbserial-XXXX examples/servo-tuner
arduino-cli monitor -p /dev/cu.usbserial-XXXX -c baudrate=115200
```

🧪 Every variant in [`ci.json`](ci.json) compiles, and the serial commands
were run against a simulated Nano (nudge, mark, glide, swing, print, save).
Not yet tried with a real servo.
