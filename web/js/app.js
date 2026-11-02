// 🎛️ The configurator page: lever bank → effect plates → pin plan → outputs.
import schema from './schema.js'
import { EFFECTS, byId, PINS, KEY_FIELDS, PIN_CHOICES, PWM_PINS, NO_PIN, pinName } from './effects.js'
import { defaultEntry, ordered, availableSources, generateSketch, encodeSettings, buildCommands } from './generate.js'
import { BUNDLES, bundleFor } from './bundles.js'
import { pinUses, checkPlan, assignPins, freeTriggerPin } from './pins.js'
import { parseHex, findBlock } from './ihex.js'
import { Stk500, BAUD } from './stk500.js'
import { supported, WebSerialTransport } from './webserial.js'

const LAMPS = ['#46b07a', '#f0b23a', '#dce4ee', '#6fb3dd', '#d08fdf', '#e9905e', '#b5d36a', '#f2d16b', '#8fd9c4', '#c9a27a', '#9fa8f0', '#e6b3c8']
const lampOf = (id) => LAMPS[EFFECTS.findIndex((e) => e.id === id) % LAMPS.length]
const STORE = 'mra-configurator-v1'
const $ = (sel) => document.querySelector(sel)
let lastAnnounced = ''
function announce(text) {
  if (text === lastAnnounced) return
  lastAnnounced = text
  $('#announcer').textContent = text
}

// ------------------------------------------------------------ state 💾
let state = load()

function load() {
  try {
    const saved = JSON.parse(localStorage.getItem(STORE) || 'null')
    if (saved && Array.isArray(saved.effects)) {
      // merge onto fresh defaults so new settings appear and removed ones vanish
      const effects = saved.effects.filter((e) => byId[e.id]).map((e) => {
        const d = defaultEntry(e.id)
        for (const cls of Object.keys(d.pins)) for (const k of Object.keys(d.pins[cls])) if (Number.isInteger(e.pins?.[cls]?.[k])) d.pins[cls][k] = e.pins[cls][k]
        for (const cls of Object.keys(d.cfg)) for (const k of Object.keys(d.cfg[cls])) if (e.cfg?.[cls]?.[k] !== undefined) d.cfg[cls][k] = e.cfg[cls][k]
        if (e.start) d.start = { ...d.start, ...e.start }
        if (e.source) d.source = e.source
        return d
      })
      return normalize({ effects, bootloader: saved.bootloader === 'old' ? 'old' : 'new' })
    }
  } catch { /* private window or bad data: start fresh */ }
  return { effects: [], bootloader: 'new' }
}

function save() {
  try { localStorage.setItem(STORE, JSON.stringify(state)) } catch { /* not essential */ }
}

// keep links valid when effects come and go
function normalize(s) {
  const ids = s.effects.map((e) => e.id)
  for (const e of s.effects) {
    if (e.source !== 'trigger' && !availableSources(e.id, ids).some((x) => x.id === e.source)) e.source = 'trigger'
  }
  return s
}

function changed({ structure = false } = {}) {
  normalize(state)
  save()
  if (structure) renderPlates()
  renderLevers()
  renderBoard()
  renderOutputs()
}

// ------------------------------------------------------- labels 🏷️
function humanize(name) {
  const units = { Ms: 'ms', Percent: '%', Deg: '°', Us: 'µs', PerMin: 'per min' }
  let unit = ''
  let base = name
  for (const [suffix, u] of Object.entries(units)) if (base.endsWith(suffix)) { unit = u; base = base.slice(0, -suffix.length); break }
  const words = base.replace(/([a-z0-9])([A-Z])/g, '$1 $2').replace(/([A-Z])([A-Z][a-z])/g, '$1 $2').toLowerCase()
    .replace(/\blvl\b/g, 'level').replace(/\bpwm\b/g, 'PWM')
  const text = words.charAt(0).toUpperCase() + words.slice(1)
  return unit ? `${text} (${unit})` : text
}
const enumLabel = (v) => { const t = v.replace(/^[A-Z]+_/, '').replace(/_/g, ' ').toLowerCase(); return t.charAt(0).toUpperCase() + t.slice(1) }
const pinOption = (p) => `${pinName(p)}${PWM_PINS.includes(p) ? ' (PWM)' : ''}`
// fields that pick "which segment/tube", stored 0–3 with 255 = none
const INDEX_FIELDS = { brokenSegment: 'Segment', badTube: 'Tube' }
const RANGES = { uint8_t: [0, 255], int8_t: [-128, 127], uint16_t: [0, 65535], int16_t: [-32768, 32767], int: [-32768, 32767], uint32_t: [0, 4294967295], int32_t: [-2147483648, 2147483647], float: [-1e6, 1e6] }

// ------------------------------------------------------- levers 🎚️
function renderLevers() {
  const bank = $('#lever-bank')
  if (!bank.children.length) {
    for (const fx of EFFECTS) {
      const b = document.createElement('button')
      b.type = 'button'
      b.className = 'lever'
      b.dataset.id = fx.id
      b.style.setProperty('--lamp', lampOf(fx.id))
      b.innerHTML = `<span class="lamp lever__lamp" aria-hidden="true"></span>
        <span class="lever__name" id="lever-${fx.id}"><span aria-hidden="true">${fx.emoji}</span> ${fx.title}</span>
        <span class="lever__handle" aria-hidden="true"></span>
        <span class="lever__blurb" id="lever-${fx.id}-blurb">${fx.blurb}</span>`
      b.setAttribute('aria-labelledby', `lever-${fx.id}`)
      b.setAttribute('aria-describedby', `lever-${fx.id}-blurb`)
      b.addEventListener('click', () => toggleEffect(fx.id))
      bank.append(b)
    }
  }
  for (const b of bank.children) b.setAttribute('aria-pressed', String(state.effects.some((e) => e.id === b.dataset.id)))
}

function toggleEffect(id) {
  const on = state.effects.some((e) => e.id === id)
  if (on) state.effects = state.effects.filter((e) => e.id !== id)
  else {
    const excludes = byId[id].excludes || []
    state.effects = state.effects.filter((e) => !excludes.includes(e.id))
    state.effects.push(assignPins(defaultEntry(id), state.effects))
  }
  changed({ structure: true })
}

// ------------------------------------------------- effect plates 🪧
function renderPlates() {
  const host = $('#effect-plates')
  host.innerHTML = ''
  const entries = ordered(state.effects)
  $('#empty').hidden = entries.length > 0
  $('#board-section').hidden = !entries.length
  $('#outputs').hidden = !entries.length
  for (const e of entries) host.append(plateFor(e))
}

function plateFor(e) {
  const fx = byId[e.id]
  const el = document.createElement('article')
  el.className = 'plate fx'
  el.style.setProperty('--lamp', lampOf(e.id))
  el.innerHTML = `<div class="fx__head"><h2><span class="lamp lamp--lit fx__lamp" aria-hidden="true"></span><span><span aria-hidden="true">${fx.emoji}</span> ${fx.title}</span></h2>
    <button type="button" class="btn btn--quiet fx__remove" aria-label="Remove ${fx.title}">Remove</button></div>
    <p class="fx__blurb">${fx.blurb}</p>`
  el.querySelector('.fx__remove').addEventListener('click', () => toggleEffect(e.id))

  // 📌 pins
  const pins = group('Pins')
  for (const o of fx.objects) {
    for (const p of schema.classes[o.cls].pins) {
      const [label, fades, optional] = PINS[o.cls][p.name]
      const choices = [...(optional ? [NO_PIN] : []), ...PIN_CHOICES]
      pins.grid.append(selectField(label, choices.map((c) => [c, c === NO_PIN ? 'Not used' : pinOption(c)]), e.pins[o.cls][p.name],
        (v) => { e.pins[o.cls][p.name] = Number(v); changed() }, fades ? 'Fades: needs a PWM pin' : '', { pin: true }))
    }
  }
  el.append(pins.el)

  // 🎛️ start mode
  el.append(startGroup(e))

  // ⚙️ settings
  for (const o of fx.objects) {
    const fields = schema.classes[o.cls].fields
    const key = KEY_FIELDS[o.cls] || []
    const settings = group(fx.objects.length > 1 ? `${o.cls === 'CrossingGates' ? 'Gate' : 'Crossing'} settings` : 'Settings')
    for (const f of fields.filter((f) => key.includes(f.name))) settings.grid.append(settingField(e, o.cls, f))
    const rest = fields.filter((f) => !key.includes(f.name))
    if (rest.length) {
      const more = document.createElement('details')
      more.className = 'more'
      more.innerHTML = `<summary>More settings (${rest.length})</summary>`
      const g = document.createElement('div')
      g.className = 'grid'
      for (const f of rest) g.append(settingField(e, o.cls, f))
      more.append(g)
      settings.el.append(more)
    }
    el.append(settings.el)
  }
  return el
}

function startGroup(e) {
  const fx = byId[e.id]
  const g = group('Start')
  const ids = state.effects.map((x) => x.id)
  const sources = availableSources(e.id, ids)
  if (sources.length) {
    g.grid.append(selectField('Switched by', [['trigger', 'Its own start mode'], ...sources.map((s) => [s.id, s.label])], e.source,
      (v) => { e.source = v; changed({ structure: true }) }))
  }
  if (e.source !== 'trigger') {
    g.el.append(note(`Another effect on this board switches it (${sources.find((s) => s.id === e.source).label.toLowerCase()}), so it needs no trigger pin.`))
    return g.el
  }

  const modes = fx.kind === 'crossing'
    ? [['POWER_UP', 'Sensors and button only'], ['WHILE_TRIGGERED', 'Also fire from a pin'], ['TOGGLE', 'Also toggle from a button']]
    : [['POWER_UP', 'Runs from power-up'], ['WHILE_TRIGGERED', 'While a pin is triggered'], ['TOGGLE', 'Toggle with a button']]
  g.grid.append(selectField('Start mode', modes, e.start.mode, (v) => {
    const was = e.start.mode
    e.start.mode = v
    if (was === 'POWER_UP' && v !== 'POWER_UP') e.start.pin = freeTriggerPin(state.effects, e)
    if (v === 'TOGGLE') e.start.activeLow = true
    changed({ structure: true })
  }))
  if (e.start.mode !== 'POWER_UP') {
    g.grid.append(selectField('Trigger pin', PIN_CHOICES.map((c) => [c, pinOption(c)]), e.start.pin, (v) => { e.start.pin = Number(v); changed() }, '', { pin: true }))
    g.grid.append(selectField('Signal', [['false', 'HIGH = on (DEJA pin, PIR sensor)'], ['true', 'Pulled to GND = on (button, relay, IR module)']],
      String(e.start.activeLow), (v) => { e.start.activeLow = v === 'true'; changed() }))
    if (e.start.mode === 'WHILE_TRIGGERED') {
      g.grid.append(numberField('Keep running after release (s)', e.start.runOnMs / 1000, 0, 4294967, 'any',
        (v) => { e.start.runOnMs = Math.round(v * 1000); changed() }, '0 = stop at once. 60 = a button press runs a minute.'))
    }
  }
  const off = fx.kind === 'crossing' ? 'The crossing is always armed by its sensors and button.' : `When it's off: ${fx.offText}.`
  g.el.append(note(off))
  return g.el
}

function settingField(e, cls, f) {
  const set = (v) => { e.cfg[cls][f.name] = v; changed() }
  const val = e.cfg[cls][f.name]
  if (INDEX_FIELDS[f.name]) {
    return selectField(humanize(f.name), [[NO_PIN, 'None'], ...[0, 1, 2, 3].map((i) => [i, `${INDEX_FIELDS[f.name]} ${i + 1}`])], val, (v) => set(Number(v)), f.help)
  }
  if (schema.enums[f.type]) return selectField(humanize(f.name), schema.enums[f.type].map((v) => [v, enumLabel(v)]), val, set, f.help)
  if (f.type === 'bool') return checkField(humanize(f.name), val, set, f.help)
  const [min, max] = RANGES[f.type]
  return numberField(humanize(f.name), val, min, max, f.type === 'float' ? 'any' : 1, set, f.help)
}

// ------------------------------------------------- form helpers 🧩
function group(title) {
  const el = document.createElement('div')
  el.className = 'group'
  el.innerHTML = `<h3>${title}</h3>`
  const grid = document.createElement('div')
  grid.className = 'grid'
  el.append(grid)
  return { el, grid }
}

function note(text) {
  const p = document.createElement('p')
  p.className = 'hint'
  p.textContent = text
  return p
}

function selectField(label, options, value, onChange, help = '', { pin = false } = {}) {
  const l = document.createElement('label')
  l.className = 'field'
  l.textContent = label
  const s = document.createElement('select')
  for (const [v, text] of options) {
    const o = document.createElement('option')
    o.value = String(v)
    o.textContent = text
    s.append(o)
  }
  s.value = String(value)
  if (pin) s.dataset.pin = ''
  s.addEventListener('change', () => onChange(s.value))
  l.append(s)
  if (help) { const h = document.createElement('small'); h.textContent = help; l.append(h) }
  return l
}

function numberField(label, value, min, max, step, onChange, help = '') {
  const l = document.createElement('label')
  l.className = 'field'
  l.textContent = label
  const i = document.createElement('input')
  Object.assign(i, { type: 'number', min, max, step, value })
  i.addEventListener('change', () => {
    const n = Number(i.value)
    if (i.value === '' || Number.isNaN(n)) { i.value = value; return }
    const v = Math.min(max, Math.max(min, step === 'any' ? n : Math.round(n)))
    i.value = v
    value = v
    onChange(v)
  })
  l.append(i)
  if (help) { const h = document.createElement('small'); h.textContent = help; l.append(h) }
  return l
}

function checkField(label, value, onChange, help = '') {
  const l = document.createElement('label')
  l.className = 'field field--check'
  const i = document.createElement('input')
  i.type = 'checkbox'
  i.checked = value
  i.addEventListener('change', () => onChange(i.checked))
  l.append(i, document.createTextNode(label))
  if (help) { const h = document.createElement('small'); h.textContent = help; l.append(h) }
  return l
}

// ------------------------------------------------- the pin panel 💡
function renderBoard() {
  if (!state.effects.length) return
  const uses = pinUses(state.effects)
  const problems = checkPlan(state.effects)
  const bad = new Set(problems.map((p) => p.pin))
  const pin = (p) => {
    const u = uses.filter((x) => x.pin === p)
    const cls = ['pin', PWM_PINS.includes(p) && 'pin--pwm', u.length && 'pin--used', bad.has(p) && 'pin--bad', p <= 1 && 'pin--reserved'].filter(Boolean).join(' ')
    const use = p <= 1 ? 'USB serial' : u.map((x) => x.role).join(', ')
    const lamp = u.length ? lampOf(u[0].effect) : ''
    return `<div class="${cls}" style="--lamp:${lamp}"><span class="pin__lamp"></span><span class="pin__name">${pinName(p)}</span><span class="pin__use">${use}</span></div>`
  }
  const digital = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13]
  $('#board').innerHTML = `<div class="board__row">${digital.map(pin).join('')}</div>
    <div class="board__chip">Arduino Nano (ATmega328P)</div>
    <div class="board__row">${[14, 15, 16, 17, 18, 19].map(pin).join('')}</div>
    <p class="board__legend">~ marks the PWM pins that can fade. A6 and A7 can't be used as digital pins.</p>`
  const list = $('#problems')
  list.innerHTML = problems.length ? problems.map((p) => `<li>${p.text}</li>`).join('') : '<li class="ok">Pin plan looks good: no clashes.</li>'
  announce(problems.length ? `Pin plan: ${problems.length} problem${problems.length > 1 ? 's' : ''} to fix.` : 'Pin plan looks good.')
  document.querySelectorAll('select[data-pin]').forEach((s) => s.closest('.field').classList.toggle('is-bad', bad.has(Number(s.value))))
}

// ------------------------------------------------------ outputs 📦
function sketchText() { return generateSketch(state, { title: 'Layout effects' }) }

const commandsText = () => buildCommands(state, state.bootloader)

function renderOutputs() {
  if (!state.effects.length) return
  $('#commands').textContent = commandsText()
  $('#bootloader-cmd').value = state.bootloader
  renderInstall()
}

let installing = false

function renderInstall() {
  if (installing) return
  const body = $('#install-body')
  const ids = state.effects.map((e) => e.id)
  const bundle = bundleFor(ids)
  const problems = checkPlan(state.effects)
  const parts = []
  if (!supported()) {
    parts.push(`<p class="notice">Installing from the page needs Chrome or Edge on a desktop computer. The commands below work everywhere.</p>`)
  } else if (!bundle) {
    const sets = BUNDLES.filter((b) => b.effects.length > 1).map((b) => b.title).join(', ')
    parts.push(`<p class="notice">There's no ready-made firmware for this combination. One-click install covers any single effect, plus these sets: ${sets}. For this board, use the commands below.</p>`)
  } else if (problems.length) {
    parts.push(`<p class="notice notice--bad">Fix the pin plan above before installing.</p>`)
  }
  const ready = supported() && bundle && !problems.length
  parts.push(`<ol class="steps">
      <li>Plug the Nano into this computer's USB port.</li>
      <li>Pick its bootloader. Genuine Nanos use the new one; many clones use the old one.</li>
      <li>Select <strong>Install</strong> and choose the Nano's port when the browser asks (often “USB Serial” or “CH340”).</li>
    </ol>
    <div class="row">
      <label class="field field--inline">Bootloader
        <select id="bootloader-install">
          <option value="new">Nano (new bootloader)</option>
          <option value="old">Nano clone (old bootloader)</option>
        </select>
      </label>
      <button type="button" class="btn btn--big" id="install" ${ready ? '' : 'disabled'}>Install${bundle ? ` ${bundle.title}` : ''}</button>
    </div>
    <div class="progress" hidden role="progressbar" aria-label="Install progress" aria-valuemin="0" aria-valuemax="100" aria-valuenow="0"><div class="progress__bar"></div></div>
    <pre class="log" id="install-log" role="log" aria-live="polite"></pre>`)
  body.innerHTML = parts.join('')
  const sel = $('#bootloader-install')
  sel.value = state.bootloader
  sel.addEventListener('change', () => { state.bootloader = sel.value; changed() })
  $('#install').addEventListener('click', install)
}

async function install() {
  const logEl = $('#install-log')
  const bar = $('.progress__bar')
  const log = (line) => { logEl.textContent += line + '\n' }
  logEl.textContent = ''
  installing = true
  $('#install').disabled = true
  $('.progress').hidden = false
  let transport
  try {
    const bundle = bundleFor(state.effects.map((e) => e.id))
    const manifest = await (await fetch('firmware/manifest.json', { cache: 'no-cache' })).json()
    const published = manifest.bundles.find((b) => b.id === bundle.id)
    if (!published) throw new Error(`The ${bundle.title} firmware isn't published yet.`)
    const { bytes, layout } = encodeSettings(state)
    if (published.hash !== layout.hash) throw new Error('The published firmware was built for a different version of this page. Reload the page and try again.')
    log(`📦 downloading ${bundle.title} (${(published.flash / 1024).toFixed(1)} KB)`)
    const image = parseHex(await (await fetch(`firmware/${published.hex}`, { cache: 'no-cache' })).text())
    const at = findBlock(image)
    if (at < 0 || new DataView(image.buffer).getUint32(at + 8, true) !== layout.hash) throw new Error('The firmware file has no matching settings block.')
    image.set(bytes, at)
    log('🎛️ your pins and settings are written into the firmware')

    const port = await navigator.serial.requestPort()
    transport = new WebSerialTransport(port)
    await transport.open(BAUD[state.bootloader])
    await new Stk500(transport, { log }).flash(image, {
      onProgress: (p) => {
        const pct = Math.round(p * 100)
        bar.style.width = `${pct}%`
        $('.progress').setAttribute('aria-valuenow', String(pct))
      },
    })
    log('🚂 installed. Disconnect the USB cable whenever you like.')
  } catch (err) {
    if (err?.name === 'NotFoundError') log('No port chosen. Select Install again when the Nano is plugged in.')
    else log(`❌ ${err.message}`)
  } finally {
    await transport?.close()
    installing = false
    $('#install').disabled = false
  }
}

// copy / download
$('#copy-commands').addEventListener('click', async () => {
  const b = $('#copy-commands')
  try { await navigator.clipboard.writeText(commandsText()); b.textContent = 'Copied'; announce('Commands copied.') } catch { b.textContent = 'Select the text and copy it' }
  setTimeout(() => { b.textContent = 'Copy commands' }, 2000)
})
$('#download-sketch').addEventListener('click', () => {
  const a = document.createElement('a')
  a.href = URL.createObjectURL(new Blob([sketchText()], { type: 'text/plain' }))
  a.download = 'layout-effects.ino'
  a.click()
  setTimeout(() => URL.revokeObjectURL(a.href), 1000)
})
$('#bootloader-cmd').addEventListener('change', (ev) => { state.bootloader = ev.target.value; changed() })

changed({ structure: true })
