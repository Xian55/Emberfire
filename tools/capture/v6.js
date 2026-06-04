const fs=require('fs');const lines=fs.readFileSync(process.argv[2],'utf8').trim().split('\n');
for(const ln of lines){const o=JSON.parse(ln);if(o.frame&&o.op===82){const b=Buffer.from(o.hex,'hex');if(b[4]===0x06)console.log(`guid=${b.readInt32LE(0)} val=${b.readInt32LE(5)} (0x${b.readUInt32LE(5).toString(16)})`);}}
