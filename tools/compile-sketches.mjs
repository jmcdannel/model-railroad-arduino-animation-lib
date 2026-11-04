#!/usr/bin/env node
// 🧪 Compile every sketch in the repo, in every variant it lists.
//
//   node tools/compile-sketches.mjs [--board prefix] [--summary file.md] [dir…]
//
// --board keeps only the builds whose FQBN starts with prefix (CI runs the
// AVR builds and the ESP32 builds as separate jobs).
//
// A sketch with no ci.json is built once, as-is, for the Nano. A sketch with
// a ci.json lists its builds instead:
//
//   [{ "name": "esp32 + mqtt", "fqbn": "esp32:esp32:esp32",
//      "set": { "USE_MQTT": "true", "DISPLAY_TYPE": "DISPLAY_OLED" } }]
//
// "set" rewrites `#define NAME value` lines in the sketch's .ino (in a temp
// copy), so the variants are exactly what a user gets by editing the file.
// "replace": [["from", "to"], …] swaps literal text too (e.g. pin tables).
import { execFileSync } from 'node:child_process'
import { cpSync, existsSync, mkdtempSync, readFileSync, readdirSync, rmSync, writeFileSync, appendFileSync } from 'node:fs'
import { tmpdir } from 'node:os'
import { basename, dirname, join, resolve } from 'node:path'

const NANO = 'arduino:avr:nano'
const root = resolve(dirname(new URL(import.meta.url).pathname), '..')
const args = process.argv.slice(2)
const summaryAt = args.indexOf('--summary')
const summary = summaryAt >= 0 ? args.splice(summaryAt, 2)[1] : null
const boardAt = args.indexOf('--board')
const board = boardAt >= 0 ? args.splice(boardAt, 2)[1] : ''

const dirs = args.length
  ? args.map((d) => resolve(d))
  : execFileSync('git', ['ls-files', '*.ino'], { cwd: root, encoding: 'utf8' })
      .split('\n').filter(Boolean).map((f) => join(root, dirname(f)))

function applySet(source, set, file) {
  for (const [name, value] of Object.entries(set)) {
    const re = new RegExp(`^(#define\\s+${name}\\s+).*$`, 'm')
    if (!re.test(source)) throw new Error(`${file}: no "#define ${name}" to set`)
    source = source.replace(re, `$1${value}`)
  }
  return source
}

const rows = []
let failed = 0
for (const dir of dirs) {
  const name = basename(dir)
  const ci = join(dir, 'ci.json')
  const builds = existsSync(ci) ? JSON.parse(readFileSync(ci, 'utf8')) : [{ fqbn: NANO }]
  for (const b of builds) {
    if (!(b.fqbn || NANO).startsWith(board)) continue
    const label = `${name}${b.name ? ` (${b.name})` : ''}`
    let src = dir, tmp = null
    if (b.set || b.replace) {
      tmp = mkdtempSync(join(tmpdir(), 'sketch-'))
      src = join(tmp, name)
      cpSync(dir, src, { recursive: true })
      const ino = join(src, `${name}.ino`)
      let text = applySet(readFileSync(ino, 'utf8'), b.set || {}, `${name}.ino`)
      for (const [from, to] of b.replace || []) {
        if (!text.includes(from)) throw new Error(`${name}.ino: no "${from}" to replace`)
        text = text.split(from).join(to)
      }
      writeFileSync(ino, text)
    }
    process.stdout.write(`::group::${label}\n`)
    try {
      const out = execFileSync('arduino-cli',
        ['compile', '--fqbn', b.fqbn || NANO, '--library', root, '--warnings', 'default', src],
        { encoding: 'utf8', stdio: ['ignore', 'pipe', 'pipe'] })
      process.stdout.write(out)
      const flash = out.match(/Sketch uses (\d+) bytes \((\d+)%\)/)
      const ram = out.match(/Global variables use (\d+) bytes \((\d+)%\)/)
      rows.push(`| \`${label}\` | ${b.fqbn || NANO} | ${flash ? `${flash[1]} B (${flash[2]}%)` : ''} | ${ram ? `${ram[1]} B (${ram[2]}%)` : ''} |`)
    } catch (e) {
      failed++
      process.stdout.write(`${e.stdout || ''}${e.stderr || e.message}\n`)
      process.stdout.write(`::error file=${dir}::${label} failed to compile\n`)
      rows.push(`| \`${label}\` | ${b.fqbn || NANO} | ❌ failed | |`)
    } finally {
      if (tmp) rmSync(tmp, { recursive: true, force: true })
    }
    process.stdout.write('::endgroup::\n')
  }
}

if (summary) {
  appendFileSync(summary, [
    '## 📏 Sketch sizes', '',
    '| Sketch | Board | Flash | RAM |', '|---|---|---|---|', ...rows, '',
  ].join('\n'))
}
console.log(`\n${rows.length - failed}/${rows.length} builds compiled`)
process.exit(failed ? 1 : 0)
