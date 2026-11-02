// ⚡ Flashes an Arduino Nano through its bootloader (STK500v1, as spoken by
// optiboot and the older ATmegaBOOT). Works over any transport with:
//   write(Uint8Array) · read(count, timeoutMs) → Uint8Array · setSignals({dtr, rts}) · flush()
// webserial.js provides one for the browser; the tests use a fake bootloader.

const STK = {
  OK: 0x10, INSYNC: 0x14, CRC_EOP: 0x20,
  GET_SYNC: 0x30, ENTER_PROGMODE: 0x50, LEAVE_PROGMODE: 0x51, LOAD_ADDRESS: 0x55,
  PROG_PAGE: 0x64, READ_PAGE: 0x74, READ_SIGN: 0x75,
}
export const ATMEGA328P = { signature: [0x1e, 0x95, 0x0f], pageSize: 128, flashSize: 32768, bootloaderSize: 2048 }
export const BAUD = { new: 115200, old: 57600 }   // "new" optiboot vs. "old bootloader" Nanos

const sleep = (ms) => new Promise((r) => setTimeout(r, ms))

export class Stk500 {
  constructor(transport, { log = () => {} } = {}) {
    this.t = transport
    this.log = log
  }

  async command(bytes, replyLength = 0, timeoutMs = 500) {
    await this.t.write(Uint8Array.from([...bytes, STK.CRC_EOP]))
    const reply = await this.t.read(replyLength + 2, timeoutMs)
    if (reply.length < replyLength + 2 || reply[0] !== STK.INSYNC || reply[reply.length - 1] !== STK.OK) {
      throw new Error(`no answer from the bootloader (command 0x${bytes[0].toString(16)})`)
    }
    return reply.slice(1, 1 + replyLength)
  }

  // pulse DTR/RTS to reset the Nano into its bootloader, then sync
  async reset() {
    await this.t.setSignals({ dtr: false, rts: false })
    await sleep(120)
    await this.t.setSignals({ dtr: true, rts: true })
    await sleep(60)
    await this.t.flush()
  }

  async sync(attempts = 10) {
    for (let i = 0; i < attempts; i++) {
      try {
        await this.command([STK.GET_SYNC], 0, 200)
        return
      } catch {
        await this.t.flush()
      }
    }
    throw new Error('the Nano didn\'t answer. Check the port, and try the other bootloader setting (old vs. new)')
  }

  async signature() { return [...(await this.command([STK.READ_SIGN], 3))] }

  async loadAddress(byteAddr) {
    const word = byteAddr >> 1
    await this.command([STK.LOAD_ADDRESS, word & 0xff, word >> 8])
  }

  // image: flash bytes starting at 0. onProgress(fraction, phase)
  async flash(image, { chip = ATMEGA328P, verify = true, onProgress = () => {} } = {}) {
    const max = chip.flashSize - chip.bootloaderSize
    if (image.length > max) throw new Error(`firmware is ${image.length} bytes; a Nano holds ${max}`)
    const pages = Math.ceil(image.length / chip.pageSize)
    const page = (i) => {
      const p = new Uint8Array(chip.pageSize).fill(0xff)
      p.set(image.subarray(i * chip.pageSize, (i + 1) * chip.pageSize))
      return p
    }

    this.log('🔌 resetting the Nano')
    await this.reset()
    await this.sync()
    const sig = await this.signature()
    if (sig.join() !== chip.signature.join()) {
      throw new Error(`this isn't an ATmega328P Nano (signature ${sig.map((b) => b.toString(16)).join(' ')})`)
    }
    await this.command([STK.ENTER_PROGMODE])
    this.log(`✍️  writing ${pages} pages`)
    for (let i = 0; i < pages; i++) {
      const data = page(i)
      await this.loadAddress(i * chip.pageSize)
      await this.command([STK.PROG_PAGE, 0, chip.pageSize, 0x46, ...data], 0, 1000)
      onProgress((i + 1) / pages / (verify ? 2 : 1), 'write')
    }
    if (verify) {
      this.log('🔍 verifying')
      for (let i = 0; i < pages; i++) {
        await this.loadAddress(i * chip.pageSize)
        const got = await this.command([STK.READ_PAGE, 0, chip.pageSize, 0x46], chip.pageSize, 1000)
        const want = page(i)
        const bad = want.findIndex((b, j) => b !== got[j])
        if (bad >= 0) throw new Error(`verify failed at 0x${(i * chip.pageSize + bad).toString(16)}`)
        onProgress(0.5 + (i + 1) / pages / 2, 'verify')
      }
    }
    await this.command([STK.LEAVE_PROGMODE])
    this.log('✅ done: the Nano restarts with the new firmware')
  }
}
