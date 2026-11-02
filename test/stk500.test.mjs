// ⚡ The flasher against a fake optiboot: same STK500v1 commands, its own flash.
import { test } from 'node:test'
import assert from 'node:assert/strict'
import { Stk500, ATMEGA328P } from '../web/js/stk500.js'

class FakeBootloader {
  constructor({ signature = ATMEGA328P.signature, syncAfterResets = 0, corruptAt = -1, inBootloader = true } = {}) {
    this.flash = new Uint8Array(ATMEGA328P.flashSize).fill(0xff)
    this.signature = signature
    this.out = []
    this.inBootloader = inBootloader
    this.resets = 0
    this.syncAfterResets = syncAfterResets
    this.corruptAt = corruptAt
    this.addr = 0
    this.commands = []
  }
  async setSignals({ dtr }) { if (!dtr) this.resets++ }
  async flush() { this.out.length = 0 }
  async read(count) { return Uint8Array.from(this.out.splice(0, count)) }
  async write(bytes) {
    const b = [...bytes]
    if (!this.inBootloader || this.resets <= this.syncAfterResets - 1) return   // asleep: no answer
    if (b.at(-1) !== 0x20) return
    const ok = (...data) => this.out.push(0x14, ...data, 0x10)
    this.commands.push(b[0])
    switch (b[0]) {
      case 0x30: case 0x50: case 0x51: ok(); break
      case 0x75: ok(...this.signature); break
      case 0x55: this.addr = (b[1] | (b[2] << 8)) * 2; ok(); break
      case 0x64: {
        const len = (b[1] << 8) | b[2]
        this.flash.set(b.slice(4, 4 + len), this.addr)
        if (this.corruptAt >= this.addr && this.corruptAt < this.addr + len) this.flash[this.corruptAt] ^= 0xff
        ok(); break
      }
      case 0x74: { const len = (b[1] << 8) | b[2]; ok(...this.flash.slice(this.addr, this.addr + len)); break }
      default: break
    }
  }
}

const image = Uint8Array.from({ length: 1000 }, (_, i) => (i * 7) & 0xff)

test('writes and verifies a firmware image', async () => {
  const fake = new FakeBootloader()
  const progress = []
  await new Stk500(fake).flash(image, { onProgress: (p) => progress.push(p) })
  assert.deepEqual(fake.flash.slice(0, image.length), image)
  assert.equal(fake.flash[image.length], 0xff, 'the rest of the last page is padded with 0xFF')
  assert.equal(progress.at(-1), 1)
  assert.equal(fake.commands.at(-1), 0x51, 'leaves programming mode at the end')
  assert.equal(fake.resets, 1)
})

test('refuses a chip that is not an ATmega328P', async () => {
  const fake = new FakeBootloader({ signature: [0x1e, 0x98, 0x01] })   // a Mega
  await assert.rejects(new Stk500(fake).flash(image), /isn't an ATmega328P/)
  assert.ok(!fake.commands.includes(0x64), 'nothing was written')
})

test('reports a verify failure', async () => {
  const fake = new FakeBootloader({ corruptAt: 300 })
  await assert.rejects(new Stk500(fake).flash(image), /verify failed at 0x12c/)
})

test('gives up with advice when the Nano never answers', async () => {
  const fake = new FakeBootloader({ inBootloader: false })
  await assert.rejects(new Stk500(fake).flash(image), /didn't answer.*old vs\. new/)
})

test('refuses an image that would overwrite the bootloader', async () => {
  const fake = new FakeBootloader()
  await assert.rejects(new Stk500(fake).flash(new Uint8Array(31 * 1024)), /a Nano holds 30720/)
})
