const { Database } = require('bun:sqlite');
const db = new Database((process.env.EMBERFIRE_GAME_DIR || '.') + '/game.db', { readonly: true });

// distinct values of a column with count + one example name
function enumCol(table, col, namecol='name', where='') {
  const w = where ? ` WHERE ${where}` : '';
  let rows;
  try {
    rows = db.prepare(
      `SELECT ${col} v, COUNT(*) c, MIN(${namecol}) ex FROM ${table}${w} GROUP BY ${col} ORDER BY ${col}`
    ).all();
  } catch(e){ console.log(`  ${table}.${col}: ERR ${e.message}`); return; }
  console.log(`\n## ${table}.${col}  (${rows.length} distinct)`);
  for (const r of rows) console.log(`  ${String(r.v).padStart(6)} | n=${String(r.c).padStart(5)} | e.g. ${r.ex}`);
}

// union of multiple columns (e.g. effect1/2/3) -> distinct values
function enumUnion(table, cols, label) {
  const sel = cols.map(c=>`SELECT ${c} v, name FROM ${table}`).join(' UNION ALL ');
  const rows = db.prepare(`SELECT v, COUNT(*) c, MIN(name) ex FROM (${sel}) GROUP BY v ORDER BY v`).all();
  console.log(`\n## ${table}.[${label}]  (${rows.length} distinct)`);
  for (const r of rows) console.log(`  ${String(r.v).padStart(6)} | n=${String(r.c).padStart(5)} | e.g. ${r.ex}`);
}

console.log('===== ITEM =====');
['quality','armor_type','weapon_type','equip_type','weapon_material','required_class','num_sockets'].forEach(c=>enumCol('item_template',c));
// flags bitmask: list distinct raw
enumCol('item_template','flags');

console.log('\n===== SPELL =====');
enumUnion('spell_template',['effect1','effect2','effect3'],'effects');
enumUnion('spell_template',['effect1_targetType','effect2_targetType','effect3_targetType'],'targetTypes');
['dispel','magic_roll_school','cast_school','prevention_type','attributes','aura_interrupt_flags','cast_interrupt_flags','req_caster_mechanic','req_tar_mechanic','abilities_tab'].forEach(c=>enumCol('spell_template',c));
// what carries aura type? effectN_data1 when effect is an apply-aura effect
enumUnion('spell_template',['effect1_data1','effect2_data1','effect3_data1'],'effectN_data1');

console.log('\n===== NPC =====');
['type','npc_flags','game_flags','movement_type','ai_type','faction'].forEach(c=>enumCol('npc_template',c));

console.log('\n===== GAMEOBJECT =====');
['type','flags'].forEach(c=>enumCol('gameobject_template',c));

console.log('\n===== QUEST =====');
enumCol('quest_template','flags');
