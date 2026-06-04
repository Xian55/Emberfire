const fs=require('fs');
const files=process.argv.slice(2);
const b8={},b13={},b14={},b16={}; let n=0;
for(const f of files){for(const ln of fs.readFileSync(f,'utf8').trim().split('\n')){const o=JSON.parse(ln);if(!o.frame||o.op!==102)continue;const p=Buffer.from(o.hex,'hex');if(p.length<17)continue;
  const amt=p.readInt16LE(9);
  b8[p[8]]=(b8[p[8]]||0)+1; b13[p[13]]=(b13[p[13]]||0)+1; b14[p[14]]=(b14[p[14]]||0)+1; b16[p[16]]=(b16[p[16]]||0)+1;
  if(n<14) console.log(`b8=${p[8]} amt@9=${amt} u16@11=${p.readUInt16LE(11)} b13=${p[13]} b14=${p[14]} b15=${p[15]} b16=${p[16]} | ${o.hex}`);
  n++;}}
const h=(o)=>Object.entries(o).map(([k,v])=>`${k}:${v}`).join(' ');
console.log(`\nn=${n}\nbyte8 dist: ${h(b8)}\nbyte13: ${h(b13)}\nbyte14: ${h(b14)}\nbyte16: ${h(b16)}`);
