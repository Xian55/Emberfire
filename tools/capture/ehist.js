const fs=require('fs');
const f=process.argv[2];
const lines=fs.readFileSync(f,'utf8').trim().split('\n');
const seen={}, firsthex={}, names={};
for(const ln of lines){const o=JSON.parse(ln); if(!o.frame) continue;
  const k=o.dir+' '+o.op; seen[k]=(seen[k]||0)+1; names[k]=o.name||''; if(!firsthex[k]) firsthex[k]=o.hex;}
const keys=Object.keys(seen).sort((a,b)=>{const[da,oa]=a.split(' ');const[db,ob]=b.split(' ');return da<db?-1:da>db?1:(+oa)-(+ob);});
for(const k of keys){const[d,op]=k.split(' ');console.log(`${d} op${op} (${names[k]}) x${seen[k]} len=${(firsthex[k].length/2)|0}`);}
