// 🔧 The kit sheet: lettered parts, wiring steps, tests.
import { test } from 'node:test'
import assert from 'node:assert/strict'
import { buildKit, partsSummary, TESTS } from '../web/js/wiring.js'
import { defaultEntry } from '../web/js/generate.js'
import { EFFECTS, NO_PIN } from '../web/js/effects.js'

test('the arc welder letters its parts like a kit: LEDs, then resistors, then wire', () => {
  const kit = buildKit([defaultEntry('welder')])
  assert.deepEqual(kit.parts.map((p) => `${p.key} ${p.name}`), [
    'A White LED, 3 mm', 'B Blue LED, 3 mm', 'C Red LED, 3 mm',
    'D 220 Ω resistor', 'E 150 Ω resistor', 'F Hookup wire',
  ])
  assert.equal(kit.parts.find((p) => p.key === 'D').qty, 2, 'two 220 Ω resistors merge into one lettered part')
  assert.deepEqual(kit.sections[0].steps.map((s) => s.title), ['White LED', 'Blue LED', 'Red LED, the cooling glow'])
})

test('every effect has wiring steps, a parts summary and test checks', () => {
  for (const fx of EFFECTS) {
    const kit = buildKit([defaultEntry(fx.id)])
    assert.ok(kit.sections[0].steps.length > 0, `${fx.id} has wiring steps`)
    assert.ok(partsSummary(fx.id).length > 0, `${fx.id} has a parts summary`)
    assert.ok(TESTS[fx.id]?.length > 0, `${fx.id} has test checks`)
    for (const s of kit.sections[0].steps) assert.ok(!/Pin$|^[a-z]/.test(s.title), `${fx.id}: readable step title "${s.title}"`)
  }
})

test('unused pins get no wiring step, and the trigger gets one', () => {
  const e = defaultEntry('lightning')
  e.pins.Lightning.flashBPin = NO_PIN
  e.pins.Lightning.thunderPin = NO_PIN
  e.start = { mode: 'TOGGLE', pin: 14, activeLow: true, runOnMs: 0 }
  const kit = buildKit([e])
  assert.deepEqual(kit.sections[0].steps.map((s) => s.title), ['Flash LED A', 'On/off button'])
  assert.ok(kit.parts.some((p) => p.kind === 'button'))
})

test('a rotating beacon ignores its second lamp; an airport beacon wires white and green', () => {
  const rot = buildKit([defaultEntry('beacon')])
  assert.equal(rot.sections[0].steps.length, 1)
  const air = defaultEntry('beacon')
  air.cfg.Beacon.type = 'BEACON_AIRPORT'
  assert.deepEqual(buildKit([air]).parts.filter((p) => p.kind === 'led').map((p) => p.name), ['White LED, 3 mm', 'Green LED, 3 mm'])
})

test('letters continue across a multi-effect board and parts merge', () => {
  const kit = buildKit([defaultEntry('welder'), defaultEntry('tv-glow')])
  const white = kit.parts.find((p) => p.name.startsWith('White LED'))
  assert.equal(white.qty, 2, 'one white LED from each effect')
  assert.equal(new Set(kit.parts.map((p) => p.key)).size, kit.parts.length, 'every part has its own letter')
})

test('choosing a button in When it runs never re-letters parts already wired', () => {
  const e = defaultEntry('welder')
  const before = buildKit([e]).parts.map((p) => `${p.key} ${p.sku}`)
  e.start = { mode: 'TOGGLE', pin: 14, activeLow: true, runOnMs: 0 }
  const after = buildKit([e]).parts.map((p) => `${p.key} ${p.sku}`)
  assert.deepEqual(after.slice(0, before.length), before)
  assert.equal(after.at(-1), 'G button')
})

test('LED steps name the resistor value and send the short leg to the ground wire', () => {
  const kit = buildKit([defaultEntry('welder')])
  const t = kit.sections[0].steps[0].tokens
  assert.ok(t.includes('220 Ω resistor →'))
  assert.ok(t.some((x) => x.groundWire), 'points at the ground wire and its step')
})

test('power: USB by default, a lettered supply on 5V or VIN, lettered last', () => {
  const welder = [defaultEntry('welder')]
  const usb = buildKit(welder)
  assert.equal(usb.power.how, 'usb')
  assert.equal(usb.power.pin, null)
  const five = buildKit(welder, { power: '5v' })
  assert.equal(five.power.pin, '5V')
  assert.equal(five.parts.at(-1).sku, 'psu:5v', 'the supply is lettered last')
  assert.deepEqual(five.parts.slice(0, -1).map((p) => p.key + p.sku), usb.parts.map((p) => p.key + p.sku), 'other letters stay put')
  assert.ok(five.power.tokens.some((t) => t.fixed === '5V'))
  const vin = buildKit(welder, { power: 'vin' })
  assert.equal(vin.power.pin, 'VIN')
  assert.equal(vin.parts.at(-1).sku, 'psu:9v')
  assert.equal(vin.power.warn, null)
})

test('VIN reuses the strips’ 12 V supply, and warns when servos or the DFPlayer need 5 V', () => {
  const dn = buildKit([defaultEntry('day-night')], { power: 'vin' })
  assert.ok(!dn.parts.some((p) => p.sku === 'psu:9v'), 'no second supply')
  assert.ok(dn.power.parts.includes('supply'))
  assert.ok(buildKit([defaultEntry('crossing-gates')], { power: 'vin' }).power.warn)
  assert.ok(!buildKit([defaultEntry('crossing-gates')], { power: '5v' }).power.warn)
})
