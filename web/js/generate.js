// 🏭 Turns a configuration into an Arduino sketch, two ways:
//
//   literal    a readable sketch with your pins and settings written in, for
//              copy & paste / arduino-cli
//   patchable  the same sketch, but every pin, start mode and setting is read
//              from one SETTINGS block in flash. CI compiles these into the
//              prebuilt firmwares; the web installer finds the block (by its
//              "MRACFG" marker) and writes your values into it before flashing.
//
// Both modes share settingsLayout(), so the block CI builds and the bytes the
// page writes always agree. A hash of the layout is stored in the block; the
// installer refuses a firmware whose hash doesn't match.
//
// A configuration ("config") looks like:
//   { effects: [ { id: 'welder',
//                  pins:  { Welder: { whitePin: 9, … } },
//                  cfg:   { Welder: { idleMin: 3000, … } },
//                  start: { mode: 'POWER_UP', pin: 12, activeLow: false, runOnMs: 0 },
//                  source: 'trigger' } ] }
import schema from './schema.js'
import { EFFECTS, byId, NO_PIN } from './effects.js'

export const MAGIC = 'MRACFG'
export const FORMAT_VERSION = 1
const START_MODES = schema.enums.StartMode   // POWER_UP, WHILE_TRIGGERED, TOGGLE

// ------------------------------------------------------------ defaults 🧰
export function defaultEntry(id) {
  const fx = byId[id]
  const pins = {}, cfg = {}
  for (const o of fx.objects) {
    const cls = schema.classes[o.cls]
    pins[o.cls] = Object.fromEntries(cls.pins.map((p) => [p.name, fx.pins?.[o.cls]?.[p.name] ?? p.default]))
    cfg[o.cls] = Object.fromEntries(cls.fields.map((f) => [f.name, fx.cfg?.[o.cls]?.[f.name] ?? f.default]))
  }
  return { id, pins, cfg, start: { mode: 'POWER_UP', pin: 12, activeLow: false, runOnMs: 0 }, source: 'trigger' }
}

// effects in update order (the catalog order), whatever order they were picked in
export const ordered = (entries) => EFFECTS.map((fx) => entries.find((e) => e.id === fx.id)).filter(Boolean)

// sources an effect can take in this configuration (trigger + links whose source is present)
export function availableSources(id, ids) {
  const fx = byId[id]
  return (fx.sources || []).filter((s) => [].concat(s.needs).some((n) => ids.includes(n)))
}

// --------------------------------------------------------- layout 📐
// The SETTINGS block: header, then for each effect (in update order) each
// object's pins and Config fields, then its start settings.
const HEADER = [
  { name: 'magic', type: 'char6', size: 6 },
  { name: 'version', type: 'uint8_t', size: 1 },
  { name: 'count', type: 'uint8_t', size: 1 },
  { name: 'hash', type: 'uint32_t', size: 4 },
]
const START = [
  { name: 'mode', type: 'uint8_t' }, { name: 'pin', type: 'uint8_t' }, { name: 'activeLow', type: 'bool' },
  { name: 'runOnMs', type: 'uint32_t' }, { name: 'source', type: 'uint8_t' },
]

const sizeOf = (type) => schema.sizes[type] ?? (schema.enums[type] ? 1 : null)

export function settingsLayout(ids) {
  const fields = []                   // { group, cls, name, type, size, offset, kind }
  let offset = 0
  const add = (f) => { fields.push({ ...f, offset }); offset += f.size }
  for (const h of HEADER) add({ group: 'header', ...h, kind: 'header' })
  for (const fx of EFFECTS.filter((f) => ids.includes(f.id))) {
    for (const o of fx.objects) {
      const cls = schema.classes[o.cls]
      for (const p of cls.pins) add({ group: fx.id, cls: o.cls, v: o.v, name: p.name, type: 'uint8_t', size: 1, kind: 'pin' })
      for (const f of cls.fields) add({ group: fx.id, cls: o.cls, v: o.v, name: f.name, type: f.type, size: sizeOf(f.type), kind: 'cfg' })
    }
    for (const s of START) add({ group: fx.id, name: s.name, type: s.type, size: sizeOf(s.type), kind: 'start' })
  }
  const signature = fields.filter((f) => f.kind !== 'header').map((f) => `${f.group}.${f.cls || ''}.${f.name}:${f.type}`).join(';')
  return { fields, size: offset, hash: fnv1a(`v${FORMAT_VERSION};${signature}`) }
}

export function fnv1a(text) {
  let h = 0x811c9dc5
  for (const b of new TextEncoder().encode(text)) { h ^= b; h = Math.imul(h, 0x01000193) >>> 0 }
  return h >>> 0
}

// ------------------------------------------------------ encode 🔢
// The bytes of the SETTINGS block for a configuration (little-endian, no
// padding: AVR structs are byte-aligned).
export function encodeSettings(config) {
  const entries = ordered(config.effects)
  const ids = entries.map((e) => e.id)
  const layout = settingsLayout(ids)
  const buf = new Uint8Array(layout.size)
  const dv = new DataView(buf.buffer)
  for (const f of layout.fields) {
    const e = entries.find((x) => x.id === f.group)
    let v
    if (f.kind === 'header') v = { magic: MAGIC, version: FORMAT_VERSION, count: entries.length, hash: layout.hash }[f.name]
    else if (f.kind === 'pin') v = e.pins[f.cls][f.name]
    else if (f.kind === 'cfg') v = e.cfg[f.cls][f.name]
    else v = startValue(e, f.name, ids)
    writeValue(dv, f, v)
  }
  return { bytes: buf, layout }
}

function startValue(e, name, ids) {
  if (name === 'mode') return START_MODES.indexOf(e.start.mode)
  if (name === 'source') {
    const i = availableSources(e.id, ids).findIndex((s) => s.id === e.source)
    return i < 0 ? 0 : i + 1               // 0 = its own trigger
  }
  return e.start[name]
}

function writeValue(dv, f, v) {
  const o = f.offset
  if (f.type === 'char6') { for (let i = 0; i < 6; i++) dv.setUint8(o + i, v.charCodeAt(i)); return }
  if (schema.enums[f.type]) v = typeof v === 'string' ? schema.enums[f.type].indexOf(v) : v
  if (typeof v === 'boolean') v = v ? 1 : 0
  switch (f.type) {
    case 'uint8_t': case 'bool': dv.setUint8(o, v); break
    case 'int8_t': dv.setInt8(o, v); break
    case 'uint16_t': dv.setUint16(o, v, true); break
    case 'int16_t': case 'int': dv.setInt16(o, v, true); break
    case 'uint32_t': dv.setUint32(o, v, true); break
    case 'int32_t': dv.setInt32(o, v, true); break
    case 'float': dv.setFloat32(o, v, true); break
    default:
      if (schema.enums[f.type]) { dv.setUint8(o, v); break }
      throw new Error(`can't encode ${f.type}`)
  }
}

// ------------------------------------------------------ sketch ✍️
const cLiteral = (type, v) => {
  if (schema.enums[type]) return typeof v === 'string' ? v : schema.enums[type][v]
  if (typeof v === 'boolean') return v ? 'true' : 'false'
  if (type === 'float') return Number.isInteger(v) ? `${v}.0` : String(v)
  return String(v)
}
const pinLiteral = (p) => (p === NO_PIN ? 'NO_PIN' : p >= 14 && p <= 21 ? `A${p - 14}` : String(p))
const ctype = (t) => (schema.enums[t] ? 'uint8_t' : t)

export function generateSketch(config, { patchable = false, title = 'My layout effects' } = {}) {
  const entries = ordered(config.effects)
  if (!entries.length) throw new Error('pick at least one effect')
  const ids = entries.map((e) => e.id)
  const layout = settingsLayout(ids)
  const S = (path) => `mra::setting(SETTINGS.${path})`
  const out = []
  const w = (line = '') => out.push(line)

  // header comment + includes
  w('/*')
  w(` * 🚂 ${title}`)
  w(` * Generated by the Model Railroad Animation configurator${patchable ? ' (installer firmware)' : ''}.`)
  w(' * Effects: ' + entries.map((e) => `${byId[e.id].emoji} ${byId[e.id].title}`).join(', '))
  if (patchable) {
    w(' *')
    w(' * Every pin, start mode and setting lives in the SETTINGS block below. The web')
    w(' * installer writes your values into it before flashing, so this one build')
    w(' * serves every wiring.')
  }
  w(' */')
  const headers = [...new Set(entries.flatMap((e) => byId[e.id].objects.map((o) => schema.classes[o.cls].header)))]
  for (const h of headers.filter((h) => !(h === 'MRA_Crossing.h' && headers.includes('MRA_CrossingGates.h')))) w(`#include <${h}>`)
  w('using namespace mra;')
  w()

  if (patchable) {
    // 📐 the SETTINGS struct, field for field from settingsLayout()
    w('// ------------------------------------------------- settings block 📐')
    w('namespace mra {')
    w('template <class T> T setting(const T &field) { T v; memcpy_P(&v, &field, sizeof(T)); return v; }')
    w('}')
    w('struct Settings {')
    w('  char magic[6]; uint8_t version; uint8_t count; uint32_t hash;')
    for (const e of entries) {
      const fl = layout.fields.filter((f) => f.group === e.id)
      w(`  struct { ${fl.map((f) => `${ctype(f.type)} ${memberName(f)};`).join(' ')} } ${groupName(e.id)};`)
    }
    w('};')
    const init = []
    for (const e of entries) {
      const fl = layout.fields.filter((f) => f.group === e.id)
      init.push(`{ ${fl.map((f) => cInit(f, e, ids)).join(', ')} }`)
    }
    w(`const Settings SETTINGS PROGMEM __attribute__((used)) = {`)
    w(`  { 'M', 'R', 'A', 'C', 'F', 'G' }, ${FORMAT_VERSION}, ${entries.length}, 0x${layout.hash.toString(16).toUpperCase()}UL,`)
    for (const line of init) w(`  ${line},`)
    w('};')
    w('#ifdef __AVR__                            // AVR structs have no padding; the page relies on it')
    w(`static_assert(sizeof(Settings) == ${layout.size}, "settings layout drifted from the configurator");`)
    w('#endif')
    w()
  }

  // 🎬 objects and triggers
  w('// ---------------------------------------------------------- effects 🎬')
  for (const e of entries) {
    const fx = byId[e.id]
    for (const o of fx.objects) {
      const cls = schema.classes[o.cls]
      const args = [...(o.args || []), ...cls.pins.map((p) => patchable ? S(`${groupName(e.id)}.${o.v}_${p.name}`) : pinLiteral(e.pins[o.cls][p.name]))]
      const roles = cls.pins.map((p) => p.name).join(', ')
      w(`${o.cls.padEnd(13)} ${o.v}(${args.join(', ')});${patchable ? '' : `   // ${roles}`}`)
    }
    w(`Trigger       ${trigVar(e.id)}(${triggerArgs(e, patchable, S)});`)
  }
  w()

  // 🚀 setup
  w('void setup() {')
  w('  randomSeed(analogRead(A7));               // A7 is input-only on a Nano: never in use')
  for (const e of entries) {
    const fx = byId[e.id]
    for (const o of fx.objects) {
      const cls = schema.classes[o.cls]
      for (const f of cls.fields) {
        if (patchable) {
          const cast = schema.enums[f.type] ? `(${f.type})` : ''
          w(`  ${o.v}.cfg.${f.name} = ${cast}${S(`${groupName(e.id)}.${o.v}_${f.name}`)};`)
        } else {
          const v = e.cfg[o.cls][f.name]
          if (v !== f.default) w(`  ${o.v}.cfg.${f.name} = ${cLiteral(f.type, v)};${f.help ? `   // ${f.help}` : ''}`)
        }
      }
    }
    for (const o of fx.objects) w(`  ${o.v}.begin();`)
    w(`  ${trigVar(e.id)}.begin();`)
  }
  w('}')
  w()

  // 🔁 loop
  w('void loop() {')
  w('  uint32_t now = millis();')
  for (const e of entries) {
    const fx = byId[e.id]
    const t = trigVar(e.id)
    if (fx.kind === 'crossing') {
      // the crossing is armed by its sensors; the trigger (if not POWER_UP) fires it
      w(`  ${'crossing'}.update(now, ${t}.mode() != POWER_UP && ${t}.update(now));`)
      if (fx.objects.some((o) => o.v === 'gates')) w('  gates.update(now);')
      continue
    }
    const srcs = availableSources(e.id, ids)
    const v = fx.objects[0].v
    if (!srcs.length) { w(`  ${v}.update(now, ${t}.update(now));`); continue }
    if (patchable) {
      w(`  switch (${S(`${groupName(e.id)}.source`)}) {`)
      srcs.forEach((s, i) => w(`    case ${i + 1}: ${v}.update(now, ${s.expr}); break;`))
      w(`    default: ${v}.update(now, ${t}.update(now));`)
      w('  }')
    } else {
      const s = srcs.find((x) => x.id === e.source)
      w(`  ${v}.update(now, ${s ? s.expr : `${t}.update(now)`});${s ? `   // ${s.label}` : ''}`)
    }
  }
  w('}')
  return out.join('\n') + '\n'
}

const groupName = (id) => 'fx_' + id.replace(/-/g, '_')
const trigVar = (id) => byId[id].objects[0].v + 'Trigger'
const memberName = (f) => (f.kind === 'start' ? f.name : `${f.v}_${f.name}`)

function triggerArgs(e, patchable, S) {
  if (patchable) {
    const g = groupName(e.id)
    return `(StartMode)${S(`${g}.mode`)}, ${S(`${g}.pin`)}, ${S(`${g}.activeLow`)}, ${S(`${g}.runOnMs`)}`
  }
  const s = e.start
  const args = [s.mode, pinLiteral(s.pin)]
  if (s.activeLow || s.runOnMs) args.push(s.activeLow ? 'ACTIVE_LOW' : 'ACTIVE_HIGH')
  if (s.runOnMs) args.push(String(s.runOnMs))
  return s.mode === 'POWER_UP' ? 'POWER_UP' : args.join(', ')
}

function cInit(f, e, ids) {
  let v
  if (f.kind === 'pin') v = e.pins[f.cls][f.name]
  else if (f.kind === 'cfg') v = e.cfg[f.cls][f.name]
  else v = startValue(e, f.name, ids)
  if (schema.enums[f.type]) return String(typeof v === 'string' ? schema.enums[f.type].indexOf(v) : v)
  if (typeof v === 'boolean') return v ? 'true' : 'false'
  if (f.type === 'float') return Number.isInteger(v) ? `${v}.0f` : `${v}f`
  if (f.type === 'uint32_t') return `${v}UL`
  return String(v)
}

// libraries a configuration needs beyond the AVR core
export const librariesFor = (config) => [...new Set(config.effects.flatMap((e) => byId[e.id].libraries || []))]

// 📋 shell commands that set up arduino-cli, write the sketch and build it
export const REPO_URL = 'https://github.com/jmcdannel/model-railroad-arduino-animation-lib.git'
export function buildCommands(config, bootloader = 'new', { port = '/dev/cu.usbserial-XXXX', name = 'layout-effects' } = {}) {
  const fqbn = bootloader === 'old' ? 'arduino:avr:nano:cpu=atmega328old' : 'arduino:avr:nano'
  return [
    '# 🧰 one-time setup',
    'arduino-cli core update-index && arduino-cli core install arduino:avr',
    ...librariesFor(config).map((l) => `arduino-cli lib install ${l}`),
    `[ -d mra-lib ] && git -C mra-lib pull --ff-only || git clone --depth 1 ${REPO_URL} mra-lib`,
    '',
    '# ✍️ your sketch',
    `mkdir -p ${name}`,
    `cat > ${name}/${name}.ino <<'SKETCH'`,
    generateSketch(config, { title: 'Layout effects' }).trimEnd(),
    'SKETCH',
    '',
    '# 🔌 build and upload (find your port with: arduino-cli board list)',
    `arduino-cli compile --fqbn ${fqbn} --library mra-lib ${name}`,
    `arduino-cli upload  --fqbn ${fqbn} -p ${port} ${name}`,
  ].join('\n')
}
