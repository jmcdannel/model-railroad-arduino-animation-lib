// 📌 Pin planning: auto-assign free pins to new effects, and check a plan for
// clashes. Mirrors the checks in examples/multi-effect.
import schema from './schema.js'
import { byId, PINS, PWM_PINS, PIN_CHOICES, NO_PIN, pinName } from './effects.js'
import { ordered } from './generate.js'

const SERVO_TIMER_PINS = [9, 10]
const usesServo = (entries) => entries.some((e) => byId[e.id].usesServoTimer)
const fadeOk = (pin, servo) => PWM_PINS.includes(pin) && !(servo && SERVO_TIMER_PINS.includes(pin))

// every pin a configuration uses: { pin, effect, role, fades, kind }
export function pinUses(entries) {
  const uses = []
  for (const e of ordered(entries)) {
    const fx = byId[e.id]
    for (const o of fx.objects) {
      for (const p of schema.classes[o.cls].pins) {
        const pin = e.pins[o.cls][p.name]
        if (pin === NO_PIN) continue
        const [label, fades] = PINS[o.cls][p.name]
        uses.push({ pin, effect: e.id, role: label, fades, kind: 'output' })
      }
    }
    if (e.start.mode !== 'POWER_UP' && e.source === 'trigger') uses.push({ pin: e.start.pin, effect: e.id, role: 'Trigger', fades: false, kind: 'trigger' })
  }
  return uses
}

export function checkPlan(entries) {
  const problems = []
  const servo = usesServo(entries)
  const seen = new Map()
  for (const u of pinUses(entries)) {
    const name = `${byId[u.effect].title}: ${u.role}`
    if (seen.has(u.pin)) problems.push({ pin: u.pin, text: `${pinName(u.pin)} is used twice (${seen.get(u.pin)} and ${name}).` })
    else seen.set(u.pin, name)
    if (u.fades && !PWM_PINS.includes(u.pin)) problems.push({ pin: u.pin, text: `${name} fades, so it needs a PWM pin (D3, D5, D6, D9, D10, D11), not ${pinName(u.pin)}.` })
    else if (u.fades && servo && SERVO_TIMER_PINS.includes(u.pin)) problems.push({ pin: u.pin, text: `${name} can't fade on ${pinName(u.pin)}: the gate servos take over D9 and D10's PWM timer.` })
  }
  return problems
}

// give a new entry pins that don't clash with what's already on the board
export function assignPins(entry, others) {
  const all = [...others, entry]
  const servo = usesServo(all)
  const taken = new Set(pinUses(others).map((u) => u.pin))
  const fx = byId[entry.id]
  const take = (want, fades) => {
    const good = (p) => !taken.has(p) && (!fades || fadeOk(p, servo))
    if (want !== NO_PIN && good(want)) { taken.add(want); return want }
    // fades: PWM pins only; on/off: prefer pins that can't fade, so PWM stays free
    const pool = fades ? PIN_CHOICES.filter((p) => fadeOk(p, servo)) : [...PIN_CHOICES.filter((p) => !PWM_PINS.includes(p)), ...PWM_PINS]
    const p = pool.find(good)
    if (p === undefined) return want
    taken.add(p)
    return p
  }
  for (const o of fx.objects) {
    for (const p of schema.classes[o.cls].pins) {
      const cur = entry.pins[o.cls][p.name]
      if (cur === NO_PIN) continue
      entry.pins[o.cls][p.name] = take(cur, PINS[o.cls][p.name][1])
    }
  }
  if (entry.start.mode !== 'POWER_UP') entry.start.pin = take(entry.start.pin, false)
  return entry
}

// a trigger pin for an effect switching from POWER_UP: A0–A5 first, then anything free
export function freeTriggerPin(entries, entry) {
  const taken = new Set(pinUses(entries.filter((e) => e !== entry)).map((u) => u.pin))
  for (const o of byId[entry.id].objects) for (const p of Object.values(entry.pins[o.cls])) taken.add(p)
  return [14, 15, 16, 17, 18, 19, 12, 13, 2, 4, 7, 8].find((p) => !taken.has(p)) ?? 12
}
