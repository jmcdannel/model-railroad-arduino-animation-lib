#!/usr/bin/env node
// 🏗️ Writes the installer firmware sketches (patchable mode) for every bundle
// in web/js/bundles.js, ready for arduino-cli to compile:
//
//   node tools/build-firmware.mjs <out-dir>          sketches only
//   node tools/build-firmware.mjs <out-dir> --compile  …and compile each to
//        <out-dir>/hex/<bundle>.hex + write <out-dir>/hex/manifest.json
//
// CI runs it with --compile and publishes the hex files next to the web page.
import { mkdirSync, writeFileSync, copyFileSync, readdirSync } from 'node:fs'
import { join, dirname, resolve } from 'node:path'
import { execFileSync } from 'node:child_process'
import { fileURLToPath } from 'node:url'
import { generateSketch, defaultEntry, settingsLayout } from '../web/js/generate.js'
import { BUNDLES } from '../web/js/bundles.js'

const root = resolve(dirname(fileURLToPath(import.meta.url)), '..')
const outDir = resolve(process.argv[2] || join(root, 'build/firmware'))
const compile = process.argv.includes('--compile')
const FQBN = 'arduino:avr:nano'

mkdirSync(outDir, { recursive: true })
const manifest = { generated: new Date().toISOString(), board: FQBN, bundles: [] }

for (const b of BUNDLES) {
  const name = `mra_${b.id.replace(/-/g, '_')}`
  const dir = join(outDir, name)
  mkdirSync(dir, { recursive: true })
  const config = { effects: b.effects.map(defaultEntry) }
  writeFileSync(join(dir, `${name}.ino`), generateSketch(config, { patchable: true, title: b.title }))
  const entry = { id: b.id, title: b.title, effects: b.effects, hash: settingsLayout(b.effects).hash }

  if (compile) {
    const build = join(dir, 'build')
    const log = execFileSync('arduino-cli', ['compile', '--fqbn', FQBN, '--library', root, '--output-dir', build, '--warnings', 'default', dir], { encoding: 'utf8' })
    const flash = Number((log.match(/Sketch uses (\d+) bytes/) || [])[1])
    const ram = Number((log.match(/Global variables use (\d+) bytes/) || [])[1])
    mkdirSync(join(outDir, 'hex'), { recursive: true })
    const hexName = readdirSync(build).find((f) => f.endsWith('.ino.hex') && !f.includes('bootloader'))
    copyFileSync(join(build, hexName), join(outDir, 'hex', `${b.id}.hex`))
    Object.assign(entry, { hex: `${b.id}.hex`, flash, ram })
    console.log(`✅ ${b.id.padEnd(16)} ${flash} B flash, ${ram} B RAM`)
  } else {
    console.log(`wrote ${dir}`)
  }
  manifest.bundles.push(entry)
}

if (compile) writeFileSync(join(outDir, 'hex', 'manifest.json'), JSON.stringify(manifest, null, 2))
