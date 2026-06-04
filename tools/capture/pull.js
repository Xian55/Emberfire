const fs=require('fs');
const ops=new Set(process.argv.slice(3).map(Number));
const files=process.argv[2].split(',');
const got={};
for(const f of files){const lines=fs.readFileSync(f,'utf8').trim().split('\n');
  for(const ln of lines){const o=JSON.parse(ln); if(!o.frame||!ops.has(o.op))continue;
    const k=o.dir+o.op; (got[k]=got[k]||[]).push(o.hex);}}
function interp(hex){const p=Buffer.from(hex,'hex');const o=[];
  o.push('u8:['+[...p].join(',')+']');
  let s=[],i=0;while(i<p.length){let e=i;while(e<p.length&&p[e]>=32&&p[e]<127)e++;if(e-i>=2)s.push(`@${i}"${p.toString('latin1',i,e)}"`);i=e+1;}
  if(s.length)o.push('str:'+s.join(' '));
  let w=[];for(let i=0;i+4<=p.length;i+=4){const u=p.readUInt32LE(i),si=p.readInt32LE(i),fl=p.readFloatLE(i);let t=`@${i}=${u}`;if(si<0&&si>-200000)t+='/g';if(Math.abs(fl)>=.5&&Math.abs(fl)<1e5)t+=` f${fl.toFixed(1)}`;w.push(t);}
  o.push('u32:'+w.join(' '));return o.join('  ');}
for(const k of Object.keys(got)){console.log(`\n## ${k}  (${got[k].length})`);
  got[k].slice(0,3).forEach(h=>console.log('  '+h+'\n     '+interp(h)));}
