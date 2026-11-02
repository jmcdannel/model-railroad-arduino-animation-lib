// 🏭 Schema, settings layout/encoding, sketches and bundles.
import { test } from 'node:test'
import assert from 'node:assert/strict'
import { join, dirname } from 'node:path'
import { readFileSync } from 'node:fs'
import { fileURLToPath } from 'node:url'
import { parseHeaders, render } from '../tools/gen-schema.mjs'
import schema from '../web/js/schema.js'
import { EFFECTS, PINS, KEY_FIELDS } from '../web/js/effects.js'
import { BUNDLES, bundleFor } from '../web/js/bundles.js'
import { defaultEntry, settingsLayout, encodeSettings, generateSketch, availableSources, librariesFor } from '../web/js/generate.js'
import { parseHex, findBlock } from '../web/js/ihex.js'

const root = join(dirname(fileURLToPath(import.meta.url)), '..')

test('web/js/schema.js matches the headers in src/', () => {
  assert.equal(readFileSync(join(root, 'web/js/schema.js'), 'utf8'), render(parseHeaders(join(root, 'src'))),
    'run: node tools/gen-schema.mjs')
})

test('the catalog covers every class, pin and key setting', () => {
  for (const fx of EFFECTS) for (const o of fx.objects) {
    const cls = schema.classes[o.cls]
    assert.ok(cls, `${o.cls} is in the schema`)
    for (const p of cls.pins) assert.ok(PINS[o.cls]?.[p.name], `${o.cls}.${p.name} has a label in PINS`)
    for (const k of KEY_FIELDS[o.cls] || []) assert.ok(cls.fields.some((f) => f.name === k), `${o.cls}.${k} exists`)
  }
})

test('the settings block decodes back to what was encoded', () => {
  const config = { effects: ['neon-sign', 'day-night', 'sound', 'lightning'].map(defaultEntry) }
  config.effects[0].cfg.Neon.pattern = 'NEON_CHASE'
  config.effects[0].source = 'night'
  config.effects[1].cfg.DayNight.onMeansDay = true
  config.effects[2].start = { mode: 'TOGGLE', pin: 16, activeLow: true, runOnMs: 90000 }
  const { bytes, layout } = encodeSettings(config)
  const dv = new DataView(bytes.buffer)
  const read = (group, name, kind) => layout.fields.find((f) => f.group === group && f.name === name && (!kind || f.kind === kind))
  assert.equal(new TextDecoder().decode(bytes.slice(0, 6)), 'MRACFG')
  assert.equal(dv.getUint32(read('header', 'hash').offset, true), layout.hash)
  assert.equal(bytes[read('neon-sign', 'pattern').offset], schema.enums.NeonPattern.indexOf('NEON_CHASE'))
  assert.equal(bytes[read('neon-sign', 'source').offset], 1, 'neon follows source #1 (night)')
  assert.equal(bytes[read('day-night', 'onMeansDay').offset], 1)
  assert.equal(bytes[read('sound', 'mode', 'start').offset], 2, 'start mode TOGGLE')
  assert.equal(bytes[read('sound', 'mode', 'cfg').offset], 0, 'Sound cfg.mode SOUND_LOOP')
  assert.equal(dv.getUint32(read('sound', 'runOnMs', 'start').offset, true), 90000)
  assert.equal(layout.size, bytes.length)
})

test('the layout hash changes when a setting is added', () => {
  const a = settingsLayout(['welder'])
  schema.classes.Welder.fields.push({ name: 'extra', type: 'uint8_t', default: 0, help: '' })
  const b = settingsLayout(['welder'])
  schema.classes.Welder.fields.pop()
  assert.notEqual(a.hash, b.hash)
})

test('a literal sketch writes only the settings that changed', () => {
  const e = defaultEntry('welder')
  e.cfg.Welder.coolTauMs = 5000
  e.start = { mode: 'WHILE_TRIGGERED', pin: 14, activeLow: true, runOnMs: 60000 }
  const ino = generateSketch({ effects: [e] })
  assert.match(ino, /#include <MRA_Welder\.h>/)
  assert.match(ino, /Welder +welder\(9, 10, 11\);/)
  assert.match(ino, /Trigger +welderTrigger\(WHILE_TRIGGERED, A0, ACTIVE_LOW, 60000\);/)
  assert.match(ino, /welder\.cfg\.coolTauMs = 5000\.0;/)
  assert.doesNotMatch(ino, /idleMin/)
  assert.doesNotMatch(ino, /SETTINGS/)
})

test('links only offer sources that are on the board', () => {
  assert.deepEqual(availableSources('sound', ['sound']), [])
  assert.deepEqual(availableSources('sound', ['sound', 'lightning']).map((s) => s.id), ['thunder'])
  assert.deepEqual(availableSources('sound', ['sound', 'crossing-gates']).map((s) => s.id), ['bell', 'crossing'])
  const e = { ...defaultEntry('sound'), source: 'thunder' }
  assert.match(generateSketch({ effects: [defaultEntry('lightning'), e] }), /sound\.update\(now, lightning\.thunder\(\)\)/)
})

test('crossing gates bring the crossing and the Servo library', () => {
  const ino = generateSketch({ effects: [defaultEntry('crossing-gates')] })
  assert.match(ino, /CrossingGates +gates\(crossing, 9, 10, 7\);/)
  assert.match(ino, /Crossing +crossing\(5, 6, 2, 3, 4, NO_PIN\);/)
  assert.deepEqual(librariesFor({ effects: [defaultEntry('crossing-gates')] }), ['Servo'])
})

test('every bundle is a valid set of catalog effects', () => {
  const ids = new Set(EFFECTS.map((e) => e.id))
  for (const b of BUNDLES) {
    for (const id of b.effects) assert.ok(ids.has(id), `${b.id}: ${id}`)
    assert.equal(bundleFor([...b.effects].reverse()), b, 'found whatever the pick order')
  }
  assert.equal(bundleFor(['welder', 'campfire']), null)
})

test('Intel HEX parsing, extended addresses and the block finder', () => {
  const rec = (addr, type, data) => {
    const bytes = [data.length, addr >> 8, addr & 0xff, type, ...data]
    const sum = (0x100 - (bytes.reduce((a, b) => a + b, 0) & 0xff)) & 0xff
    return ':' + [...bytes, sum].map((b) => b.toString(16).padStart(2, '0').toUpperCase()).join('')
  }
  const hex = [rec(0, 0, [1, 2, 3]), rec(0x10, 0, [..."MRACFG"].map((c) => c.charCodeAt(0)).concat(1)), rec(0, 1, [])].join('\n')
  const image = parseHex(hex)
  assert.deepEqual([...image.slice(0, 4)], [1, 2, 3, 0xff])
  assert.equal(findBlock(image), 0x10)
  assert.throws(() => parseHex(':0300000001020305'), /bad checksum/)
})

test('the copy-and-paste commands install what the sketch needs and pick the bootloader', async () => {
  const { buildCommands } = await import('../web/js/generate.js')
  const gates = buildCommands({ effects: [defaultEntry('crossing-gates')] }, 'old')
  assert.match(gates, /arduino-cli lib install Servo/)
  assert.match(gates, /--fqbn arduino:avr:nano:cpu=atmega328old --library mra-lib layout-effects/)
  assert.match(gates, /cat > layout-effects\/layout-effects\.ino <<'SKETCH'\n[\s\S]*#include <MRA_CrossingGates\.h>[\s\S]*\nSKETCH\n/)
  const welder = buildCommands({ effects: [defaultEntry('welder')] })
  assert.doesNotMatch(welder, /lib install/)
  assert.match(welder, /--fqbn arduino:avr:nano --library/)
})
