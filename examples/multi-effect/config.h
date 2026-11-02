// 🧩 Pick your effects. Set USE_… to 1 for each effect you want on this
// board, give it pins, and choose how it starts. Only the effects you turn
// on are compiled in.
//
// Start modes: POWER_UP · WHILE_TRIGGERED · TOGGLE        (see MRA_Trigger.h)
// Polarity:    ACTIVE_HIGH (DEJA IO pin, PIR sensor; unwired = on)
//              ACTIVE_LOW  (button or relay to GND, IR obstacle module)
// Run-on:      WHILE_TRIGGERED keeps going this many ms after release
//              (0 = stop at once; 60000 = a button press runs 1 minute)
//
// 📌 Pin budget on a Nano:
//   • Fading needs PWM pins: 3, 5, 6, 9, 10, 11. That's six in all.
//   • Crossing gates use the Servo library, which takes PWM away from 9 and 10.
//   • Plain on/off pins: 2, 4, 7, 8, 12, 13, and A0–A5 (= 14–19).
//   • A6/A7 can't be digital pins. D0/D1 are the USB serial port.
// Open Serial Monitor at 115200 after uploading: the sketch checks the pin
// plan and warns about clashes.
//
// This default: welder + campfire + traffic light, each with its own
// trigger input on A0, A1, A2.
#pragma once

// ---------------------------------------------------------- 🔥 Welder
#define USE_WELDER          1
#define WELDER_PINS         9, 10, 11            // white, blue, red (PWM)
#define WELDER_START        WHILE_TRIGGERED
#define WELDER_TRIGGER      A0, ACTIVE_HIGH, 0   // pin, polarity, run-on ms

// -------------------------------------------------------- 🔥 Campfire
#define USE_CAMPFIRE        1
#define CAMPFIRE_PINS       3, 5, 6              // flame 1, flame 2, ember (PWM)
#define CAMPFIRE_START      TOGGLE
#define CAMPFIRE_TRIGGER    A1, ACTIVE_LOW, 0    // a button to GND

// ---------------------------------------------------------- 📺 TV glow
#define USE_TV_GLOW         0
#define TV_GLOW_PINS        9, 10, 11            // white, blue, red (PWM)
#define TV_GLOW_START       POWER_UP
#define TV_GLOW_TRIGGER     A3, ACTIVE_HIGH, 0

// -------------------------------------------------------- ⛈️ Lightning
#define USE_LIGHTNING       0
#define LIGHTNING_PINS      9, 10, NO_PIN        // flash A, flash B, thunder out
#define LIGHTNING_START     POWER_UP
#define LIGHTNING_TRIGGER   A3, ACTIVE_HIGH, 0

// ---------------------------------------------------- 🚦 Crossing flashers
#define USE_CROSSING        0
#define CROSSING_PINS       5, 6, 2, 3, 4, NO_PIN   // lamp L, lamp R, sensor A, sensor B, button, active out
#define CROSSING_FIRE       0                    // 1 = also fire it from the trigger below
#define CROSSING_TRIGGER    A3, ACTIVE_HIGH, 0

// ------------------------------------------- 🚧 Crossing gates (needs crossing)
#define USE_GATES           0
#define GATES_PINS          9, 10, 7             // gate 1, gate 2, bell out

// ------------------------------------------------------ 🚥 Traffic light
#define USE_TRAFFIC_LIGHT   1
#define TRAFFIC_LIGHT_PINS  2, 4, 7, 8, 12, 13   // A red, yellow, green, B red, yellow, green
#define TRAFFIC_LIGHT_START WHILE_TRIGGERED      // off = night flash
#define TRAFFIC_LIGHT_TRIGGER A2, ACTIVE_HIGH, 0

// ---------------------------------------------------------- 🪧 Neon sign
#define USE_NEON            0
#define NEON_PINS           9, 10, 11, NO_PIN    // up to 4 segments (PWM)
#define NEON_PATTERN        NEON_SPELL           // NEON_STEADY, NEON_SPELL, NEON_CHASE, NEON_BLINK
#define NEON_SOURCE         SOURCE_TRIGGER       // SOURCE_TRIGGER, or SOURCE_NIGHT (on at dusk)
#define NEON_START          POWER_UP
#define NEON_TRIGGER        A3, ACTIVE_HIGH, 0

// -------------------------------------------------------- 💡 Fluorescent
#define USE_FLUORESCENT     0
#define FLUORESCENT_PINS    9, 10, 11, NO_PIN    // up to 4 tubes
#define FLUORESCENT_START   TOGGLE
#define FLUORESCENT_TRIGGER A3, ACTIVE_LOW, 0

// ------------------------------------------------------------ 🚨 Beacon
#define USE_BEACON          0
#define BEACON_PINS         9, NO_PIN            // lamp A, lamp B (PWM)
#define BEACON_TYPE         BEACON_ROTATING      // BEACON_ROTATING, BEACON_AIRPORT, BEACON_TOWER
#define BEACON_START        POWER_UP
#define BEACON_TRIGGER      A3, ACTIVE_HIGH, 0

// ------------------------------------------------- 🔔 Sound (DFPlayer Mini)
#define USE_SOUND           0
#define SOUND_PINS          A4, NO_PIN, A5       // TX → player RX, BUSY, RX ← player TX
#define SOUND_MODE          SOUND_LOOP           // SOUND_LOOP, SOUND_EACH_TRIGGER, SOUND_RANDOM
#define SOUND_TRACKS        1, 1                 // first, last (mp3/0001.mp3…)
#define SOUND_VOLUME        20                   // 0–30
#define SOUND_SOURCE        SOURCE_TRIGGER       // SOURCE_TRIGGER, SOURCE_LIGHTNING (thunder),
                                                 //   SOURCE_GATES_BELL, SOURCE_CROSSING
#define SOUND_START         WHILE_TRIGGERED
#define SOUND_TRIGGER       A3, ACTIVE_HIGH, 0

// ------------------------------------------------------- 🌅 Day-night
#define USE_DAY_NIGHT       0
#define DAY_NIGHT_PINS      9, 10, 11, NO_PIN    // day, warm, night (PWM), night out
#define DAY_NIGHT_START     POWER_UP
#define DAY_NIGHT_TRIGGER   A3, ACTIVE_HIGH, 0
