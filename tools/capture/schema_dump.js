const { Database } = require('bun:sqlite');
const db = new Database((process.env.EMBERFIRE_GAME_DIR || '.') + '/game.db', { readonly: true });
const tbls = db.prepare("SELECT name FROM sqlite_master WHERE type='table' ORDER BY name").all().map(t=>t.name);
console.log('TABLES:', tbls.join(', '));
const want = ['item_template','spell_template','npc_template','gameobject_template','quest_template','player_class_stats','affix_template'];
for (const t of want) {
  if (!tbls.includes(t)) { console.log(`\n## ${t}: MISSING`); continue; }
  const cols = db.prepare(`PRAGMA table_info(${t})`).all();
  console.log(`\n## ${t}  (${db.prepare(`SELECT COUNT(*) c FROM ${t}`).get().c} rows)`);
  console.log('cols:', cols.map(c=>`${c.name}:${c.type}`).join(', '));
}
