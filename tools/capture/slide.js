const fs=require('fs');
const F=process.argv[2];
const lines=fs.readFileSync(F,'utf8').trim().split('\n');
let self=null;
// op80 SetController = self guid
for(const ln of lines){const o=JSON.parse(ln); if(o.frame&&o.op===80){self=Buffer.from(o.hex,'hex').readInt32LE(0);break;}}
console.log('SELF (controller) guid =',self);
// scan op90 flag bytes; report distinct (flagA,flagB) combos + whether guid==self
const combo={};
for(const ln of lines){const o=JSON.parse(ln); if(!o.frame||o.op!==90)continue;
  const b=Buffer.from(o.hex,'hex'); const g=b.readInt32LE(0),fa=b[4],fb=b[13];
  const isSelf=g===self;
  const k=`fa=${fa} fb=${fb} self=${isSelf}`; combo[k]=(combo[k]||0)+1;
  if((fa||fb)){ if(!combo['_ex'+k]){combo['_ex'+k]=1; console.log(`NONZERO op90 guid=${g}${isSelf?'(SELF)':''} flagA@4=${fa} flagB@13=${fb} | ${o.hex}`);}}
}
console.log('\nop90 flag combos:'); for(const k of Object.keys(combo)) if(!k.startsWith('_ex')) console.log(`  ${k}  x${combo[k]}`);
// chat
console.log('\n--- C2S Chat(17) ---');
for(const ln of lines){const o=JSON.parse(ln); if(o.frame&&o.op===17&&o.dir==='C2S'){const b=Buffer.from(o.hex,'hex');console.log(`  channel=${b[0]} | ${o.hex}`);}}
console.log('--- S2C ChatMsg(91) ---');
for(const ln of lines){const o=JSON.parse(ln); if(o.frame&&o.op===91){const b=Buffer.from(o.hex,'hex');console.log(`  senderGuid=${b.readInt32LE(0)} channel=${b[4]} | ${o.hex.slice(0,80)}`);}}
