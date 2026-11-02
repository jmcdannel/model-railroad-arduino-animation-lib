// 🔌 Web Serial transport for stk500.js (Chrome and Edge on desktop).

export const supported = () => typeof navigator !== 'undefined' && 'serial' in navigator

export class WebSerialTransport {
  constructor(port) {
    this.port = port
    this.buffer = []
    this.waiters = []
  }

  async open(baudRate) {
    await this.port.open({ baudRate, bufferSize: 1024 })
    this.reader = this.port.readable.getReader()
    this.writer = this.port.writable.getWriter()
    this.pump()
  }

  // keep reading in the background into a buffer that read() drains
  async pump() {
    try {
      for (;;) {
        const { value, done } = await this.reader.read()
        if (done) break
        this.buffer.push(...value)
        this.waiters.splice(0).forEach((w) => w())
      }
    } catch { /* port closed */ }
  }

  async write(bytes) { await this.writer.write(bytes) }

  async read(count, timeoutMs) {
    const deadline = Date.now() + timeoutMs
    while (this.buffer.length < count) {
      const left = deadline - Date.now()
      if (left <= 0) break
      await new Promise((r) => { const t = setTimeout(r, left); this.waiters.push(() => { clearTimeout(t); r() }) })
    }
    return Uint8Array.from(this.buffer.splice(0, count))
  }

  async flush() { this.buffer.length = 0 }

  async setSignals({ dtr, rts }) { await this.port.setSignals({ dataTerminalReady: dtr, requestToSend: rts }) }

  async close() {
    try { await this.reader?.cancel() } catch {}
    try { this.reader?.releaseLock() } catch {}
    try { this.writer?.releaseLock() } catch {}
    try { await this.port.close() } catch {}
  }
}
