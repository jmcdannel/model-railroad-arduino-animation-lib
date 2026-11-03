// 🔧 The kit sheet's knowledge: what part goes on every pin, how it's wired,
// the lettered parts list, and how to test it. Taken from each effect's README.
//
// A wiring step is a list of tokens the page renders inline:
//   'text'                  plain words
//   { pin: [cls, name] }    a pin the user can change (a select in the step)
//   { fixed: 'GND' }        a fixed Nano pin: GND, 5V
//   { part: sku }           a lettered part (the letter is assigned per board)
import schema from './schema.js'
import { byId, NO_PIN, PINS } from './effects.js'
import { ordered, availableSources, defaultEntry } from './generate.js'

// ------------------------------------------------------------ parts 🧩
// sku → how the part is named and drawn
const COLOR_NAMES = { white: 'White', 'cool-white': 'Cool-white', blue: 'Blue', red: 'Red', yellow: 'Yellow', orange: 'Orange', amber: 'Amber', green: 'Green', neon: 'Red, pink or blue' }
export const LED_FILL = { white: '#ffffff', 'cool-white': '#eef4ff', blue: '#d6e1f3', red: '#f5d3cd', yellow: '#fbefb5', orange: '#fbdcbf', amber: '#fbe2a6', green: '#d3ecd9', neon: '#f8d3e6' }

export function partInfo(sku) {
  const [kind, a] = sku.split(':')
  switch (kind) {
    case 'led': return { kind, name: `${COLOR_NAMES[a]} LED, 3 mm`, color: a }
    case 'res': return { kind, name: `${Number(a) >= 1000 ? `${a / 1000} kΩ` : `${a} Ω`} resistor`, ohms: Number(a) }
    case 'ir': return { kind, name: 'IR obstacle sensor (FC-51)' }
    case 'button': return { kind, name: 'Push button' }
    case 'servo': return { kind, name: 'Micro servo (SG90)' }
    case 'strip': return { kind, name: `${a === 'day' ? 'Daylight-white' : a === 'warm' ? 'Warm-white' : 'Blue'} LED strip, 12 V`, color: a }
    case 'mosfet': return { kind, name: 'Logic-level MOSFET module' }
    case 'supply': return { kind, name: '12 V power supply for the strips' }
    case 'dfplayer': return { kind, name: 'DFPlayer Mini MP3 module' }
    case 'speaker': return { kind, name: 'Speaker, 8 Ω, up to 3 W' }
    case 'sd': return { kind, name: 'microSD card with your sounds' }
    case 'wire': return { kind, name: 'Hookup wire' }
    default: return { kind, name: sku }
  }
}

// what sits on each pin. Return null when the pin does nothing in this setup.
const led = (color, ohms = 220) => ({ type: 'led', color, ohms })
const PIN_PARTS = {
  Welder: { whitePin: () => led('white'), bluePin: () => led('blue'), redPin: () => led('red', 150) },
  Campfire: { flame1Pin: () => led('yellow'), flame2Pin: () => led('orange'), emberPin: () => led('red') },
  TvGlow: { whitePin: () => led('white'), bluePin: () => led('blue'), redPin: () => led('red') },
  Lightning: { flashAPin: () => led('white'), flashBPin: () => led('white'), thunderPin: () => ({ type: 'signal', what: 'thunder', to: 'D12 of a sound board (THUNDER preset)' }) },
  Crossing: {
    lampLPin: () => led('red'), lampRPin: () => led('red'),
    sensorAPin: () => ({ type: 'ir', label: 'IR sensor A, on one approach' }), sensorBPin: () => ({ type: 'ir', label: 'IR sensor B, on the other approach' }),
    buttonPin: () => ({ type: 'button', label: 'Push button for visitors' }),
    activeOutPin: () => ({ type: 'signal', what: 'active', to: 'a bell module or another board (HIGH while flashing)' }),
  },
  CrossingGates: {
    gate1Pin: () => ({ type: 'servo', label: 'Gate 1 servo' }), gate2Pin: () => ({ type: 'servo', label: 'Gate 2 servo (other side of the road)' }),
    bellPin: () => ({ type: 'signal', what: 'bell', to: 'D12 of a sound board (BELL preset)' }),
  },
  TrafficLight: { aRed: () => led('red'), aYellow: () => led('yellow'), aGreen: () => led('green'), bRed: () => led('red'), bYellow: () => led('yellow'), bGreen: () => led('green') },
  Neon: { seg0: () => led('neon'), seg1: () => led('neon'), seg2: () => led('neon'), seg3: () => led('neon') },
  Fluorescent: { tube0: () => led('cool-white'), tube1: () => led('cool-white'), tube2: () => led('cool-white'), tube3: () => led('cool-white') },
  Beacon: {
    pinA: (e) => led({ BEACON_AIRPORT: 'white', BEACON_TOWER: 'red' }[e.cfg.Beacon.type] || 'amber'),
    pinB: (e) => ({ BEACON_AIRPORT: led('green'), BEACON_TOWER: led('red') }[e.cfg.Beacon.type] || null),
  },
  Sound: {
    txPin: () => ({ type: 'df-rx' }), busyPin: () => ({ type: 'df-busy' }), rxPin: () => ({ type: 'df-tx' }),
  },
  DayNight: {
    dayPin: () => ({ type: 'strip', color: 'day' }), warmPin: () => ({ type: 'strip', color: 'warm' }), nightPin: () => ({ type: 'strip', color: 'night' }),
    nightOutPin: () => ({ type: 'signal', what: 'night', to: 'D12 of another board, or a relay module for building lights' }),
  },
}

const LABELS = { Welder: { whitePin: 'White LED', bluePin: 'Blue LED', redPin: 'Red LED, the cooling glow' } }
const roleLabel = (fx, cls, pinName, spec) => LABELS[cls]?.[pinName] || spec.label || PINS[cls]?.[pinName]?.[0] || {
  led: () => `${COLOR_NAMES[spec.color]} LED`,
  strip: () => partInfo(`strip:${spec.color}`).name.replace(', 12 V', ''),
}[spec.type]?.() || pinName

// ------------------------------------------------------ the board 🔩
// Everything the sheet needs for a configuration: lettered parts, wiring
// steps per effect, and a few extras (5V used, GND used).
export function buildKit(effects) {
  const entries = ordered(effects)
  const counts = new Map()                     // sku → quantity, in order of first use
  const add = (sku, n = 1) => counts.set(sku, (counts.get(sku) || 0) + n)
  const sections = []
  let uses5V = false

  for (const e of entries) {
    const fx = byId[e.id]
    const steps = []
    const ids = entries.map((x) => x.id)
    for (const o of fx.objects) {
      const cls = schema.classes[o.cls]
      let soundPowerDone = false
      for (const p of cls.pins) {
        const pin = e.pins[o.cls][p.name]
        if (pin === NO_PIN) continue
        const spec = PIN_PARTS[o.cls]?.[p.name]?.(e)
        if (!spec) continue
        const P = { pin: [o.cls, p.name] }
        const title = roleLabel(fx, o.cls, p.name, spec)
        if (spec.type === 'led') {
          const L = `led:${spec.color}`, R = `res:${spec.ohms}`
          add(L); add(R)
          steps.push({ title, tokens: ['Nano', P, '→', { part: R }, `${partInfo(R).name} →`, { part: L }, 'long leg; short leg →', { groundWire: true }], parts: [R, L] })
        } else if (spec.type === 'ir') {
          add('ir'); uses5V = true
          steps.push({ title: spec.label, tokens: [{ part: 'ir' }, 'OUT →', P, '; VCC →', { fixed: '5V' }, '; GND →', { fixed: 'GND' }], parts: ['ir'], note: 'Mount it between the ties pointing up; turn its pot until it sees a car at 2–3 cm but not the ballast.' })
        } else if (spec.type === 'button') {
          add('button')
          steps.push({ title: spec.label, tokens: [{ part: 'button' }, 'between', P, 'and', { fixed: 'GND' }, '(no resistor needed)'], parts: ['button'] })
        } else if (spec.type === 'servo') {
          add('servo'); uses5V = true
          steps.push({ title: spec.label, tokens: [{ part: 'servo' }, 'signal (orange) →', P, '; red →', { fixed: '5V' }, '; brown →', { fixed: 'GND' }], parts: ['servo'] })
        } else if (spec.type === 'signal') {
          steps.push({ title: `${title} (optional)`, tokens: ['Nano', P, '→', spec.to, '; also join the two boards’', { fixed: 'GND' }], parts: [], optional: true })
        } else if (spec.type === 'strip') {
          const S = `strip:${spec.color}`
          add(S); add('mosfet')
          steps.push({ title, tokens: ['Nano', P, '→', { part: 'mosfet' }, 'gate (SIG); strip', { part: S }, '(−) → MOSFET OUT; strip (+) → 12 V'], parts: ['mosfet', S] })
        } else if (spec.type.startsWith('df-')) {
          if (!soundPowerDone) {
            soundPowerDone = true
            add('dfplayer'); add('speaker'); add('sd'); uses5V = true
            steps.push({ title: 'DFPlayer power and speaker', tokens: [{ part: 'dfplayer' }, 'VCC →', { fixed: '5V' }, '; GND →', { fixed: 'GND' }, '; SPK_1 and SPK_2 →', { part: 'speaker' }, '; card', { part: 'sd' }, 'in the slot'], parts: ['dfplayer', 'speaker', 'sd'] })
          }
          if (spec.type === 'df-rx') {
            add('res:1000')
            steps.push({ title: 'Sound commands', tokens: ['Nano', P, '→', { part: 'res:1000' }, '1 kΩ resistor →', { part: 'dfplayer' }, 'RX'], parts: ['res:1000', 'dfplayer'], note: 'The resistor keeps the player from hissing.' })
          } else if (spec.type === 'df-busy') {
            steps.push({ title: 'Busy signal (optional)', tokens: [{ part: 'dfplayer' }, 'BUSY →', P], parts: ['dfplayer'], optional: true })
          } else {
            steps.push({ title: 'Reserved pin (leave unwired)', tokens: ['Keep', P, 'free; the sound library listens on it'], parts: [], optional: true })
          }
        }
      }
    }
    if (fx.id === 'day-night' && steps.length) add('supply')
    // 🎛️ trigger wiring, from the start mode
    const linked = e.source !== 'trigger' && availableSources(e.id, ids).some((s) => s.id === e.source)
    if (!linked && e.start.mode !== 'POWER_UP') {
      const P = { trigger: true }
      if (e.start.mode === 'TOGGLE') {
        add('button')
        steps.push({ title: 'On/off button', tokens: [{ part: 'button' }, 'between', P, 'and', { fixed: 'GND' }], parts: ['button'] })
      } else if (e.start.activeLow) {
        steps.push({ title: 'Trigger', tokens: ['A button, relay contact or IR sensor between', P, 'and', { fixed: 'GND' }], parts: [] })
      } else {
        steps.push({ title: 'Trigger', tokens: ['DEJA.js IO pin (or a switch to 5 V) →', P, '; join its', { fixed: 'GND' }, 'to the Nano’s'], parts: [] })
      }
    }
    sections.push({ id: e.id, steps })
  }
  if (counts.size) add('wire')

  // 🔤 letters like a kit's parts tree: grouped by kind, then in order of use;
  // buttons come last: going back to When it runs to add a button never re-letters wired parts
  const KIND_ORDER = ['led', 'strip', 'ir', 'servo', 'dfplayer', 'speaker', 'sd', 'res', 'mosfet', 'supply', 'wire', 'button']
  const skus = [...counts.keys()].sort((a, b) => KIND_ORDER.indexOf(a.split(':')[0]) - KIND_ORDER.indexOf(b.split(':')[0]))
  const letters = new Map(skus.map((sku, i) => [sku, String.fromCharCode(65 + i)]))
  const parts = skus.map((sku) => ({ sku, qty: sku === 'wire' ? null : counts.get(sku), key: letters.get(sku), ...partInfo(sku) }))
  return { parts, letters, sections, uses5V }
}

// --------------------------------------------------------- testing ✅
export const TESTS = {
  'day-night': ['The strips come up to daylight white within a few seconds.', 'Shorten the times in When it runs to watch a full cycle quickly: dusk turns golden, then orange, then blue.'],
  welder: ['After about a second, white and blue flicker in bursts like an arc.', 'When a burst ends, the red LED glows and slowly fades.'],
  campfire: ['Yellow and orange flicker independently; red breathes slowly under them.', 'Every few seconds the fire flares up or settles down.'],
  'tv-glow': ['The LEDs cut between brightness and color every few seconds.', 'Now and then a bright flash, a dark moment, or rapid channel surfing.'],
  lightning: ['About 3 seconds after power-up, a cluster of 1–6 flashes.', 'Strikes then come every few seconds, with quiet spells between.'],
  crossing: ['Wave a hand over sensor A: both lamps alternate at about 50 flashes a minute.', 'Pass over sensor B and wait 3 seconds: the flashers stop. The button runs them for 20 seconds.'],
  'crossing-gates': ['Wave a hand over sensor A: flashers start, and after 3 seconds the gates lower and bounce.', 'Pass over sensor B and wait: the gates rise, then the flashers stop.'],
  'traffic-light': ['The heads start at all-red, then A goes green while B stays red.', 'Yellow, all-red, then B gets its green. A full cycle takes about 32 seconds.'],
  'neon-sign': ['Each segment buzzes and flickers as it strikes, then glows steady.', 'Your pattern runs: spell, chase or blink.'],
  fluorescent: ['Each tube waits, glows dimly at the ends, blinks once or a few times, then strikes.', 'The tubes settle to full brightness about 2 seconds after they catch.'],
  beacon: ['The lamp sweeps slowly at first, then reaches full speed.', 'Each pass swells to a bright peak and falls back to a faint glow.'],
  sound: ['About 1.5 seconds after power-up the player is ready.', 'Trigger it the way you chose in When it runs; you should hear your clip.'],
}

export const TROUBLESHOOT = [
  { q: 'An LED stays dark', a: 'Turn it around: the long leg goes toward the resistor and pin, the short leg to GND. Then check the resistor is in series, not beside the LED.' },
  { q: 'Nothing works at all', a: 'Make sure every GND wire reaches the Nano’s GND pin, and that you installed after wiring (step 4).' },
  { q: 'The page can’t find the Nano', a: 'Use a data USB cable (some only charge). Clones with a CH340 chip may need its driver on Windows and older macOS.' },
  { q: 'Install says the Nano didn’t answer', a: 'Switch the bootloader setting between new and old and try again. Close the Arduino IDE or any serial monitor first.' },
  { q: 'It runs, but not when I expect', a: 'Check the start mode in When it runs. With a trigger set to HIGH = on, an unwired trigger pin reads as on.' },
]

// a short description of an effect's parts, for the picker
export function partsSummary(id) {
  const kit = buildKit([defaultEntry(id)])
  const n = (kind) => kit.parts.filter((p) => p.kind === kind).reduce((a, p) => a + (p.qty || 0), 0)
  const bits = []
  const leds = n('led'), res = n('res')
  if (leds) bits.push(`${leds} LED${leds > 1 ? 's' : ''}`)
  if (res) bits.push(`${res} resistor${res > 1 ? 's' : ''}`)
  for (const [kind, word] of [['ir', 'IR sensor'], ['servo', 'servo'], ['strip', 'LED strip'], ['dfplayer', 'MP3 module'], ['button', 'button']]) {
    const c = n(kind)
    if (c) bits.push(`${c} ${word}${c > 1 ? 's' : ''}`)
  }
  return bits.join(', ')
}

