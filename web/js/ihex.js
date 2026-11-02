// 📄 Intel HEX: what arduino-cli produces and the bootloader is fed.
// parseHex() turns it into one flat flash image; findBlock() locates the
// SETTINGS block by its marker so the installer can overwrite it.

export function parseHex(text) {
  const chunks = []
  let base = 0, end = 0
  for (const [n, raw] of text.split(/\r?\n/).entries()) {
    const line = raw.trim()
    if (!line) continue
    if (line[0] !== ':') throw new Error(`hex line ${n + 1}: missing ':'`)
    const bytes = line.slice(1).match(/../g).map((h) => parseInt(h, 16))
    const [len, hi, lo, type] = bytes
    const sum = bytes.reduce((a, b) => a + b, 0) & 0xff
    if (sum !== 0 || bytes.length !== len + 5) throw new Error(`hex line ${n + 1}: bad checksum or length`)
    const data = bytes.slice(4, 4 + len)
    if (type === 0x00) {
      const addr = base + ((hi << 8) | lo)
      chunks.push([addr, data])
      end = Math.max(end, addr + len)
    } else if (type === 0x01) break
    else if (type === 0x02) base = ((data[0] << 8) | data[1]) << 4
    else if (type === 0x04) base = ((data[0] << 8) | data[1]) << 16
    // 0x03 / 0x05 (start address) don't matter for AVR
  }
  const image = new Uint8Array(end).fill(0xff)
  for (const [addr, data] of chunks) image.set(data, addr)
  return image
}

// offset of the SETTINGS block ("MRACFG" + version), or -1
export function findBlock(image, magic = 'MRACFG', version = 1) {
  const m = [...magic].map((c) => c.charCodeAt(0)).concat(version)
  outer: for (let i = 0; i <= image.length - m.length; i++) {
    for (let j = 0; j < m.length; j++) if (image[i + j] !== m[j]) continue outer
    return i
  }
  return -1
}
