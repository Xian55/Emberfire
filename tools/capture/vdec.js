const fs=require('fs');
const lines=fs.readFileSync(process.argv[2],'utf8').trim().split('\n');
const vid={}; // varId -> {n, vals:Set}
for(const ln of lines){const o=JSON.parse(ln); if(!o.frame||o.op!==82) continue;
  const b=Buffer.from(o.hex,'hex'); if(b.length<9) continue;
  const v=b[4]; const val=b.readInt32LE(5);
  if(!vid[v]) vid[v]={n:0,vals:[]}; vid[v].n++; if(vid[v].vals.length<6 && !vid[v].vals.includes(val)) vid[v].vals.push(val);
}
const ks=Object.keys(vid).map(Number).sort((a,b)=>a-b);
for(const k of ks){console.log(`0x${k.toString(16).padStart(2,'0')} (${k})  n=${String(vid[k].n).padStart(4)}  vals=[${vid[k].vals.join(', ')}]`);}
