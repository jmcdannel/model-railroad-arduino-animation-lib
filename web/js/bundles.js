// 📦 Prebuilt installer firmwares. "Install to Nano" works when the effects
// you picked are exactly one of these sets; anything else gets the
// copy-and-paste commands instead. Every bundle has to fit in a Nano, and
// CI compiles them all.
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
  // themed combinations
  { id: 'storm', title: 'Storm with thunder', effects: ['lightning', 'sound'] },
  { id: 'crossing-bell', title: 'Crossing gates with bell', effects: ['crossing-gates', 'sound'] },
  { id: 'night-town', title: 'Town at night', effects: ['day-night', 'neon-sign', 'tv-glow'] },
  { id: 'industry', title: 'Industry', effects: ['welder', 'fluorescent', 'beacon'] },
]

// the bundle for exactly this set of effects, if there is one
export function bundleFor(ids) {
  const key = [...ids].sort().join(',')
  return BUNDLES.find((b) => [...b.effects].sort().join(',') === key) || null
}
