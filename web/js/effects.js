// 🎬 The effect catalog: what the configurator offers and how each effect is
// wired into a sketch. Settings, types and defaults come from schema.js
// (generated from src/), so this file only holds what the headers can't say.
//
// Order matters: it's the order effects update in loop(), so an effect that
// drives another (day-night → neon, lightning → sound) comes first.

// pin roles per class: [label, needs PWM to fade, optional (may be NO_PIN)]
export const PINS = {
  DayNight:      { dayPin: ['Daylight white strip', true, true], warmPin: ['Warm white strip', true, true], nightPin: ['Blue (moonlight) strip', true, true], nightOutPin: ['Night output', false, true] },
  Welder:        { whitePin: ['White LED', true], bluePin: ['Blue LED', true], redPin: ['Red LED (glow)', true] },
  Campfire:      { flame1Pin: ['Flame 1 (yellow)', true], flame2Pin: ['Flame 2 (orange)', true], emberPin: ['Ember (red)', true] },
  TvGlow:        { whitePin: ['White LED', true], bluePin: ['Blue LED', true], redPin: ['Red LED', true] },
  Lightning:     { flashAPin: ['Flash LED A', true], flashBPin: ['Flash LED B', true, true], thunderPin: ['Thunder output', false, true] },
  Crossing:      { lampLPin: ['Left lamp', true], lampRPin: ['Right lamp', true], sensorAPin: ['IR sensor A', false], sensorBPin: ['IR sensor B', false, true], buttonPin: ['Button', false, true], activeOutPin: ['Active output', false, true] },
  CrossingGates: { gate1Pin: ['Gate 1 servo', false], gate2Pin: ['Gate 2 servo', false, true], bellPin: ['Bell output', false, true] },
  TrafficLight:  { aRed: ['A red', false], aYellow: ['A yellow', false], aGreen: ['A green', false], bRed: ['B red', false], bYellow: ['B yellow', false], bGreen: ['B green', false] },
  Neon:          { seg0: ['Segment 1', true], seg1: ['Segment 2', true, true], seg2: ['Segment 3', true, true], seg3: ['Segment 4', true, true] },
  Fluorescent:   { tube0: ['Tube 1', true], tube1: ['Tube 2', true, true], tube2: ['Tube 3', true, true], tube3: ['Tube 4', true, true] },
  Beacon:        { pinA: ['Lamp A', true], pinB: ['Lamp B (airport green / tower 2nd tier)', true, true] },
  Sound:         { txPin: ['TX → DFPlayer RX (via 1 kΩ)', false], busyPin: ['DFPlayer BUSY', false, true], rxPin: ['RX ← DFPlayer TX (unused, but reserved)', false] },
}

// settings shown up front; the rest go under "More settings"
export const KEY_FIELDS = {
  DayNight: ['dayMs', 'duskMs', 'nightMs', 'dawnMs', 'onMeansDay', 'nightLevel'],
  Welder: ['idleMin', 'idleMax', 'jobMin', 'jobMax', 'coolTauMs', 'redMax'],
  Campfire: ['flameEase', 'gustBoost', 'lullDrop', 'lullPercent'],
  TvGlow: ['sceneMin', 'sceneMax', 'whiteMax', 'blueMax', 'flashPercent', 'surfPercent'],
  Lightning: ['gapActiveMin', 'gapActiveMax', 'gapQuietMin', 'gapQuietMax', 'flashesMax'],
  Crossing: ['flashesPerMin', 'incandescentFade', 'holdMs', 'buttonRunMs', 'sensorActiveLow'],
  CrossingGates: ['upDeg', 'downDeg', 'mirrorGate2', 'delayMs', 'lowerMs', 'raiseMs', 'bounceDeg', 'bellUntilDown'],
  TrafficLight: ['greenAMs', 'greenBMs', 'yellowMs', 'allRedMs', 'nightFlash', 'offNightFlash'],
  Neon: ['pattern', 'stepMs', 'flashes', 'brokenSegment', 'level'],
  Fluorescent: ['staggerMax', 'attemptsMax', 'badTube', 'badNeverStrikes'],
  Beacon: ['type', 'rpm', 'reflectors', 'sharpness', 'ambient', 'towerFlashesPerMin'],
  Sound: ['mode', 'firstTrack', 'lastTrack', 'volume', 'fadeOutMs', 'delayMin', 'delayMax', 'distanceDrop', 'gapMin', 'gapMax'],
}

// kind: 'standard' = update(now, trigger) · 'crossing' = always armed, trigger fires it
// pins / cfg: where the example sketch differs from the class defaults
export const EFFECTS = [
  { id: 'day-night', title: 'Day-night lighting', emoji: '🌅', blurb: 'Dawn, day, dusk and night on LED strips, with a night output.',
    objects: [{ cls: 'DayNight', v: 'dayNight' }], kind: 'standard', pins: { DayNight: { nightOutPin: 2 } }, offText: 'fades out and pauses (or sunset → night)' },
  { id: 'welder', title: 'Arc welder', emoji: '🔥', blurb: 'Blue/white arc flicker, then a red glow that cools.',
    objects: [{ cls: 'Welder', v: 'welder' }], kind: 'standard', offText: 'no new jobs; the glow cools down' },
  { id: 'campfire', title: 'Campfire', emoji: '🔥', blurb: 'Two flames and an ember, with gusts and lulls.',
    objects: [{ cls: 'Campfire', v: 'campfire' }], kind: 'standard', offText: 'fades out' },
  { id: 'tv-glow', title: 'TV glow', emoji: '📺', blurb: 'Scene cuts, flashes and channel surfing through a window.',
    objects: [{ cls: 'TvGlow', v: 'tvGlow' }], kind: 'standard', offText: 'fades to black' },
  { id: 'lightning', title: 'Lightning storm', emoji: '⛈️', blurb: 'Strike clusters, active and quiet spells, thunder output.',
    objects: [{ cls: 'Lightning', v: 'lightning' }], kind: 'standard', offText: 'dark' },
  { id: 'crossing', title: 'Grade crossing', emoji: '🚦', blurb: 'Flashers at 50 per minute from IR sensors and a button.',
    objects: [{ cls: 'Crossing', v: 'crossing' }], kind: 'crossing', excludes: ['crossing-gates'] },
  { id: 'crossing-gates', title: 'Crossing with gates', emoji: '🚧', blurb: 'The crossing plus servo gates and a bell output.',
    objects: [{ cls: 'Crossing', v: 'crossing' }, { cls: 'CrossingGates', v: 'gates', args: ['crossing'] }], kind: 'crossing',
    pins: { Crossing: { lampLPin: 5, lampRPin: 6, activeOutPin: 255 } },
    excludes: ['crossing'], libraries: ['Servo'], usesServoTimer: true },
  { id: 'traffic-light', title: 'Traffic light', emoji: '🚥', blurb: 'One head or a synced four-way, with night flash.',
    objects: [{ cls: 'TrafficLight', v: 'trafficLight' }], kind: 'standard', offText: 'night flash (or dark)' },
  { id: 'neon-sign', title: 'Neon sign', emoji: '🪧', blurb: 'Buzzy strike-up, spell-and-flash, a failing letter.',
    objects: [{ cls: 'Neon', v: 'neon' }], kind: 'standard', pins: { Neon: { seg1: 10, seg2: 11 } }, cfg: { Neon: { pattern: 'NEON_SPELL' } }, offText: 'tubes go out',
    sources: [{ id: 'night', label: 'On at dusk (from day-night)', needs: 'day-night', expr: 'dayNight.isNight()' }] },
  { id: 'fluorescent', title: 'Fluorescent startup', emoji: '💡', blurb: 'Tubes that glow, blink and strike, one maybe worn out.',
    objects: [{ cls: 'Fluorescent', v: 'fluorescent' }], kind: 'standard', pins: { Fluorescent: { tube1: 10, tube2: 11 } }, offText: 'all tubes out' },
  { id: 'beacon', title: 'Rotating beacon', emoji: '🚨', blurb: 'Rotator, airport white/green, or tower light.',
    objects: [{ cls: 'Beacon', v: 'beacon' }], kind: 'standard', pins: { Beacon: { pinB: 10 } }, offText: 'lamp fades, motor coasts down' },
  { id: 'sound', title: 'Sound (DFPlayer)', emoji: '🔔', blurb: 'Crossing bell, thunder or ambient clips from an SD card.',
    objects: [{ cls: 'Sound', v: 'sound' }], kind: 'standard', pins: { Sound: { busyPin: 8 } }, offText: 'loop fades out',
    sources: [
      { id: 'thunder', label: 'Thunder after each strike (from lightning)', needs: 'lightning', expr: 'lightning.thunder()' },
      { id: 'bell', label: 'Bell with the gates (from crossing gates)', needs: 'crossing-gates', expr: 'gates.bell()' },
      { id: 'crossing', label: 'While the crossing flashes', needs: ['crossing', 'crossing-gates'], expr: 'crossing.flashing()' },
    ] },
]

export const byId = Object.fromEntries(EFFECTS.map((e) => [e.id, e]))
export const PWM_PINS = [3, 5, 6, 9, 10, 11]
export const NO_PIN = 255

// Nano pins a user can pick: D2–D13 and A0–A5 (14–19). D0/D1 are USB serial.
export const PIN_CHOICES = [2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19]
export const pinName = (p) => (p === NO_PIN ? 'not used' : p >= 14 ? `A${p - 14}` : `D${p}`)
