const { Database } = require('bun:sqlite');
const db = new Database((process.env.EMBERFIRE_GAME_DIR || '.') + '/game.db', { readonly: true });
// which effect is "apply aura"? crosstab effect1 -> effect1_data1 distinct, with a few names
console.log('== effect1 x data1 (aura-type candidates) ==');
const r = db.prepare(`SELECT effect1 ef, effect1_data1 d1, COUNT(*) c, MIN(name) ex FROM spell_template WHERE effect1 IS NOT NULL GROUP BY effect1, effect1_data1 ORDER BY effect1, effect1_data1`).all();
let cur=null;
for (const x of r){ if(x.ef!==cur){console.log(`\n effect ${x.ef}:`);cur=x.ef;} console.log(`    data1=${String(x.d1).padStart(4)} n=${String(x.c).padStart(3)} ${x.ex}`); }
