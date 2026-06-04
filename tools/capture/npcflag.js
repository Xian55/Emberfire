const fs=require('fs');const {Database}=require('bun:sqlite');
const db=new Database((process.env.EMBERFIRE_GAME_DIR || '.') + '/game.db',{readonly:true});
const lines=fs.readFileSync(process.argv[2],'utf8').trim().split('\n');
const guidEntry={}, guid06={};
for(const ln of lines){const o=JSON.parse(ln);if(!o.frame)continue;const b=Buffer.from(o.hex,'hex');
  if(o.op===78){ // NPC spawn: guid@0,x@4,y@8,varCount@12,vars(5B),orient@?,entry@?
    const g=b.readInt32LE(0); const vc=b.readUInt32LE(12); let off=16;
    for(let k=0;k<vc&&off+5<=b.length;k++){const id=b[off],val=b.readInt32LE(off+1); if(id===0x06) guid06[g]=val; off+=5;}
    const entry=off+4<=b.length?b.readUInt32LE(off+4):null; guidEntry[g]=entry;
  }
  if(o.op===82&&b[4]===0x06){guid06[b.readInt32LE(0)]=b.readInt32LE(5);}
}
console.log('guid | wire0x06 | entry | DB npc_flags | DB game_flags | name');
for(const g of Object.keys(guid06)){const e=guidEntry[g];let row=null;
  if(e!=null){try{row=db.prepare('SELECT name,npc_flags,game_flags FROM npc_template WHERE entry=?').get(e);}catch{}}
  const m=guid06[g];
  console.log(`${g} | 0x${(m>>>0).toString(16)} (${m}) | ${e} | ${row?'0x'+(row.npc_flags>>>0).toString(16)+'('+row.npc_flags+')':'?'} | ${row?row.game_flags:'?'} | ${row?row.name:'?'}`);
}
