// 🧰 Layout Effects: a kit instruction sheet for an Arduino Nano.
// Five steps (pick, when it runs, wire, install, test) on the left;
// "Your Nano" (lettered parts, pin map, budget, problems) on the right.
import schema from './schema.js'
import { EFFECTS, byId, PINS, KEY_FIELDS, PIN_CHOICES, PWM_PINS, NO_PIN, pinName } from './effects.js'
import { defaultEntry, ordered, availableSources, encodeSettings, buildCommands, generateSketch } from './generate.js'
import { BUNDLES, bundleFor } from './bundles.js'
import { pinUses, checkPlan, assignPins, freeTriggerPin } from './pins.js'
import { buildKit, partsSummary, TESTS, TROUBLESHOOT, LED_FILL } from './wiring.js'
import { parseHex, findBlock } from './ihex.js'
import { Stk500, BAUD } from './stk500.js'
import { supported, WebSerialTransport } from './webserial.js'

const STEPS = [
  { id: 'pick', label: 'Pick effects' },
  { id: 'start', label: 'When it runs' },
  { id: 'wire', label: 'Wire it' },
  { id: 'install', label: 'Install' },
  { id: 'test', label: 'Test' },
]
const TIPS = {
  welder: 'Put the LEDs inside the shop and aim them at a wall so the flicker washes the room.',
  campfire: 'Bunch the three LEDs together and diffuse them with tissue or a dab of hot glue so the colors blend into one flame.',
  'tv-glow': 'Point the LEDs at the inside wall of the room, not out the window, so the glow washes the room like a real screen.',
  lightning: 'Hide the LEDs behind the backdrop, pointing at the sky, so you see the flash and not the LED.',
  crossing: 'Mount each sensor between the ties pointing up, and shade it from strong room light.',
  'crossing-gates': 'Set the gate angles in When it runs before gluing the arms on; two micro servos run fine from a good USB supply.',
  'traffic-light': 'For a four-way intersection, wire two heads in parallel per group, each LED with its own 470 Ω resistor.',
  'neon-sign': 'Put the LEDs behind a thin cut-out sign face or inside clear tubing so the light spreads like glass.',
  fluorescent: 'A whole building of fixtures on one pin needs a transistor or MOSFET: a pin gives about 20 mA.',
  beacon: 'Use a PWM pin for each lamp: the sweep is a fade.',
  sound: 'Name the files 0001.mp3, 0002.mp3 and so on, in a folder called mp3 on the card.',
  'day-night': 'A 10 kΩ resistor from each MOSFET gate to GND keeps the strips off while the Nano starts.',
}
const STORE = 'mra-kit-v1'
const $ = (sel, root = document) => root.querySelector(sel)
const esc = (s) => String(s).replace(/[&<>"]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]))

// ---------------------------------------------------------------- state 💾
let state = load()
let manifest = null
let installing = false
let lit = null                         // the wiring step being pointed at: { parts:Set, pins:Set }

function load() {
  const base = { name: 'My Nano', step: 'pick', effects: [], bootloader: 'new', checks: {}, visited: { pick: true }, installed: false }
  try {
    const saved = JSON.parse(localStorage.getItem(STORE) || localStorage.getItem('mra-configurator-v1') || 'null')
    if (!saved || !Array.isArray(saved.effects)) return base
    // merge onto fresh defaults so new settings appear and removed ones vanish
    const effects = saved.effects.filter((e) => byId[e?.id]).map((e) => {
      const d = defaultEntry(e.id)
      for (const cls of Object.keys(d.pins)) for (const k of Object.keys(d.pins[cls])) if (Number.isInteger(e.pins?.[cls]?.[k])) d.pins[cls][k] = e.pins[cls][k]
      for (const cls of Object.keys(d.cfg)) for (const k of Object.keys(d.cfg[cls])) if (e.cfg?.[cls]?.[k] !== undefined) d.cfg[cls][k] = e.cfg[cls][k]
      if (e.start) d.start = { ...d.start, ...e.start }
      if (e.source) d.source = e.source
      return d
    })
    return normalize({
      ...base, ...saved, effects,
      checks: saved.checks || {}, visited: saved.visited || { pick: true },
      step: STEPS.some((s) => s.id === saved.step) ? saved.step : 'pick',
      bootloader: saved.bootloader === 'old' ? 'old' : 'new',
    })
  } catch {
    return base
  }
}

function save() { try { localStorage.setItem(STORE, JSON.stringify(state)) } catch { /* private window: fine */ } }

function normalize(s) {
  const ids = s.effects.map((e) => e.id)
  for (const e of s.effects) if (e.source !== 'trigger' && !availableSources(e.id, ids).some((x) => x.id === e.source)) e.source = 'trigger'
  return s
}

function changed() {
  normalize(state)
  save()
  render()
}

let lastAnnounced = ''
function announce(text) {
  if (text === lastAnnounced) return
  lastAnnounced = text
  $('#announcer').textContent = text
}

// --------------------------------------------------------------- render 🖨️
function render() {
  const keep = document.activeElement?.id
  const kit = buildKit(state.effects)
  renderSteps(kit)
  renderSheet(kit)
  renderNano(kit)
  const el = keep && document.getElementById(keep)
  if (el && document.activeElement !== el) el.focus({ preventScroll: true })
  const problems = checkPlan(state.effects)
  if (state.effects.length) announce(problems.length ? `Pin plan: ${problems.length} problem${problems.length > 1 ? 's' : ''} to fix.` : 'Pin plan is good.')
}

function stepDone(id, kit) {
  if (id === 'pick') return state.effects.length > 0
  if (id === 'wire') return kit.parts.length > 0 && wiringKeys(kit).every((k) => state.checks[k])
  if (id === 'start') return !!state.visited.wire && state.effects.length > 0
  if (id === 'install') return state.installed
  if (id === 'test') return state.effects.length > 0 && testKeys().every((k) => state.checks[k])
  return false
}

function go(step) {
  state.visited[step] = true
  state.step = step
  lit = null
  save()
  render()
  $('#sheet').focus({ preventScroll: true })
  window.scrollTo({ top: 0, behavior: matchMedia('(prefers-reduced-motion: reduce)').matches ? 'auto' : 'smooth' })
}

function renderSteps(kit) {
  $('#steps').innerHTML = STEPS.map((s, i) => {
    const done = stepDone(s.id, kit)
    const cur = state.step === s.id
    const mark = done && !cur ? '<svg width="14" height="14" viewBox="0 0 14 14" aria-hidden="true"><path d="M3 7.5l2.5 2.5L11 4" stroke="currentColor" stroke-width="2" fill="none"/></svg>' : i + 1
    return `<button type="button" class="step-tab${done ? ' is-done' : ''}" data-step="${s.id}" ${cur ? 'aria-current="step"' : ''}><span class="d">${mark}</span><span class="label">${s.label}${done && !cur ? '<span class="visually-hidden"> (done)</span>' : ''}</span></button>`
  }).join('')
  $('#steps').querySelectorAll('.step-tab').forEach((b) => b.addEventListener('click', () => go(b.dataset.step)))
}

function renderSheet(kit) {
  if (state.step === 'install' && installing) return      // keep the running install on screen
  const sheet = $('#sheet')
  const html = { pick: pickSheet, wire: wireSheet, start: startSheet, install: installSheet, test: testSheet }[state.step](kit)
  sheet.innerHTML = html + footer(kit)
  wireUp(sheet)
}

function footer(kit) {
  const i = STEPS.findIndex((s) => s.id === state.step)
  const back = i > 0 ? `<button type="button" class="btn" data-go="${STEPS[i - 1].id}">Back</button>` : ''
  const next = STEPS[i + 1]
  let prog = `Step ${i + 1} of ${STEPS.length}`
  if (state.step === 'wire' && state.effects.length) {
    const keys = wiringKeys(kit)
    prog = `${keys.filter((k) => state.checks[k]).length} of ${keys.length} wiring steps done`
  }
  // one yellow action per screen: on Install, the install button owns it
  const installOwnsYellow = state.step === 'install' && installReady().ready
  const nextBtn = next
    ? `<button type="button" class="btn${installOwnsYellow ? '' : ' primary'}" data-go="${next.id}" ${state.step === 'pick' && !state.effects.length ? 'disabled' : ''}>Next: ${next.label}</button>`
    : ''
  return `<div class="spacer"></div><div class="actions">${back}<span class="prog">${prog}</span>${nextBtn}</div>`
}

function emptySheet(title, text) {
  return `<h1>${title}</h1><p class="lede">${text}</p><p><button type="button" class="btn" data-go="pick">Pick an effect</button></p>`
}

// ---------------------------------------------------------- 1 pick 🧺
function pickSheet() {
  const ids = state.effects.map((e) => e.id)
  const rows = EFFECTS.map((fx) => {
    const on = ids.includes(fx.id)
    const replaces = !on && (fx.excludes || []).find((x) => ids.includes(x))
    return `<li class="${on ? 'is-on' : ''}">
      <div><h2>${esc(fx.title)}</h2><p>${esc(fx.blurb)}</p><p class="needs">Needs: ${esc(partsSummary(fx.id))}${replaces ? `. Replaces the ${esc(byId[replaces].title.toLowerCase())}.` : ''}</p></div>
      <button type="button" class="btn small" id="add-${fx.id}" data-toggle="${fx.id}" aria-pressed="${on}">${on ? 'Remove' : 'Add to board'}</button>
    </li>`
  }).join('')
  return `<h1>Pick your effects</h1>
    <p class="lede"><b>Start with one.</b> Each effect runs on its own Arduino Nano, and several can share one board.</p>
    <ul class="catalog">${rows}</ul>`
}

// ----------------------------------------------------------- 3 wire 🔌
function wiringKeys(kit) {
  const keys = []
  for (const sec of kit.sections) sec.steps.forEach((s, i) => { if (!s.optional) keys.push(`wire:${sec.id}:${i}`) })
  if (kit.parts.length) keys.push('wire:gnd', 'wire:usb')
  return keys
}

function wireSheet(kit) {
  if (!state.effects.length) return emptySheet('Wire it', 'Pick an effect first, and its wiring appears here step by step.')
  const entries = ordered(state.effects)
  const total = kit.parts.reduce((a, p) => a + (p.qty || 0), 0)
  const single = entries.length === 1
  let n = 0
  gndStepNumber = kit.sections.reduce((a, sec) => a + sec.steps.length, 0) + 1
  const sections = kit.sections.map((sec) => {
    const e = entries.find((x) => x.id === sec.id)
    const fx = byId[sec.id]
    const items = sec.steps.map((s, i) => wireItem(++n, s, `wire:${sec.id}:${i}`, e, kit)).join('')
    const head = single ? '<h2 class="visually-hidden">Wiring steps</h2>' : `<h2 class="sec">${esc(fx.title)} <small>${esc(partsSummary(fx.id))}</small></h2>`
    return `${head}<ol class="wire">${items}</ol>`
  }).join('')
  const ledKeys = kit.parts.filter((p) => p.kind === 'led').map((p) => ({ part: p.sku }))
  const otherGnd = kit.sections.some((sec) => sec.steps.some((st) => st.tokens.some((t) => t.fixed === 'GND')))
  const gndTokens = ledKeys.length
    ? ['Twist the short legs of', ...ledKeys, 'onto one', { part: 'wire' }, `wire${otherGnd ? ', with every other GND wire,' : ''} and run it to the Nano’s`, { fixed: 'GND' }, 'pin']
    : ['Join every GND wire with', { part: 'wire' }, 'to the Nano’s', { fixed: 'GND' }, 'pin']
  const common = `${single ? '' : '<h2 class="sec">Finish</h2>'}<ol class="wire">
    ${wireItem(++n, { title: 'Join the grounds', tokens: gndTokens, parts: ['wire', ...ledKeys.map((t) => t.part)] }, 'wire:gnd', null, kit)}
    ${wireItem(++n, { title: 'Plug in USB', tokens: ['Plug the Nano into USB. Nothing lights yet; that’s normal until it’s installed.'], parts: [] }, 'wire:usb', null, kit)}
  </ol>`
  const title = single ? `Wire the ${esc(byId[entries[0].id].title.toLowerCase())}` : `Wire ${esc(state.name)}`
  const tips = entries.map((e) => TIPS[e.id]).filter(Boolean)
  return `<h1>${title}</h1>
    <p class="lede"><b>${total} parts, about ${Math.max(10, n * 3)} minutes.</b> Long LED leg = +. Letters match the parts list.</p>
    ${legend(kit)}${sections}${common}
    ${tips.map((t) => `<div class="tip"><b>Tip</b><span>${esc(t)}</span></div>`).join('')}`
}

function wireItem(num, s, key, e, kit) {
  const checked = !!state.checks[key]
  const pins = new Set()
  const tokens = s.tokens.map((t) => {
    if (typeof t === 'string') return esc(t)
    if (t.part) return `<i class="k" title="${esc(kit.parts.find((p) => p.sku === t.part)?.name || '')}">${kit.letters.get(t.part)}</i>`
    if (t.fixed) { pins.add(t.fixed); return `<span class="pin">${t.fixed}</span>` }
    if (t.groundWire) { pins.add('GND'); return `<span class="nowrap">ground wire <i class="k">${kit.letters.get('wire')}</i> (step ${gndStepNumber})</span>` }
    if (t.trigger) { pins.add(e.start.pin); return pinSelect(`trig-${e.id}`, e.start.pin, false, `${byId[e.id].title} trigger pin`) }
    const [cls, name] = t.pin
    const v = e.pins[cls][name]
    pins.add(v)
    return pinSelect(`pin-${e.id}-${cls}-${name}`, v, PINS[cls][name][2], `${byId[e.id].title}: ${PINS[cls][name][0]} pin`)
  }).join(' ')
  const opt = s.optional ? ' <span class="opt">optional</span>' : ''
  return `<li class="${checked ? 'is-done' : ''}${s.optional ? ' is-optional' : ''}" data-parts="${s.parts.map((p) => kit.letters.get(p)).join(' ')}" data-pins="${[...pins].join(' ')}">
    <span class="n">${num}</span>
    <div><h3>${esc(s.title)}${opt}</h3><p class="path">${tokens}</p>${s.note ? `<p class="note">${esc(s.note)}</p>` : ''}</div>
    ${s.optional ? '<span></span>' : `<label class="done"><input type="checkbox" id="chk-${key}" data-check="${key}" ${checked ? 'checked' : ''}> Done</label>`}
  </li>`
}

let gndStepNumber = 0

// on narrow screens the parts list is far below, so the letters get a key right here
function legend(kit) {
  return `<ul class="legend" aria-label="Parts key">${kit.parts.map((p) => `<li><i class="k">${p.key}</i>${partArt(p)}<span>${esc(p.name.replace(/, (3 mm|12 V)$/, ''))}</span>${p.qty ? `<b>\u00d7${p.qty}</b>` : ''}</li>`).join('')}</ul>`
}

function pinSelect(id, value, optional, label) {
  const bad = new Set(checkPlan(state.effects).map((p) => p.pin))
  const opts = [...(optional ? [NO_PIN] : []), ...PIN_CHOICES]
    .map((p) => `<option value="${p}" ${p === value ? 'selected' : ''}>${p === NO_PIN ? 'not used' : pinName(p) + (PWM_PINS.includes(p) ? ' ~' : '')}</option>`).join('')
  return `<select class="pin-pick${bad.has(value) ? ' is-bad' : ''}" id="${id}" aria-label="${esc(label)}">${opts}</select>`
}

// ---------------------------------------------------- 2 when it runs ⏱️
function startSheet() {
  if (!state.effects.length) return emptySheet('When it runs', 'Pick an effect first, then choose here when it should run.')
  const ids = state.effects.map((e) => e.id)
  const blocks = ordered(state.effects).map((e) => {
    const fx = byId[e.id]
    const sources = availableSources(e.id, ids)
    const crossing = fx.kind === 'crossing'
    const modes = crossing
      ? [['POWER_UP', 'Sensors and button only', 'The crossing is always armed by its own sensors and button.'], ['WHILE_TRIGGERED', 'Also fire it from a pin', 'A DEJA.js IO pin or detection block runs the flashers while it’s on.'], ['TOGGLE', 'Also a button that toggles it', 'A push button starts and stops the flashers.']]
      : [['POWER_UP', 'As soon as the Nano has power', 'No extra wiring.'], ['WHILE_TRIGGERED', 'While a pin is switched on', 'A DEJA.js IO pin, a switch, a relay or a sensor.'], ['TOGGLE', 'Press a button to start and stop', 'A push button between a pin and GND.']]
    const linkChoices = sources.map((s) => radio(e, `src:${s.id}`, s.label, 'Another effect on this board switches it; no wiring needed.', e.source === s.id))
    const modeChoices = modes.map(([m, label, small]) => radio(e, m, label, small, e.source === 'trigger' && e.start.mode === m))
    const trig = e.source === 'trigger' && e.start.mode !== 'POWER_UP'
      ? `<div class="row">
          <label class="field">Trigger pin ${pinSelect(`trig-${e.id}`, e.start.pin, false, `${fx.title} trigger pin`)}</label>
          <label class="field">What's connected
            <select id="pol-${e.id}" data-pol="${e.id}">
              <option value="high" ${!e.start.activeLow ? 'selected' : ''}>DEJA.js pin or switch to 5 V (HIGH = on)</option>
              <option value="low" ${e.start.activeLow ? 'selected' : ''}>Button, relay or IR sensor to GND</option>
            </select></label>
          ${e.start.mode === 'WHILE_TRIGGERED' ? `<label class="field">Keep running after release (s)<input type="number" id="runon-${e.id}" data-runon="${e.id}" min="0" max="86400" step="1" value="${e.start.runOnMs / 1000}"><small>0 stops at once. 60 makes a button press run a minute.</small></label>` : ''}
        </div>`
      : ''
    const off = crossing ? '' : `<p class="note">When it’s off: ${esc(fx.offText)}.</p>`
    const adds = e.source === 'trigger' && e.start.mode !== 'POWER_UP'
      ? `<p class="notice"><b>Wiring</b><span>Wire it, next, includes a step for this: ${e.start.mode === 'TOGGLE' ? `a <b>push button</b> between ${pinName(e.start.pin)} and GND` : e.start.activeLow ? `a button, relay or sensor between ${pinName(e.start.pin)} and GND` : `your DEJA.js pin or switch to ${pinName(e.start.pin)}`}.</span></p>`
      : ''
    return `<section class="effect-block">
      <h2 class="sec">${esc(fx.title)}</h2>
      <fieldset><legend>${crossing ? 'The sensors and button always work. Anything else?' : `When should the ${esc(fx.title.toLowerCase())} run?`}</legend>
        <div class="choices">${[...linkChoices, ...modeChoices].join('')}</div>
      </fieldset>
      ${trig}${adds}${off}
      ${settingsBlock(e)}
    </section>`
  }).join('')
  return `<h1>When it runs</h1>
    <p class="lede"><b>Choose how each effect starts,</b> then adjust it if you like. The defaults look right on most layouts.</p>${blocks}`
}

function radio(e, value, label, small, checked) {
  const id = `mode-${e.id}-${value.replace(':', '-')}`
  return `<label class="choice" for="${id}"><input type="radio" id="${id}" name="mode-${e.id}" value="${value}" data-mode="${e.id}" ${checked ? 'checked' : ''}><b>${esc(label)}</b><small>${esc(small)}</small></label>`
}

// settings: the key ones, then everything else
const INDEX_FIELDS = { brokenSegment: 'Segment', badTube: 'Tube' }
const RANGES = { uint8_t: [0, 255], int8_t: [-128, 127], uint16_t: [0, 65535], int16_t: [-32768, 32767], int: [-32768, 32767], uint32_t: [0, 4294967295], int32_t: [-2147483648, 2147483647], float: [-1e6, 1e6] }
function humanize(name) {
  const units = { Ms: 'ms', Percent: '%', Deg: '°', Us: 'µs' }
  let unit = '', base = name
  for (const [suffix, u] of Object.entries(units)) if (base.endsWith(suffix)) { unit = u; base = base.slice(0, -suffix.length); break }
  const words = base.replace(/([a-z0-9])([A-Z])/g, '$1 $2').replace(/([A-Z])([A-Z][a-z])/g, '$1 $2').toLowerCase().replace(/\blvl\b/g, 'level')
  const text = words.charAt(0).toUpperCase() + words.slice(1)
  return unit ? `${text} (${unit})` : text
}
const enumLabel = (v) => { const t = v.replace(/^[A-Z]+_/, '').replace(/_/g, ' ').toLowerCase(); return t.charAt(0).toUpperCase() + t.slice(1) }

// xMin must not be above xMax (and firstTrack above lastTrack)
function rangeError(cfg, name) {
  const pair = name.endsWith('Max') ? [name.slice(0, -3) + 'Min', name] : name === 'lastTrack' ? ['firstTrack', 'lastTrack'] : null
  if (!pair || cfg[pair[0]] === undefined || cfg[pair[1]] === undefined) return ''
  return cfg[pair[0]] > cfg[pair[1]] ? `Must be at least ${humanize(pair[0]).toLowerCase()}.` : ''
}

function settingsBlock(e) {
  const fx = byId[e.id]
  return fx.objects.map((o) => {
    const fields = schema.classes[o.cls].fields
    const key = KEY_FIELDS[o.cls] || []
    const label = fx.objects.length > 1 ? (o.cls === 'CrossingGates' ? 'Adjust the gates' : 'Adjust the flashers') : `Adjust the ${fx.title.toLowerCase()}`
    const keyHtml = fields.filter((f) => key.includes(f.name)).map((f) => field(e, o.cls, f)).join('')
    const rest = fields.filter((f) => !key.includes(f.name))
    const open = state.openSettings?.[`${e.id}:${o.cls}`] ? 'open' : ''
    return `<details class="more" data-open-key="${e.id}:${o.cls}" ${open}><summary>${label}</summary>
      <div class="grid">${keyHtml}</div>
      ${rest.length ? `<details class="more"><summary>All settings (${rest.length})</summary><div class="grid">${rest.map((f) => field(e, o.cls, f)).join('')}</div></details>` : ''}
    </details>`
  }).join('')
}

function field(e, cls, f) {
  const id = `cfg-${e.id}-${cls}-${f.name}`
  const v = e.cfg[cls][f.name]
  const help = f.help ? `<small>${esc(f.help)}</small>` : ''
  const data = `data-cfg="${e.id}|${cls}|${f.name}"`
  if (INDEX_FIELDS[f.name]) {
    const opts = [[NO_PIN, 'None'], ...[0, 1, 2, 3].map((i) => [i, `${INDEX_FIELDS[f.name]} ${i + 1}`])].map(([x, t]) => `<option value="${x}" ${x === v ? 'selected' : ''}>${t}</option>`).join('')
    return `<label class="field">${humanize(f.name)}<select id="${id}" ${data} data-kind="int">${opts}</select>${help}</label>`
  }
  if (schema.enums[f.type]) {
    const opts = schema.enums[f.type].map((x) => `<option value="${x}" ${x === v ? 'selected' : ''}>${enumLabel(x)}</option>`).join('')
    return `<label class="field">${humanize(f.name)}<select id="${id}" ${data} data-kind="enum">${opts}</select>${help}</label>`
  }
  if (f.type === 'bool') return `<label class="field field--check"><input type="checkbox" id="${id}" ${data} data-kind="bool" ${v ? 'checked' : ''}>${humanize(f.name)}${help}</label>`
  const [min, max] = RANGES[f.type]
  const err = rangeError(e.cfg[cls], f.name)
  return `<label class="field${err ? ' is-bad' : ''}">${humanize(f.name)}<input type="number" id="${id}" ${data} data-kind="${f.type === 'float' ? 'float' : 'int'}" min="${min}" max="${max}" step="${f.type === 'float' ? 'any' : 1}" value="${v}" ${err ? `aria-invalid="true" aria-describedby="${id}-err"` : ''}>${err ? `<span class="err" id="${id}-err">${err}</span>` : help}</label>`
}

// ------------------------------------------------------------ 4 install ⚡
function installReady() {
  const ids = state.effects.map((e) => e.id)
  const bundle = bundleFor(ids)
  const problems = checkPlan(state.effects)
  if (!state.effects.length) return { ready: false, why: 'Pick an effect first.' }
  if (!supported()) return { ready: false, bundle, why: 'Installing from this page needs Chrome or Edge on a desktop computer. You can build it yourself below on any computer.' }
  if (!bundle) return { ready: false, bundle, why: `There’s no ready-made firmware for this mix of effects. One-click install covers any single effect and four sets (${BUNDLES.filter((b) => b.effects.length > 1).map((b) => b.title).join(', ')}). Build this one yourself below.` }
  if (problems.length) return { ready: false, bundle, bad: true, why: 'Fix the pin problems shown under Your Nano before installing.' }
  return { ready: true, bundle }
}

const folderName = () => (state.name || 'layout-effects').toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/^-|-$/g, '') || 'layout-effects'

function bootloaderSelect(id) {
  return `<label class="field">Bootloader<select id="${id}" data-boot>
    <option value="new" ${state.bootloader === 'new' ? 'selected' : ''}>Nano (new bootloader)</option>
    <option value="old" ${state.bootloader === 'old' ? 'selected' : ''}>Nano clone (old bootloader)</option></select></label>`
}

function installSheet() {
  if (!state.effects.length) return emptySheet('Install', 'Pick an effect first, then install it here.')
  const r = installReady()
  const one = r.ready ? `
    <ol class="numbered">
      <li>Plug the Nano into this computer.</li>
      <li>Pick its bootloader: genuine Nanos use the new one, many clones the old one.</li>
      <li>Select <b>Install</b>, then choose the Nano when the browser asks (often “USB Serial” or “CH340”).</li>
    </ol>
    <div class="row">${bootloaderSelect('boot-install')}<button type="button" class="btn primary" id="install">Install ${esc(r.bundle.title)}</button></div>
    <div class="progress" role="progressbar" aria-label="Install progress" aria-valuemin="0" aria-valuemax="100" aria-valuenow="0" hidden><i></i></div>
    <pre class="log" id="install-log" role="log" aria-live="polite"></pre>` : `<p class="notice${r.bad ? ' is-bad' : ''}"><b>${r.bad ? 'Not yet' : 'Heads up'}</b><span>${esc(r.why)}</span></p>`
  const name = folderName()
  const diy = `
    <h2 class="sec">Build it yourself <small>any computer with arduino-cli and git</small></h2>
    <p class="hint">Paste into a terminal on macOS or Linux. On Windows, download the sketch and open it in the Arduino IDE with this library installed.</p>
    <div class="row">${bootloaderSelect('boot-diy')}<button type="button" class="btn small" id="copy">Copy commands</button><button type="button" class="link-btn" id="download">Download ${esc(name)}.ino</button></div>
    <pre class="code"><code>${esc(buildCommands(state, state.bootloader, { name }))}</code></pre>`
  return `<h1>Install</h1>
    <p class="lede"><b>${r.ready ? 'One click puts your board on the Nano.' : 'Two ways to get it onto the Nano.'}</b> Your pins and settings go with it.</p>
    ${one}${r.ready ? `<details class="more"><summary>Build it yourself instead</summary>${diy}</details>` : diy}`
}

const errorText = (err) => {
  const m = err?.message || String(err)
  if (err?.name === 'NotFoundError') return 'No port was chosen. Select Install again with the Nano plugged in.'
  if (err?.name === 'InvalidStateError' || /already open/i.test(m)) return 'The Nano is busy. Close the Arduino IDE or any serial monitor, then try again.'
  if (err?.name === 'NetworkError' || /Failed to open/i.test(m)) return 'The port couldn’t be opened. Unplug the Nano, plug it back in, and try again.'
  if (err?.name === 'AbortError' || /Failed to fetch|NetworkError when/i.test(m)) return 'The firmware couldn’t be downloaded. Check your internet connection and try again.'
  return m
}

async function fetchOk(url, ms = 20000) {
  const ctl = new AbortController()
  const t = setTimeout(() => ctl.abort(), ms)
  try {
    const res = await fetch(url, { cache: 'no-cache', signal: ctl.signal })
    if (!res.ok) throw new Error(`The firmware isn’t available right now (${res.status}). Try again later, or build it yourself below.`)
    return res
  } finally { clearTimeout(t) }
}

async function install() {
  const logEl = $('#install-log')
  const log = (line) => { logEl.textContent += line + '\n' }
  const bar = $('.progress')
  logEl.textContent = ''
  installing = true
  $('#install').disabled = true
  bar.hidden = false
  let transport
  try {
    const bundle = bundleFor(state.effects.map((e) => e.id))
    manifest = await (await fetchOk('firmware/manifest.json')).json()
    const published = manifest.bundles.find((b) => b.id === bundle.id)
    if (!published) throw new Error(`The ${bundle.title} firmware isn’t published yet. Build it yourself below.`)
    const { bytes, layout } = encodeSettings(state)
    if (published.hash !== layout.hash) throw new Error('The published firmware is from a different version of this page. Reload the page and try again.')
    log(`Downloading ${bundle.title}`)
    const image = parseHex(await (await fetchOk(`firmware/${published.hex}`)).text())
    const at = findBlock(image)
    if (at < 0 || new DataView(image.buffer).getUint32(at + 8, true) !== layout.hash) throw new Error('The firmware file has no matching settings block. Reload the page and try again.')
    image.set(bytes, at)
    log('Your pins and settings are written into the firmware')
    const port = await navigator.serial.requestPort()
    transport = new WebSerialTransport(port)
    await transport.open(BAUD[state.bootloader])
    await new Stk500(transport, { log: (l) => log(l.replace(/^[^\p{L}]+/u, '')) }).flash(image, {
      onProgress: (p) => { const v = Math.round(p * 100); bar.firstElementChild.style.width = `${v}%`; bar.setAttribute('aria-valuenow', String(v)) },
    })
    log('Installed. Next, test it.')
    state.installed = true
    save()
    announce('Installed. Next, test it.')
  } catch (err) {
    log(`Didn’t install: ${errorText(err)} It’s safe to try again; the Nano can always be re-installed.`)
    announce('Install didn’t finish. See the message under the Install button.')
  } finally {
    await transport?.close()
    installing = false
    const btn = $('#install')
    if (btn) btn.disabled = false
    renderSteps(buildKit(state.effects))
  }
}

// --------------------------------------------------------------- 5 test ✅
function testKeys() {
  const keys = []
  for (const e of ordered(state.effects)) (TESTS[e.id] || []).forEach((_, i) => keys.push(`test:${e.id}:${i}`))
  return keys
}

function testSheet() {
  if (!state.effects.length) return emptySheet('Test it', 'Pick, wire and install an effect, then check it here.')
  const blocks = ordered(state.effects).map((e) => {
    const items = (TESTS[e.id] || []).map((t, i) => {
      const key = `test:${e.id}:${i}`
      return `<li><input type="checkbox" id="chk-${key}" data-check="${key}" ${state.checks[key] ? 'checked' : ''}><label for="chk-${key}">${esc(t)}</label></li>`
    }).join('')
    return `<h2 class="sec">${esc(byId[e.id].title)}</h2><ul class="checks">${items}</ul>`
  }).join('')
  return `<h1>Test it</h1>
    <p class="lede"><b>Power the Nano and watch.</b> Tick each thing you see.</p>
    ${blocks}
    <h2 class="sec">If something’s wrong</h2>
    <dl class="trouble">${TROUBLESHOOT.map((t) => `<dt>${esc(t.q)}</dt><dd>${esc(t.a)}</dd>`).join('')}</dl>`
}

// ------------------------------------------------------------ Your Nano 📟
// the real board, USB on the left: top and bottom header rows
const TOP = ['D13', '3V3', 'REF', 'A0', 'A1', 'A2', 'A3', 'A4', 'A5', 'A6', 'A7', '5V', 'RST', 'GND', 'VIN']
const BOTTOM = ['D12', 'D11', 'D10', 'D9', 'D8', 'D7', 'D6', 'D5', 'D4', 'D3', 'D2', 'GND', 'RST', 'RX0', 'TX1']
const pinNumber = (label) => (/^D(\d+)$/.test(label) ? Number(label.slice(1)) : /^A[0-5]$/.test(label) ? 14 + Number(label[1]) : null)

function partArt(p) {
  const s = (inner) => `<svg width="76" height="44" viewBox="0 0 76 44" class="art" fill="none" stroke="currentColor" stroke-width="1.5" aria-hidden="true">${inner}</svg>`
  const BANDS = { 220: ['#c8322b', '#c8322b', '#7b4a26'], 150: ['#7b4a26', '#2e7d4b', '#7b4a26'], 1000: ['#7b4a26', '#141414', '#c8322b'], 470: ['#e2b313', '#7a3fa0', '#7b4a26'] }
  switch (p.kind) {
    case 'led': return s(`<path d="M6 22a10 10 0 0 1 10-10h12v20H16A10 10 0 0 1 6 22z" fill="${LED_FILL[p.color] || '#ffffff'}"/><path d="M28 9h4v26h-4z" class="pf"/><path d="M32 17h40M32 27h28"/><text x="66" y="12" font-size="11" fill="currentColor" stroke="none">+</text>`)
    case 'res': return s(`<path d="M2 22h16M58 22h16"/><rect x="18" y="14" width="40" height="16" rx="7" class="res"/>${(BANDS[p.ohms] || []).map((c, i) => `<rect x="${24 + i * 7}" y="14" width="3" height="16" fill="${c}" stroke="none"/>`).join('')}<rect x="47" y="14" width="3" height="16" fill="#b8963a" stroke="none"/>`)
    case 'ir': return s('<rect x="10" y="10" width="44" height="24" rx="2" class="pb"/><circle cx="20" cy="22" r="5" class="pf"/><circle cx="34" cy="22" r="5" class="pi"/><path d="M54 16h16M54 22h16M54 28h16"/>')
    case 'button': return s('<rect x="24" y="10" width="28" height="24" rx="3" class="pf"/><circle cx="38" cy="22" r="7" class="pi"/><path d="M24 14h-8M24 30h-8M52 14h8M52 30h8"/>')
    case 'servo': return s('<rect x="10" y="14" width="40" height="20" rx="2" class="pb"/><circle cx="24" cy="14" r="4" class="pf"/><path d="M24 14l26-8"/><path d="M50 26h20" stroke-width="3"/>')
    case 'strip': return s(`<rect x="4" y="16" width="68" height="12" rx="2" fill="${{ day: '#ffffff', warm: '#fbe2a6', night: '#d6e1f3' }[p.color]}"/>${[14, 30, 46, 62].map((x) => `<rect x="${x - 4}" y="19" width="8" height="6" fill="currentColor" stroke="none"/>`).join('')}`)
    case 'mosfet': return s('<rect x="18" y="8" width="40" height="28" rx="2" class="pf"/><path d="M28 36v6M38 36v6M48 36v6"/><circle cx="38" cy="18" r="4"/>')
    case 'supply': return s('<rect x="8" y="10" width="40" height="24" rx="4" class="pf"/><path d="M48 22h22"/><text x="15" y="26" font-size="11" fill="currentColor" stroke="none">12 V</text>')
    case 'dfplayer': return s('<rect x="14" y="6" width="48" height="32" rx="2" class="pb"/><rect x="24" y="12" width="28" height="14" class="pf"/><path d="M18 38v4M26 38v4M34 38v4M42 38v4M50 38v4M58 38v4"/>')
    case 'speaker': return s('<circle cx="38" cy="22" r="16" class="pf"/><circle cx="38" cy="22" r="7" class="pi"/>')
    case 'sd': return s('<path d="M28 8h18l6 6v22H28z" class="pf"/><path d="M33 8v6M38 8v6M43 8v6"/>')
    case 'wire': return s('<path d="M12 32C26 6 44 42 62 14" stroke-width="4" stroke-linecap="round"/><path d="M12 32l-6 6M62 14l6-6"/>')
    default: return s('')
  }
}

function renderNano(kit) {
  const entries = ordered(state.effects)
  const uses = pinUses(state.effects)
  const problems = checkPlan(state.effects)
  const bad = new Set(problems.map((p) => p.pin))
  // the lettered part on each pin, from the wiring steps
  const partOnPin = new Map()
  for (const sec of kit.sections) {
    const e = entries.find((x) => x.id === sec.id)
    for (const s of sec.steps) {
      const key = s.parts.filter((p) => !p.startsWith('res')).map((p) => kit.letters.get(p))[0]
      for (const t of s.tokens) {
        const pin = t.pin ? e.pins[t.pin[0]][t.pin[1]] : t.trigger ? e.start.pin : null
        if (pin != null && !partOnPin.has(pin)) partOnPin.set(pin, key)
      }
    }
  }
  const used = new Set(uses.map((u) => u.pin))
  const hole = (label, x, y, ty) => {
    const n = pinNumber(label)
    const on = (n != null && used.has(n)) || (label === 'GND' && kit.parts.length > 0) || (label === '5V' && kit.uses5V)
    const isBad = n != null && bad.has(n)
    const isLit = lit && (n != null ? lit.pins.has(n) : lit.pins.has(label))
    const pwm = PWM_PINS.includes(n) ? '~' : ''
    const cls = ['pinhole', on && 'is-used', isBad && 'is-bad', isLit && 'is-lit'].filter(Boolean).join(' ')
    const textCls = isBad ? 'is-bad' : on ? 'is-used' : /^A[67]$/.test(label) ? 'no-io' : ''
    return `<circle class="${cls}" cx="${x}" cy="${y}" r="6.5" data-pin="${n != null ? n : label}"/><text x="${x}" y="${ty}" text-anchor="middle" class="${textCls}">${pwm}${label}</text>`
  }
  const map = `<svg class="nano-map" viewBox="0 0 488 132" role="img" aria-label="Nano pin map. ${uses.length ? `In use: ${esc(uses.map((u) => `${pinName(u.pin)} ${u.role}`).join(', '))}.` : 'No pins in use yet.'}">
    <rect class="board" x="34" y="28" width="440" height="76" rx="5" stroke-width="2"/>
    <rect class="board" x="12" y="48" width="30" height="36" rx="3" stroke-width="2"/>
    <text class="usb-label" x="27" y="70" font-size="9" transform="rotate(-90 27 70)" text-anchor="middle">USB</text>
    <rect class="board" x="240" y="56" width="28" height="20" rx="2" stroke-width="1.2"/>
    ${TOP.map((l, i) => hole(l, 58 + i * 28, 42, 18)).join('')}
    ${BOTTOM.map((l, i) => hole(l, 58 + i * 28, 90, 124)).join('')}
  </svg>`
  const usesList = uses.length
    ? `<ul class="uses">${uses.map((u) => `<li><span class="pin">${pinName(u.pin)}</span>${partOnPin.get(u.pin) ? `<i class="k">${partOnPin.get(u.pin)}</i> ` : ''}${esc(u.role)}${entries.length > 1 ? ` <span class="who">${esc(byId[u.effect].title)}</span>` : ''}</li>`).join('')}${kit.parts.length ? '<li><span class="pin">GND</span>Common ground</li>' : ''}</ul>`
    : ''
  const tree = kit.parts.length
    ? `<div class="tree">${kit.parts.map((p) => `<div class="part${lit?.parts.has(p.key) ? ' is-lit' : ''}" data-key="${p.key}"><i class="k">${p.key}</i>${partArt(p)}<p><span>${esc(p.name)}</span><b>${p.qty ? `×${p.qty}` : ''}</b></p></div>`).join('')}</div>
       <p class="also">Also needed: <b>Arduino Nano ×1</b> and a <b>USB cable</b> (Mini-B or USB-C, to match your Nano)</p>`
    : '<p class="empty-tree">Pick an effect and its lettered parts list appears here.</p>'

  // 📏 budget: PWM pins, pins, flash
  const servo = entries.some((e) => byId[e.id].usesServoTimer)
  const pwmCap = servo ? 4 : 6
  const pwmUsed = new Set(uses.filter((u) => u.fades).map((u) => u.pin)).size
  const flash = flashEstimate()
  const meter = (label, value, cap, text) => `<div class="${value > cap ? 'is-over' : ''}">${label}<b>${text}</b><i style="--v:${Math.min(100, Math.round((value / cap) * 100))}%"></i></div>`
  const meters = `<div class="meters" aria-label="Budget">
    ${meter('PWM pins', pwmUsed, pwmCap, `${pwmUsed} of ${pwmCap}`)}
    ${meter('Pins', used.size, 18, `${used.size} of 18`)}
    ${flash ? meter('Flash', flash.kb, 30, `${flash.exact ? '' : 'about '}${flash.kb.toFixed(1)} of 30 KB`) : '<div>Flash<b>—</b><i style="--v:0%"></i></div>'}
  </div>`
  const status = !entries.length ? '' : problems.length
    ? `<div class="status is-bad"><svg width="24" height="24" viewBox="0 0 24 24" aria-hidden="true" style="color: var(--bad)"><circle cx="12" cy="12" r="10.5" fill="none" stroke="currentColor" stroke-width="2"/><path d="M12 6.5v7M12 16.5v1" stroke="currentColor" stroke-width="2.4" stroke-linecap="round"/></svg><div><b>${problems.length} pin problem${problems.length > 1 ? 's' : ''}</b><ul>${problems.map((p) => `<li>${esc(p.text)}</li>`).join('')}</ul></div></div>`
    : `<div class="status"><svg width="24" height="24" viewBox="0 0 24 24" aria-hidden="true" style="color: var(--ok)"><circle cx="12" cy="12" r="10.5" fill="none" stroke="currentColor" stroke-width="2"/><path d="M7 12.5l3.2 3.2L17 9" stroke="currentColor" stroke-width="2.2" fill="none"/></svg><span><b>No problems.</b> Pin plan is good.</span></div>`
  const names = entries.map((e) => byId[e.id].title)
  $('#nano').innerHTML = `
    <div class="ahead"><h2 id="nano-title">Your Nano</h2><span>${names.length ? esc(names.join(', ')) : 'Nothing on it yet'}</span></div>
    ${problems.length ? status : ''}
    <section><div class="sh"><h3>Parts</h3><span>${kit.parts.length ? 'Letters match the wiring steps' : ''}</span></div>${tree}</section>
    <section><div class="sh"><h3>Pin map</h3><span>~ can fade (PWM), filled = in use, A6 and A7 analog in only</span></div>${map}${usesList}</section>
    ${entries.length ? meters : ''}${problems.length ? '' : status}`
}

function flashEstimate() {
  if (!manifest || !state.effects.length) return null
  const ids = state.effects.map((e) => e.id)
  const exact = bundleFor(ids)
  const find = (id) => manifest.bundles.find((b) => b.id === id)
  if (exact && find(exact.id)) return { kb: find(exact.id).flash / 1024, exact: true }
  const CORE = 1500                    // the Arduino core and startup code every build shares
  const sizes = ids.map((id) => find(id)?.flash)
  if (sizes.some((x) => !x)) return null
  return { kb: (CORE + sizes.reduce((a, f) => a + f - CORE, 0)) / 1024, exact: false }
}

// -------------------------------------------------------------- events 🖱️
function wireUp(sheet) {
  sheet.querySelectorAll('[data-go]').forEach((b) => b.addEventListener('click', () => {
    go(b.dataset.go)
  }))
  sheet.querySelectorAll('[data-toggle]').forEach((b) => b.addEventListener('click', () => toggleEffect(b.dataset.toggle)))
  sheet.querySelectorAll('[data-check]').forEach((c) => c.addEventListener('change', () => { state.checks[c.dataset.check] = c.checked; changed() }))
  sheet.querySelectorAll('select.pin-pick').forEach((s) => s.addEventListener('change', () => {
    const v = Number(s.value)
    if (s.id.startsWith('trig-')) state.effects.find((e) => s.id === `trig-${e.id}`).start.pin = v
    else {
      const e = state.effects.find((x) => s.id.startsWith(`pin-${x.id}-`))
      const [cls, name] = s.id.slice(`pin-${e.id}-`.length).split('-')
      e.pins[cls][name] = v
    }
    changed()
  }))
  sheet.querySelectorAll('[data-mode]').forEach((r) => r.addEventListener('change', () => {
    const e = state.effects.find((x) => x.id === r.dataset.mode)
    if (r.value.startsWith('src:')) e.source = r.value.slice(4)
    else {
      const was = e.source === 'trigger' ? e.start.mode : 'POWER_UP'
      e.source = 'trigger'
      e.start.mode = r.value
      if (was === 'POWER_UP' && r.value !== 'POWER_UP') e.start.pin = freeTriggerPin(state.effects, e)
      if (r.value === 'TOGGLE') e.start.activeLow = true
    }
    changed()
  }))
  sheet.querySelectorAll('[data-pol]').forEach((s) => s.addEventListener('change', () => { state.effects.find((e) => e.id === s.dataset.pol).start.activeLow = s.value === 'low'; changed() }))
  sheet.querySelectorAll('[data-runon]').forEach((i) => i.addEventListener('change', () => {
    const v = Math.max(0, Math.min(86400, Number(i.value) || 0))
    state.effects.find((e) => e.id === i.dataset.runon).start.runOnMs = Math.round(v * 1000)
    changed()
  }))
  sheet.querySelectorAll('[data-cfg]').forEach((c) => c.addEventListener('change', () => {
    const [id, cls, name] = c.dataset.cfg.split('|')
    const e = state.effects.find((x) => x.id === id)
    const f = schema.classes[cls].fields.find((x) => x.name === name)
    const kind = c.dataset.kind
    if (kind === 'bool') e.cfg[cls][name] = c.checked
    else if (kind === 'enum') e.cfg[cls][name] = c.value
    else {
      const [min, max] = RANGES[f.type] || [0, 255]
      const n = Number(c.value)
      if (c.value === '' || Number.isNaN(n)) { c.value = e.cfg[cls][name]; return }
      e.cfg[cls][name] = Math.min(max, Math.max(min, kind === 'float' ? n : Math.round(n)))
    }
    changed()
  }))
  sheet.querySelectorAll('details[data-open-key]').forEach((d) => d.addEventListener('toggle', () => {
    state.openSettings = { ...(state.openSettings || {}), [d.dataset.openKey]: d.open }
    save()
  }))
  sheet.querySelectorAll('[data-boot]').forEach((s) => s.addEventListener('change', () => { state.bootloader = s.value; changed() }))
  $('#install', sheet)?.addEventListener('click', install)
  $('#copy', sheet)?.addEventListener('click', async (ev) => {
    const b = ev.currentTarget
    try { await navigator.clipboard.writeText(buildCommands(state, state.bootloader, { name: folderName() })); b.textContent = 'Copied'; announce('Commands copied.') } catch { b.textContent = 'Select the text and copy it' }
    setTimeout(() => { b.textContent = 'Copy commands' }, 2000)
  })
  $('#download', sheet)?.addEventListener('click', () => {
    const a = document.createElement('a')
    a.href = URL.createObjectURL(new Blob([generateSketch(state, { title: state.name })], { type: 'text/plain' }))
    a.download = `${folderName()}.ino`
    a.click()
    setTimeout(() => URL.revokeObjectURL(a.href), 1000)
  })
  // ✨ the signature: pointing at a wiring step lights its parts and pins
  sheet.querySelectorAll('ol.wire li').forEach((li) => {
    const on = () => highlight(new Set((li.dataset.parts || '').split(' ').filter(Boolean)), new Set((li.dataset.pins || '').split(' ').filter(Boolean).map((x) => (/^\d+$/.test(x) ? Number(x) : x))), li)
    li.addEventListener('mouseenter', on)
    li.addEventListener('focusin', on)
    li.addEventListener('mouseleave', () => highlight(null))
    li.addEventListener('focusout', (ev) => { if (!li.contains(ev.relatedTarget)) highlight(null) })
  })
}

function highlight(parts, pins, li) {
  document.querySelectorAll('ol.wire li.is-lit').forEach((x) => x.classList.remove('is-lit'))
  lit = parts ? { parts, pins } : null
  if (li) li.classList.add('is-lit')
  document.querySelectorAll('#nano .part').forEach((p) => p.classList.toggle('is-lit', !!lit && lit.parts.has(p.dataset.key)))
  document.querySelectorAll('#nano .pinhole[data-pin]').forEach((c) => {
    const pin = /^\d+$/.test(c.dataset.pin) ? Number(c.dataset.pin) : c.dataset.pin
    c.classList.toggle('is-lit', !!lit && lit.pins.has(pin))
  })
}

function toggleEffect(id) {
  if (state.effects.some((e) => e.id === id)) state.effects = state.effects.filter((e) => e.id !== id)
  else {
    const excludes = byId[id].excludes || []
    state.effects = state.effects.filter((e) => !excludes.includes(e.id))
    state.effects.push(assignPins(defaultEntry(id), state.effects))
  }
  state.installed = false
  changed()
  announce(`${byId[id].title} ${state.effects.some((e) => e.id === id) ? 'added to' : 'removed from'} your board.`)
}

// ---------------------------------------------------------------- start 🚀
const nameInput = $('#board-name')
nameInput.value = state.name
nameInput.addEventListener('input', () => { state.name = nameInput.value.trim() || 'My Nano'; save(); if (state.step === 'install' || state.step === 'wire') render() })
fetch('firmware/manifest.json', { cache: 'no-cache' }).then((r) => (r.ok ? r.json() : null)).then((m) => { manifest = m; render() }).catch(() => {})
render()
