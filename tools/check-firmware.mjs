#!/usr/bin/env node
// 🔍 Proves the web installer and the firmware agree. Run after
// `node tools/build-firmware.mjs <dir> --compile`:
//
//   node tools/check-firmware.mjs <dir>
//
//   1. every published firmware's SETTINGS block is byte-for-byte what the
//      page would write for the default settings
//   2. sketches compiled with every setting changed (floats, enums, negative
//      numbers, unused pins) still match the page's encoder byte for byte
//   3. the copy-and-paste (literal) sketch compiles for every bundle
import { readFileSync, writeFileSync, mkdirSync, readdirSync } from 'node:fs'
import { join, dirname, resolve } from 'node:path'
import { execFileSync } from 'node:child_process'
import { fileURLToPath } from 'node:url'
import schema from '../web/js/schema.js'
import { BUNDLES } from '../web/js/bundles.js'
import { generateSketch, defaultEntry, encodeSettings } from '../web/js/generate.js'
import { parseHex, findBlock } from '../web/js/ihex.js'

const root = resolve(dirname(fileURLToPath(import.meta.url)), '..')
const dir = resolve(process.argv[2] || join(root, 'build/firmware'))
let failures = 0
const report = (ok, text) => { console.log(`${ok ? '✅' : '❌'} ${text}`); if (!ok) failures++ }

function compile(name, ino) {
  const sketchDir = join(dir, 'check', name)
  mkdirSync(sketchDir, { recursive: true })
  writeFileSync(join(sketchDir, `${name}.ino`), ino)
  execFileSync('arduino-cli', ['compile', '--fqbn', 'arduino:avr:nano', '--library', root, '--output-dir', join(sketchDir, 'build'), sketchDir], { encoding: 'utf8' })
  const hex = readdirSync(join(sketchDir, 'build')).find((f) => f.endsWith('.ino.hex') && !f.includes('bootloader'))
  return parseHex(readFileSync(join(sketchDir, 'build', hex), 'utf8'))
}

function blockMatches(image, config) {
  const at = findBlock(image)
  const { bytes } = encodeSettings(config)
  if (at < 0) return 'no SETTINGS block found'
  const diff = bytes.findIndex((b, i) => b !== image[at + i])
  return diff < 0 ? null : `byte ${diff} of the block differs`
}

// 1️⃣ published firmwares vs. default settings
const manifest = JSON.parse(readFileSync(join(dir, 'hex', 'manifest.json'), 'utf8'))
for (const b of manifest.bundles) {
  const image = parseHex(readFileSync(join(dir, 'hex', b.hex), 'utf8'))
  const err = blockMatches(image, { effects: b.effects.map(defaultEntry) })
  report(!err, `${b.id}: published block ${err || 'matches the page'}`)
}

// 2️⃣ every setting changed: the encoder and avr-gcc must still agree
let seed = 7
const rnd = (n) => (seed = (seed * 1103515245 + 12345) % 2147483648) % n
function scramble(e) {
  for (const cls of Object.keys(e.cfg)) for (const f of schema.classes[cls].fields) {
    const t = f.type, cur = e.cfg[cls][f.name]
    e.cfg[cls][f.name] = schema.enums[t] ? schema.enums[t][(schema.enums[t].indexOf(cur) + 1) % schema.enums[t].length]
      : t === 'bool' ? !cur : t === 'float' ? 1234.5 + rnd(100) : t === 'int16_t' ? -1 - rnd(300)
      : t === 'uint32_t' ? 70000 + rnd(900000) : t === 'uint16_t' ? 1 + rnd(65000) : 1 + rnd(254)
  }
  for (const cls of Object.keys(e.pins)) for (const p of Object.keys(e.pins[cls])) e.pins[cls][p] = [2, 3, 5, 14, 19, 255][rnd(6)]
  e.start = { mode: 'TOGGLE', pin: 17, activeLow: true, runOnMs: 123456 }
  return e
}
for (const ids of [['welder', 'neon-sign'], ['crossing-gates', 'sound'], ['day-night', 'beacon', 'fluorescent', 'traffic-light'], ['lightning', 'sound', 'campfire', 'tv-glow']]) {
  const config = { effects: ids.map((id) => scramble(defaultEntry(id))) }
  const sound = config.effects.find((e) => e.id === 'sound')
  if (sound) sound.source = ids.includes('lightning') ? 'thunder' : 'bell'
  const err = blockMatches(compile(`scrambled_${ids.join('_').replace(/-/g, '_')}`, generateSketch(config, { patchable: true })), config)
  report(!err, `${ids.join(' + ')}: every setting changed, ${err || 'block matches'}`)
}

// 3️⃣ copy-and-paste sketches compile
for (const b of BUNDLES) {
  try {
    compile(`literal_${b.id.replace(/-/g, '_')}`, generateSketch({ effects: b.effects.map(defaultEntry) }))
    report(true, `${b.id}: copy-and-paste sketch compiles`)
  } catch (err) {
    report(false, `${b.id}: copy-and-paste sketch failed to compile\n${err.stdout || err.message}`)
  }
}

process.exit(failures ? 1 : 0)
