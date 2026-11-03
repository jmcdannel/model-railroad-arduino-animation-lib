// 📦 Prebuilt installer firmwares. "Install to Nano" works when the effects
// you picked are exactly one of these sets; anything else gets the
// copy-and-paste commands instead. Every bundle has to fit in a Nano, and
// CI compiles them all.
import { NO_PIN } from './effects.js'

export const BUNDLES = [
  // one effect each
  { id: 'day-night', title: 'Day-night lighting', effects: ['day-night'] },
  { id: 'welder', title: 'Arc welder', effects: ['welder'] },
  { id: 'campfire', title: 'Campfire', effects: ['campfire'] },
  { id: 'tv-glow', title: 'TV glow', effects: ['tv-glow'] },
  { id: 'lightning', title: 'Lightning storm', effects: ['lightning'] },
  { id: 'crossing', title: 'Grade crossing', effects: ['crossing'] },
  { id: 'crossing-gates', title: 'Crossing with gates', effects: ['crossing-gates'] },
  { id: 'traffic-light', title: 'Traffic light', effects: ['traffic-light'] },
  { id: 'neon-sign', title: 'Neon sign', effects: ['neon-sign'] },
  { id: 'fluorescent', title: 'Fluorescent startup', effects: ['fluorescent'] },
  { id: 'beacon', title: 'Rotating beacon', effects: ['beacon'] },
  { id: 'sound', title: 'Sound (DFPlayer)', effects: ['sound'] },
  // themed combinations, offered as ready-made sets on step 1;
  // links: which effect switches which, so a set works with no trigger wiring;
  // pins: overrides so the set fits the Nano's six fading pins
  { id: 'storm', title: 'Storm with thunder', effects: ['lightning', 'sound'], links: { sound: 'thunder' },
    blurb: 'Lightning flickers behind the backdrop and thunder rolls after each strike.' },
  { id: 'crossing-bell', title: 'Crossing gates with bell', effects: ['crossing-gates', 'sound'], links: { sound: 'bell' },
    blurb: 'Lights flash, the gates come down and the bell rings until they lift.' },
  // (no TV glow here: day-night and the neon sign already fill all six fading pins)
  { id: 'night-town', title: 'Town at night', effects: ['day-night', 'neon-sign'], links: { 'neon-sign': 'night' },
    blurb: 'The room fades from day to dusk to moonlight, and the neon sign lights up as night falls.' },
  // (two fluorescent tubes, and no Lamp B: a rotating beacon never lights it)
  { id: 'industry', title: 'Industry', effects: ['welder', 'fluorescent', 'beacon'],
    pins: { fluorescent: { Fluorescent: { tube2: NO_PIN } }, beacon: { Beacon: { pinB: NO_PIN } } },
    blurb: 'A welder at work in the shop, two fluorescent tubes striking up and a beacon turning on the roof.' },
]

// the bundle for exactly this set of effects, if there is one
export function bundleFor(ids) {
  const key = [...ids].sort().join(',')
  return BUNDLES.find((b) => [...b.effects].sort().join(',') === key) || null
}
