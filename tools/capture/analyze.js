// Decode a capture: reassemble per-direction byte streams, split into frames
//   frame = [u32 LE bodyLen][u16 LE opcode][payload(bodyLen-2)]
// Output: opcode histogram per direction + first-occurrence hexdump.
const fs = require('fs');
const path = require('path');

const file = process.argv[2] || fs.readdirSync(__dirname).filter(f => f.endsWith('.ndjson')).sort().pop();
const lines = fs.readFileSync(path.join(__dirname, file), 'utf8').trim().split('\n');

const streams = { C2S: [], S2C: [] };
for (const ln of lines) {
    const o = JSON.parse(ln);
    streams[o.dir].push(Buffer.from(o.hex, 'hex'));
}

function dump(buf, max = 64) {
    const b = buf.subarray(0, max);
    let s = '';
    for (let i = 0; i < b.length; i += 16) {
        const sl = b.subarray(i, i + 16);
        const hex = [...sl].map(x => x.toString(16).padStart(2, '0')).join(' ');
        const asc = [...sl].map(x => (x >= 32 && x < 127) ? String.fromCharCode(x) : '.').join('');
        s += `      ${hex.padEnd(47)}  ${asc}\n`;
    }
    if (buf.length > max) s += `      ... (${buf.length} total)\n`;
    return s;
}

for (const dir of ['C2S', 'S2C']) {
    const all = Buffer.concat(streams[dir]);
    let off = 0, n = 0;
    const hist = new Map(); const first = new Map();
    while (off + 4 <= all.length) {
        const bodyLen = all.readUInt32LE(off); off += 4;
        if (bodyLen < 2 || off + bodyLen > all.length) { console.log(`${dir}: desync at off ${off} bodyLen ${bodyLen}`); break; }
        const op = all.readUInt16LE(off);
        const payload = all.subarray(off + 2, off + bodyLen);
        off += bodyLen; n++;
        hist.set(op, (hist.get(op) || 0) + 1);
        if (!first.has(op)) first.set(op, payload);
    }
    console.log(`\n======== ${dir}: ${n} frames, ${hist.size} distinct opcodes ========`);
    const ops = [...hist.keys()].sort((a, b) => a - b);
    for (const op of ops) {
        const p = first.get(op);
        console.log(`  opcode ${op} (0x${op.toString(16)})  x${hist.get(op)}  firstPayloadLen=${p.length}`);
        console.log(dump(p, 48).replace(/\n$/, ''));
    }
}
