const fs=require('fs');
const lines=fs.readFileSync(process.argv[2],'utf8').trim().split('\n');
let c=0;
for(const ln of lines){const o=JSON.parse(ln); if(!o.frame||o.op!==90) continue;
  const b=Buffer.from(o.hex,'hex');
  const guid=b.readUInt32LE(0), b4=b[4], sx=b.readFloatLE(5), sy=b.readFloatLE(9), b13=b[13], cnt=b.readUInt16LE(14);
  console.log(`guid=${guid} byte4=${b4} sx=${sx.toFixed(1)} sy=${sy.toFixed(1)} byte13=${b13} cnt=${cnt} | ${o.hex}`);
  if(++c>=8) break;
}
