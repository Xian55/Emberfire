const fs=require('fs');const h={};
for(const ln of fs.readFileSync(process.argv[2],'utf8').trim().split('\n')){const o=JSON.parse(ln);if(!o.frame)continue;
  if(o.op===87){const p=Buffer.from(o.hex,'hex');const c=p.length>=2?p.readInt16LE(0):p[0];h['err'+c]=(h['err'+c]||0)+1;}
  if(o.op===89){const p=Buffer.from(o.hex,'hex');h['chatErr'+(p.length>=2?p.readInt16LE(0):p[0])]=(h['chatErr']||0)+1;}}
console.log(JSON.stringify(h,null,0));
