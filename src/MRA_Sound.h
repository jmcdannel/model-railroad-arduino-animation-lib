// 🔔 Sound from a DFPlayer Mini MP3 module.
//
//   SOUND_LOOP          plays a clip over and over while on: a crossing bell,
//                       a running engine, crickets. Fades out when switched off.
//   SOUND_EACH_TRIGGER  one clip every time it's switched on, after an optional
//                       random delay: thunder after a lightning strike (longer
//                       delay = farther away = quieter), a horn on a button.
//   SOUND_RANDOM        while on, a random clip every so often: distant
//                       thunder rumbles, dogs, a factory whistle.
//
// SD card: a folder named "mp3" with files 0001.mp3, 0002.mp3, … (the number
// is the track; anything after it in the name is ignored, e.g.
// 0001_bell.mp3). cfg.firstTrack…cfg.lastTrack is the set it picks from.
//
// Wiring: Nano TX pin → 1 kΩ resistor → DFPlayer RX (the resistor keeps 5 V
// signals from making the player hiss). DFPlayer VCC → 5V, GND → GND,
// SPK_1/SPK_2 → an 8 Ω speaker up to 3 W. BUSY → a Nano pin (optional) lets
// the sketch know when a clip has finished.
//
// Uses SoftwareSerial, which comes with the Arduino AVR core.
#pragma once
#include <SoftwareSerial.h>
#include "MRA_Trigger.h"

namespace mra {

enum SoundMode : uint8_t { SOUND_LOOP, SOUND_EACH_TRIGGER, SOUND_RANDOM };

class Sound {
 public:
  // 🎛️ Tuning. Set fields before begin().
  struct Config {
    SoundMode mode = SOUND_LOOP;
    uint16_t firstTrack = 1, lastTrack = 1;   // mp3/0001.mp3 … picks at random between
    uint8_t  volume = 20;                     // 0–30
    uint16_t bootMs = 1500;                   // DFPlayer start-up time after power-on
    uint16_t fadeOutMs = 500;                 // switched off: fade, then stop (0 = cut)
    uint16_t delayMin = 0, delayMax = 0;      // EACH_TRIGGER / RANDOM: wait before the
                                              //   first clip (thunder: 300–4000)
    uint8_t  distanceDrop = 0;                // EACH_TRIGGER: longest delay plays this
                                              //   much quieter (0–30)
    uint32_t gapMin = 20000, gapMax = 90000;  // RANDOM: between clips
    bool     interrupt = true;                // EACH_TRIGGER: a new trigger cuts off a clip
                                              //   still playing (false needs the BUSY pin)
  } cfg;

  // rxPin is the DFPlayer's TX → Nano pin. Nothing is read from it, so it
  // can stay unwired, but it must be a real, otherwise unused pin.
  Sound(uint8_t txPin = 10, uint8_t busyPin = NO_PIN, uint8_t rxPin = 11)
    : serial_(rxPin, txPin), busyPin_(busyPin) {}

  void begin() {
    serial_.begin(9600);
    if (busyPin_ != NO_PIN) pinMode(busyPin_, INPUT_PULLUP);
    sendAt_ = millis() + cfg.bootMs;      // nothing reaches the player before it's ready
    queue(CMD_VOLUME, cfg.volume);
  }

  // true while a clip plays (needs the BUSY pin)
  bool playing() const { return busyPin_ != NO_PIN && digitalRead(busyPin_) == LOW; }

  void update(uint32_t now, bool on) {
    bool started = on && !wasOn_, stopped = !on && wasOn_;
    wasOn_ = on;

    switch (cfg.mode) {
      case SOUND_LOOP:
        if (started) { fading_ = false; play(cfg.volume, true); }
        if (stopped) stop(now);
        break;

      case SOUND_EACH_TRIGGER:
        if (started) {
          uint16_t wait = between(cfg.delayMin, cfg.delayMax);
          uint8_t drop = cfg.delayMax > cfg.delayMin
                       ? (uint32_t)cfg.distanceDrop * (wait - cfg.delayMin) / (cfg.delayMax - cfg.delayMin) : 0;
          pendingVol_ = cfg.volume > drop ? cfg.volume - drop : 0;
          playAt_ = now + wait;
          pending_ = true;
        }
        if (pending_ && due(now, playAt_)) {
          pending_ = false;
          if (cfg.interrupt || !playing()) { fading_ = false; play(pendingVol_, false); }
        }
        break;

      case SOUND_RANDOM:
        if (started) { playAt_ = now + between(cfg.delayMin, cfg.delayMax); fading_ = false; }
        if (stopped) stop(now);
        if (on && due(now, playAt_) && !playing()) {
          play(cfg.volume, false);
          playAt_ = now + between(cfg.gapMin, cfg.gapMax);
        }
        break;
    }

    // 🔉 fade out: step the volume down, then stop
    if (fading_ && due(now, fadeAt_)) {
      if (fadeVol_ > 0) {                  // one step per command slot, sized to finish on time
        uint8_t dec = (uint32_t)cfg.volume * GAP_MS / cfg.fadeOutMs;
        if (dec < 1) dec = 1;
        fadeVol_ = fadeVol_ > dec ? fadeVol_ - dec : 0;
        queue(CMD_VOLUME, fadeVol_);
        fadeAt_ = now + GAP_MS;
      } else {
        queue(CMD_STOP, 0);
        fading_ = false;
      }
    }

    // 📤 send one queued command at a time; the player needs a breather between them
    if (count_ && due(now, sendAt_)) {
      Cmd &c = q_[head_];
      send(c.cmd, c.param);
      head_ = (head_ + 1) % QUEUE;
      count_--;
      sendAt_ = now + GAP_MS;
    }
  }

 private:
  static const uint8_t QUEUE = 6, GAP_MS = 100;
  enum : uint8_t { CMD_VOLUME = 0x06, CMD_STOP = 0x16, CMD_PLAY_MP3 = 0x12, CMD_LOOP_CURRENT = 0x19 };
  struct Cmd { uint8_t cmd; uint16_t param; };

  void play(uint8_t vol, bool loop) {
    count_ = 0;                            // whatever was queued is stale now
    queue(CMD_VOLUME, vol);
    queue(CMD_PLAY_MP3, pickTrack());
    if (loop) queue(CMD_LOOP_CURRENT, 0);  // 0 = repeat this track
  }

  void stop(uint32_t now) {
    if (cfg.fadeOutMs == 0) { count_ = 0; queue(CMD_STOP, 0); return; }
    fading_ = true;
    fadeVol_ = cfg.volume;
    fadeAt_ = now;
  }

  uint16_t pickTrack() {
    uint16_t t = between(cfg.firstTrack, cfg.lastTrack);
    if (t == lastTrack_ && cfg.lastTrack > cfg.firstTrack)        // no instant repeats
      t = (t < cfg.lastTrack) ? t + 1 : cfg.firstTrack;
    return lastTrack_ = t;
  }

  void queue(uint8_t cmd, uint16_t param) {
    if (count_ == QUEUE) return;
    q_[(head_ + count_) % QUEUE] = { cmd, param };
    count_++;
  }

  // DFPlayer frame: 7E FF 06 cmd 00 paramH paramL checksumH checksumL EF
  void send(uint8_t cmd, uint16_t param) {
    uint8_t f[10] = { 0x7E, 0xFF, 0x06, cmd, 0x00, (uint8_t)(param >> 8), (uint8_t)param, 0, 0, 0xEF };
    uint16_t sum = 0;
    for (uint8_t i = 1; i < 7; i++) sum += f[i];
    sum = -sum;
    f[7] = sum >> 8;
    f[8] = sum;
    serial_.write(f, sizeof(f));
  }

  SoftwareSerial serial_;
  uint8_t  busyPin_;
  Cmd      q_[QUEUE];
  uint8_t  head_ = 0, count_ = 0, fadeVol_ = 0, pendingVol_ = 0;
  uint16_t lastTrack_ = 0;
  uint32_t sendAt_ = 0, playAt_ = 0, fadeAt_ = 0;
  bool     wasOn_ = false, fading_ = false, pending_ = false;
};

}  // namespace mra
