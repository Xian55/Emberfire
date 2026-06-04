const {Database}=require('bun:sqlite');
const db=new Database((process.env.EMBERFIRE_GAME_DIR || '.') + '/game.db',{readonly:true});
const r=db.prepare("SELECT Class, MIN(Level) lo, MAX(Level) hi, COUNT(*) n FROM player_class_stats GROUP BY Class ORDER BY Class").all();
console.log('player_class_stats classes:'); for(const x of r) console.log(`  Class ${x.Class}: levels ${x.lo}-${x.hi} (${x.n} rows)`);
// class spells (which class learns Aimed Shot etc)
try{const s=db.prepare("SELECT name FROM spell_template WHERE name LIKE 'Learn:%' AND (name LIKE '%Aimed%' OR name LIKE '%Multi-Shot%' OR name LIKE '%Arrow%')").all();
console.log('ranger-ish learn spells:', s.map(x=>x.name).join(', '));}catch(e){console.log(e.message);}
