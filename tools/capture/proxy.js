// Emberfire capture proxy — sits between the game client and the real game server,
// logging every byte both directions AND decoding the framed packets into human-readable form.
//
// Flow:  client  ->  127.0.0.1:LISTEN_PORT  (this proxy)  ->  SERVER_HOST:SERVER_PORT (real server)
//
// Wire framing:  [u32 LE bodyLen][u16 LE opcode][payload(bodyLen-2)]   (bodyLen counts opcode+payload)
//
// Usage:
//   1. Real game server running on 16383.
//   2. bun proxy.js           (override ports: PROXY_PORT=16599 SERVER_PORT=17000 bun proxy.js)
//   3. Client config [Tcp] Port=16500  (%AppData%\Roaming\Emberfire\config.ini).
//   4. Play. Ctrl+C to stop.
// Output files (./), one set per connection, NAMED WITH THE CLIENT BUILD (from Authenticate op2):
//   session-<ts>-conn<N>-v<build>.decoded.log  <- READ THIS: one line per packet, opcode named + fields
//   session-<ts>-conn<N>-v<build>.log          <- raw hex chunks
//   session-<ts>-conn<N>-v<build>.ndjson       <- raw structured (per chunk) + one record per decoded frame
//   <build> = client build number: 1190 = our local rebuild, 1189 = steam/original, vUNKNOWN = never authed.
//
// ───────────────────────────────────────────────────────────────────────────────────────────────
// STRUCTURE: every opcode is ONE entry in the C2S / S2C registry below:
//     <op>: { name, decode?, quiet?, probe? }
//   name   — display label (single source of truth; the framer prints it, decoders never repeat it)
//   decode — (r) => string of decoded fields, using the readers from reader(payload). Omit if unknown
//            (an opcode with no decode auto-falls-through to the raw PROBE byte dump).
//   quiet  — collapse high-frequency frames (heartbeats/streams) to a one-char tick instead of a line.
//   probe  — also emit the raw multi-interpretation byte dump (for layouts whose record order is unverified).
// Layouts are mined from build/recon/GHIDRA_PROTOCOL.md (server serialize funcs) + OPCODES.md (capture).
// ───────────────────────────────────────────────────────────────────────────────────────────────

const net = require('net');
const fs = require('fs');
const path = require('path');

const LISTEN_PORT = Number(process.env.PROXY_PORT) || 16500;   // override: PROXY_PORT=16599 bun proxy.js
const SERVER_HOST = process.env.SERVER_HOST || '127.0.0.1';
const SERVER_PORT = Number(process.env.SERVER_PORT) || 16383;

// ── reader: per-payload typed accessors. All bounds-checked (return null past the end). ──────────
function reader(p) {
    const u8 = i => i < p.length ? p.readUInt8(i) : null;
    const u16 = i => i + 2 <= p.length ? p.readUInt16LE(i) : null;
    const i16 = i => i + 2 <= p.length ? p.readInt16LE(i) : null;
    const u32 = i => i + 4 <= p.length ? p.readUInt32LE(i) : null;
    const i32 = i => i + 4 <= p.length ? p.readInt32LE(i) : null;
    const f32 = i => i + 4 <= p.length ? p.readFloatLE(i).toFixed(2) : null;
    const guid = i => { const v = u32(i); return v == null ? null : (v | 0); };          // signed (npc guids negative)
    const strEnd = i => { let e = i; while (e < p.length && p[e] !== 0) e++; return e; }; // index of the NUL
    const cstr = i => i >= p.length ? '""' : JSON.stringify(p.toString('latin1', i, strEnd(i)));
    const raw = i => i >= p.length ? '' : p.toString('latin1', i, strEnd(i));             // unquoted string body
    const hex = (a, b) => p.subarray(a, b).toString('hex');
    // base-object: guid:u32, x:f32, y:f32, varCount:u32, varCount×{id:u8, val:i32}, [tail @o]
    const baseObj = () => { const vc = u32(12); let o = 16, vars = []; for (let k = 0; k < (vc || 0) && o + 5 <= p.length; k++) { vars.push(`0x${p[o].toString(16)}=${p.readInt32LE(o + 1)}`); o += 5; } return { vc, o, vars }; };
    return { p, len: p.length, u8, u16, i16, u32, i32, f32, guid, cstr, raw, strEnd, hex, baseObj };
}

// ── shared mini-decoders (reused by many opcodes) ───────────────────────────────────────────────
const D = {
    name: r => `name=${r.cstr(0)}`,
    accept: r => `accept=${r.u8(0)}`,
    arg0: r => `arg0=${r.u32(0)}`,
    questId: r => `questId=${r.u32(0)}`,
    guid0: r => `guid=${r.guid(0)}`,
    len: r => `len=${r.len}`,
    empty: () => `(empty)`,
};

// ── named decoders for the multi-field / loop layouts ───────────────────────────────────────────
function decAuthenticate(r) { const e = r.strEnd(0); return `token=${r.cstr(0)} build=${e + 5 <= r.len ? r.u32(e + 1) : null} fp=${r.cstr(e + 5)}`; }
function decEquipItem(r) { return `slotInv=${r.u8(0)} slotEq=${r.u8(1)} item=${r.u16(2)} affix=${r.u16(4)} score=${r.u8(6)} gems=${[r.u8(7), r.u8(8), r.u8(9), r.u8(10)].join(',')} enchant=${r.u8(11)} soulbound=${r.u8(12)} dur=${r.u8(13)}`; }
function decLootItem(r) { return `srcGuid=${r.guid(0)} item=${r.u16(4)} affix=${r.u16(6)} score=${r.u8(8)} b=${r.u8(9)}` + (r.u16(4) === 0xffff ? '  (TAKE ALL)' : ''); }

function decPlayer(r) { const b = r.baseObj(); return `guid=${r.guid(0)} x=${r.f32(4)} y=${r.f32(8)} varCnt=${b.vc} vars[${b.vars.length}]=${b.vars.slice(0, 6).join(',')}… tail@${b.o}=${r.hex(b.o, b.o + 16)}`; }
function decNpc(r) { const b = r.baseObj(); return `guid=${r.guid(0)} x=${r.f32(4)} y=${r.f32(8)} varCnt=${b.vc} orient=${r.f32(b.o)} entry=${r.u32(b.o + 4)}`; }
function decGameObject(r) { const b = r.baseObj(); return `guid=${r.guid(0)} x=${r.f32(4)} y=${r.f32(8)} varCnt=${b.vc} entry=${r.u32(b.o)} (NO orient)`; }

// CONFIRMED: count:u32, [name:cstr, class:u8, level:u8, guid:u32, portrait:u8, gender:u8]
function decCharList(r) { const n = r.u32(0); let o = 4, cs = []; for (let k = 0; k < (n || 0) && o < r.len; k++) { const e = r.strEnd(o); const nm = r.p.toString('latin1', o, e); o = e + 1; cs.push(`${nm}(cls${r.u8(o)} lv${r.u8(o + 1)} g${r.u32(o + 2)})`); o += 8; } return `count=${n} [${cs.join(' ')}]`; }
// CONFIRMED 13B item record (Inventory op85 / Bank op141 share it)
function decInventory(r) { const n = r.u8(0); let o = 1, items = []; for (let k = 0; k < (n || 0) && o + 13 <= r.len; k++) { items.push(`s${k}:e${r.u16(o)}x${r.u8(o + 12)}`); o += 13; } return `count=${n} [${items.filter(s => !/e0x/.test(s)).slice(0, 8).join(' ')}]`; }
function decBank(r) { const n = r.u8(0); let o = 1, items = []; for (let k = 0; k < (n || 0) && o + 13 <= r.len; k++) { items.push(`s${r.u16(o)}:e${r.u16(o + 2)}`); o += 13; } return `count=${n} [${items.filter(s => !/:e0$/.test(s)).slice(0, 8).join(' ')}]`; }
// CONFIRMED 7B loot items
function decLoot(r) { const n = r.u32(8); let o = 12, it = []; for (let k = 0; k < (n || 0) && o + 7 <= r.len; k++) { it.push(`e${r.u16(o)}/af${r.u16(o + 2)}/sc${r.u8(o + 4)}/du${r.u8(o + 5)}/ct${r.u8(o + 6)}`); o += 7; } return `objGuid=${r.guid(0)} money=${r.u32(4)} items=${n} [${it.join(' ')}]`; }
// CONFIRMED (GHIDRA FUN_00467520, 15B record): u32 unitGuid, u8 buffCount, buffs[{u16 spellId, u32 t0, u32 t1, u8 stacks, u32 caster}], u8 debuffCount, debuffs[…]. t0/t1 = elapsed/max duration (client derives start/end).
function decAuras(r) {
    let o = 4; const rec = (tag) => { const s = `${tag}{sp=${r.u16(o)} maxDur=${r.u32(o + 2)} elapsed=${r.u32(o + 6)} stk=${r.u8(o + 10)} caster=${r.guid(o + 11)}}`; o += 15; return s; };
    const bc = r.u8(o) || 0; o += 1; const buffs = []; for (let k = 0; k < bc && o + 15 <= r.len; k++) buffs.push(rec('b'));
    const dc = r.u8(o); o += 1; const debuffs = []; for (let k = 0; k < (dc || 0) && o + 15 <= r.len; k++) debuffs.push(rec('d'));
    return `unit=${r.guid(0)} buffs=${bc} debuffs=${dc == null ? '?' : dc} [${buffs.concat(debuffs).join(' ')}]`;
}
// CONFIRMED (GHIDRA FUN_0046dd70, VARIABLE-length entries — NOT stride 0x10): u8 count, [u16 spellId, u8 rank, u8 fxCount, {u16 min, u16 max}×fxCount].
function decSpellbook(r) { const n = r.u8(0); let o = 1, es = []; for (let k = 0; k < (n || 0) && o + 4 <= r.len; k++) { const sp = r.u16(o), rank = r.u8(o + 2), fc = r.u8(o + 3); o += 4; const fx = []; for (let j = 0; j < (fc || 0) && o + 4 <= r.len; j++) { fx.push(`${r.u16(o)}:${r.u16(o + 2)}`); o += 4; } es.push(`sp${sp}/r${rank}${fx.length ? '[' + fx.join(',') + ']' : ''}`); } return `count=${n} [${es.join(' ')}]`; }
// CONFIRMED (GHIDRA FUN_00431380): npcGuid:u32, textId:u32, 3× {count:u32, count×u32}, vendorCount:u32
function decGossip(r) { let o = 8, parts = []; for (const lbl of ['questAvail', 'questActive', 'gossipOpt']) { const c = r.u32(o); o += 4; const ids = []; for (let k = 0; k < (c || 0) && o + 4 <= r.len; k++) { ids.push(r.u32(o)); o += 4; } parts.push(`${lbl}=${c}[${ids.join(',')}]`); } return `npc=${r.guid(0)} textId=${r.u32(4)} ${parts.join(' ')} vendorItems=${r.u32(o)}`; }
// CONFIRMED (GHIDRA FUN_0046d620): count:u8, per quest { questId:u32, state:u8, 4×{objCount:u8, objCount×u32} }
function decQuestList(r) { const n = r.u8(0); let o = 1, qs = []; for (let k = 0; k < (n || 0) && o + 5 <= r.len; k++) { const qid = r.u32(o), st = r.u8(o + 4); o += 5; for (let j = 0; j < 4; j++) { o += 1 + (r.u8(o) || 0) * 4; } qs.push(`q${qid}/s${st}`); } return `count=${n} [${qs.join(' ')}]`; }
// CONFIRMED: senderGuid:u32, channel:u8, text:cstr, senderName:cstr
function decChatMsg(r) { const e = r.strEnd(5); return `sender=${r.guid(0)} channel=${r.u8(4)} text=${r.cstr(5)} name=${r.cstr(e + 1)}`; }
// CONFIRMED: leaderGuid:u32, count:u8, members×u32
function decPartyList(r) { const n = r.u8(4); const m = []; for (let k = 0, o = 5; k < (n || 0) && o + 4 <= r.len; k++, o += 4) m.push(r.guid(o)); return `leader=${r.guid(0)} count=${n} members=[${m.join(',')}]`; }
function decWaypointResp(r) { const n = r.u16(0); const ids = []; for (let i = 2; i + 4 <= r.len; i += 4) ids.push(r.u32(i)); return `count=${n} ids=[${ids}]`; }

// ── CLIENT → SERVER registry (1..74). Wrong value = server DROPS the connection. ─────────────────
const C2S = {
    1:  { name: 'Ping', quiet: true },
    2:  { name: 'Authenticate', decode: decAuthenticate },         // build distinguishes local(1190)/steam(1189)
    4:  { name: 'CharCreate' },
    5:  { name: 'EnterWorld', decode: r => `enterGuid=${r.u32(0)}` },
    6:  { name: 'ClickedGossipOption' },
    7:  { name: 'AcceptQuest', decode: r => `questId=${r.u32(0)} giverGuid=${r.guid(4)}` }, // CONFIRMED vs steam (conn1 18-58): THIS accepts quests -> AcceptedQuest(112). giver may be 0.
    8:  { name: 'AcceptQuest(alt/unused)', decode: D.questId },   // GHIDRA FUN_0048a030 reads 1 int; gossip flow never uses it (server ignored our op8 sends).
    9:  { name: 'EquipItem', decode: decEquipItem },               // CONFIRMED (GHIDRA FUN_00484d80): u8,u8,u16,u16,u8×6,u8,u8 = 14B
    10: { name: 'DestroyItem', decode: r => `slot=${r.u8(0)} item=${r.u16(1)} affix=${r.u16(3)} … (server: "item was destroyed")` },
    11: { name: 'UseItem', decode: r => `slot=${r.u8(0)} item=${r.u16(1)} tgtInv=${r.u8(3)} tgtEq=${r.u8(4)}` },
    12: { name: 'SellItem', decode: r => `slot=${r.u8(0)} item=${r.u16(1)}  (CONFIRMED money-verified)` },
    13: { name: 'MoveItem', decode: r => `from=${r.u8(0)} to=${r.u8(1)}` },
    14: { name: 'UnequipItem', decode: r => `eqSlot=${r.u8(0)} invDst=${r.u8(1)}` },
    15: { name: 'Move', quiet: true, decode: r => `x=${r.f32(0)} y=${r.f32(4)} wasd=${r.u8(8)}` },
    16: { name: 'Stop' },
    17: { name: 'Chat', decode: r => `channel=${r.u8(0)} text=${r.cstr(1)}` },
    18: { name: 'GuildMotd', decode: r => `motd=${r.cstr(0)}` },
    19: { name: 'AbandonQuest', decode: D.questId },          // CONFIRMED (GHIDRA FUN_0048a140): one int = questId.
    21: { name: 'YieldDuel' },
    22: { name: 'GuildInvMember', decode: D.name },
    23: { name: 'GuildCreate', decode: D.name },
    24: { name: 'GuildInvResponse' },
    25: { name: 'GuildPromote' },
    26: { name: 'GuildDemote' },
    27: { name: 'CastSpell', decode: r => `spellId=${r.u32(0)} target=${r.guid(4)} x=${r.f32(8)} y=${r.f32(12)} ctm=${r.u8(16)}` },
    28: { name: 'GuildRosterReq' },
    29: { name: 'SetSelected', decode: r => `selectGuid=${r.guid(0)}` },
    30: { name: 'LootItem', decode: decLootItem },
    31: { name: 'SetIgnore', decode: r => `ignore=${r.u8(0)} name=${r.cstr(1)}` },
    32: { name: 'ReqTheoreticalSpell' },
    // CONFIRMED (GHIDRA FUN_0048a310 -> FUN_00432e70): reads 3 ints; #1=questId (match key), #2=ignored, #3=itemChoiceIdx. No giver guid on wire.
    33: { name: 'CompleteQuest', decode: r => `questId=${r.u32(0)} _ign=${r.u32(4)} choice=${r.i32(8)}` },
    34: { name: 'RecoverMailLoot' },
    35: { name: 'BuyVendorItem', decode: r => `item=${r.u16(0)} affix=${r.u16(2)} score=${r.u8(4)} pad=${r.u8(5)} count=${r.u32(6)}` }, // CONFIRMED (GHIDRA FUN_00484ca0): u16,u16,u8,u8,u32
    36: { name: 'Buyback', decode: D.empty },
    37: { name: 'Repair', decode: r => `confirmed=${r.u8(0)}` },
    38: { name: 'RequestRespawn' },
    39: { name: 'DuelResponse', decode: D.accept },
    40: { name: 'LevelUp' },                                       // 64B, no confirmed layout → auto-probe
    41: { name: 'SpellRankInvest', decode: r => `spellId=${r.u16(0)} rank=${r.u16(2)}` }, // CONFIRMED (GHIDRA FUN_004929a0): u16 spellId (validated), u16 rank∈1..6
    42: { name: 'ReqAbilityList?' },
    44: { name: 'SortInventory' },
    45: { name: 'QueryWaypoints', decode: D.arg0 },
    46: { name: 'ActivateWaypoint', decode: r => `wpA=${r.u32(0)} wpB=${r.u32(4)}` },
    47: { name: 'PartyInvMember', decode: D.name },
    48: { name: 'PartyInvResponse', decode: D.accept },
    49: { name: 'PartyChanges', decode: r => `leave=${r.u8(0)} kick=${r.guid(1)} promote=${r.guid(5)}` },
    50: { name: 'GuildQuit' },
    51: { name: 'GuildDisband' },
    52: { name: 'OpenTrade', decode: D.name },
    53: { name: 'TradeAddItem', decode: r => `itemGuid=${r.u32(0)}` },
    54: { name: 'TradeRemoveItem', decode: r => `slot=${r.u32(0)}` },   // CONFIRMED
    55: { name: 'TradeCancel' },
    56: { name: 'TradeConfirm' },
    57: { name: 'TradeSetGold', decode: r => `gold=${r.u32(0)}` },      // CONFIRMED
    58: { name: 'ResetDungeons' },
    59: { name: 'UnBankItem' },
    60: { name: 'SortBank' },
    61: { name: 'MoveBankToBank' },
    62: { name: 'MoveInvToBank' },
    63: { name: 'ChangeChannels' },
    64: { name: 'Respec' },
    65: { name: 'DeleteCharacter' },
    66: { name: 'SplitItemStack' },
    67: { name: 'UpdateArena', decode: r => `enter=${r.u8(0)} leave=${r.u8(1)}` },
    68: { name: 'RollDice' },
    69: { name: 'TogglePvP' },
    70: { name: 'MOD_Mute', decode: D.name },
    71: { name: 'MOD_Kick', decode: D.name },
    72: { name: 'MOD_Ban', decode: D.name },
    73: { name: 'MOD_Warn', decode: D.name },
    74: { name: 'Report', decode: D.name },
};

// ── SERVER → CLIENT registry (3, 75..151). ──────────────────────────────────────────────────────
const S2C = {
    3:   { name: 'Validate', decode: r => `result=${r.u32(0)} serverTime=${r.u32(4)}` },
    75:  { name: 'CharList', decode: decCharList },
    76:  { name: 'CreateResult', decode: r => `result=${r.u8(0)}` },
    77:  { name: 'Player/NewWorld', decode: decPlayer },
    78:  { name: 'Npc', decode: decNpc },
    79:  { name: 'GameObject', decode: decGameObject },
    80:  { name: 'SetController', decode: r => `controlGuid=${r.guid(0)}` },
    82:  { name: 'ObjectVariable', quiet: true, decode: r => `guid=${r.guid(0)} var=0x${(r.u8(4) || 0).toString(16)} val=${r.i32(5)}` },
    83:  { name: 'Inspect', decode: r => `guid=${r.guid(0)} … len=${r.len}` },
    84:  { name: 'NotifyAdd?', decode: r => `guid=${r.guid(0)}  (guid-only notify)` },
    85:  { name: 'Inventory', decode: decInventory },
    86:  { name: 'EquipItem', probe: true, decode: r => `guid=${r.guid(0)} slot=${r.u8(4)} item=${r.u16(5)} affix=${r.u16(7)} … len=${r.len}` },
    87:  { name: 'WorldError', decode: r => `code=${r.u16(0)}` },
    88:  { name: 'OfferDuel', decode: r => `challenger=${r.cstr(0)}` },
    89:  { name: 'ChatError', decode: r => `code=${r.u16(0)}` },
    90:  { name: 'UnitSpline', quiet: true, decode: r => `guid=${r.guid(0)} silent=${r.u8(4)} start=${r.f32(5)},${r.f32(9)} slide=${r.u8(13)} cnt=${r.u16(14)}` },
    91:  { name: 'ChatMsg', decode: decChatMsg },
    94:  { name: 'GuildAdd', decode: r => `guid=${r.guid(0)} name=${r.cstr(4)}` },
    95:  { name: 'GuildRoster', decode: r => `len=${r.len} (roster list)` },
    96:  { name: 'SetSubname', decode: r => `guid=${r.guid(0)} name=${r.cstr(4)}` },
    97:  { name: 'GuildInvite', decode: r => `inviter=${r.cstr(0)}` },
    98:  { name: 'CastStart', decode: r => `guid=${r.guid(0)} spell=${r.u16(4)} timer=${r.u32(6)}` },
    99:  { name: 'CastStop', decode: r => `guid=${r.guid(0)} spell=${r.u16(4)}` },
    // CONFIRMED (GHIDRA FUN_0046e0a0, validated vs capture): u32 caster, u32 spellId, u16 hitCount, {u32 targetGuid, u32 value}×hitCount, u32, u32.
    100: { name: 'SpellGo', decode: r => { const n = r.u16(8); let o = 10, h = []; for (let k = 0; k < (n || 0) && o + 8 <= r.len; k++) { h.push(`${r.guid(o)}:v${(r.u32(o + 4) >>> 0)}`); o += 8; } return `caster=${r.guid(0)} spell=${r.u32(4)} hits=${n} [${h.join(' ')}] groundX=${r.f32(o)} groundY=${r.f32(o + 4)}`; } },
    101: { name: 'UnitTeleport', decode: r => `guid=${r.guid(0)} x=${r.f32(4)} y=${r.f32(8)} ori=${r.f32(12)}` },
    102: { name: 'CombatMsg', decode: r => `caster=${r.guid(0)} target=${r.guid(4)} hit=${r.u8(8)} amt=${r.i16(9)} spell=${r.u16(11)} eff=${r.u8(13)} periodic=${r.u8(14)} aura=${r.u8(15)} pos=${r.u8(16)}` }, // CONFIRMED 17B
    103: { name: 'NotifyItemAdd', decode: r => `affix=${r.u16(0)} item=${r.u16(2)} attrs=${r.hex(4, 11)} amount=${r.u32(11)} name=${r.cstr(15)}` }, // CONFIRMED (affix@0, item@2)
    104: { name: 'OpenLootWindow', decode: decLoot },
    105: { name: 'UnitOrientation', quiet: true, decode: r => `guid=${r.guid(0)} orient=${r.f32(4)}` },
    107: { name: 'Cooldown', decode: r => `spell=${r.u32(0)} total=${r.u32(4)}ms elapsed=${r.u32(8)}` },
    108: { name: 'UnitAuras', decode: decAuras },
    109: { name: 'Spellbook', decode: decSpellbook },
    110: { name: 'GossipMenu', decode: decGossip },
    111: { name: 'QuestList', decode: decQuestList },
    112: { name: 'AcceptedQuest', decode: D.questId },
    113: { name: 'QuestTally' },                                   // 13B, no confirmed layout → auto-probe
    114: { name: 'QuestComplete', decode: r => `questId=${r.u32(0)} done=${r.u8(4)}` },
    115: { name: 'RewardedQuest', decode: D.questId },
    117: { name: 'AbandonQuest', decode: D.questId },
    119: { name: 'ExpNotify', decode: r => `gained=${r.i32(0)} flag=${r.u8(4)}` }, // 5B observed
    120: { name: 'AvailWorldQuests', decode: r => `count=${r.u8(0)} ids=u16… len=${r.len}` },
    121: { name: 'RespawnResponse', decode: r => `${r.len ? 'ok=' + r.u8(0) : '(empty)'}` },
    122: { name: 'SpellbookUpdate', decode: r => `spell=${r.u16(0)} level=${r.u8(2)} subCnt=${r.u8(3)}` },
    124: { name: 'LvlResponse', decode: r => `bool=${r.u8(0)}` },
    125: { name: 'AggroMob', decode: D.guid0 },
    126: { name: 'LearnedSpell', decode: r => `spell=${r.i32(0)}` }, // CONFIRMED (stub GP_Server_LearnedSpell): i32 spellId
    127: { name: 'UpdateVendorStock' },                           // 7B, no confirmed layout → auto-probe
    128: { name: 'RepairCost', decode: r => `amount=${r.i32(0)} finished=${r.u8(4)}` }, // CONFIRMED
    130: { name: 'WaypointResp', decode: decWaypointResp },
    131: { name: 'DiscoverWaypoint', decode: r => `guid=${r.guid(0)}  (only leading guid confirmed)` },
    132: { name: 'OfferParty', decode: r => `inviter=${r.cstr(0)}` },
    133: { name: 'PartyList', decode: decPartyList },
    134: { name: 'OnObjectWasLooted', decode: r => `objGuid=${r.guid(0)} looter=${r.guid(4)}` }, // CONFIRMED
    135: { name: 'MarkNpcsOnMap' },
    137: { name: 'TradeUpdate', decode: r => `partner=${r.guid(0)} len=${r.len}` },
    138: { name: 'TradeCanceled', decode: D.empty },
    // CONFIRMED (client Game.cpp:1726 / stub): cstr playerName, i32 role
    139: { name: 'GuildRoleChange', decode: r => { const e = r.strEnd(0); return `name=${r.cstr(0)} role=${r.i32(e + 1)}`; } },
    // CONFIRMED (client Game.cpp:1665 / stub): cstr playerName, u8 online(bool)
    140: { name: 'GuildOnline', decode: r => { const e = r.strEnd(0); return `name=${r.cstr(0)} online=${r.u8(e + 1)}`; } },
    141: { name: 'Bank', decode: decBank },
    142: { name: 'OpenBank', decode: r => `(len=${r.len})` },
    143: { name: 'ChannelInfo', quiet: true },                    // server heartbeat, spammed
    144: { name: 'PromptRespec', decode: D.len },
    145: { name: 'ChannelChangeConfirm', decode: D.len },
    146: { name: 'ArenaReady', decode: D.len },
    147: { name: 'SocketResult', decode: r => `ok=${r.u8(0)}` },
    148: { name: 'ArenaOutcome', decode: D.len },
    149: { name: 'ArenaStatus', decode: D.len },
    150: { name: 'PkNotify', decode: D.name },
    151: { name: 'QueuePosition', decode: r => `pos=${r.u32(0)}` },
};

const quietLabel = t => Object.keys(t).filter(k => t[k].quiet).map(k => `${t[k].name}(${k})`).join(', ');

// ── probe: multi-interpretation byte dump (spot which width/offset yields sane values) ───────────
function probe(p) {
    if (!p.length) return '      (empty payload)';
    const L = [];
    L.push('      hex : ' + [...p].map(b => b.toString(16).padStart(2, '0')).join(' '));
    L.push('      u8  : ' + [...p].join(' '));
    const u16 = []; for (let i = 0; i + 2 <= p.length; i += 2) u16.push(`@${i}=${p.readUInt16LE(i)}`);
    L.push('      u16 : ' + u16.join('  '));
    const w = []; for (let i = 0; i + 4 <= p.length; i += 4) {
        const u = p.readUInt32LE(i), s = p.readInt32LE(i), f = p.readFloatLE(i);
        let t = '';
        if (s < 0 && s > -200000) t += ' guid?';
        if (Math.abs(f) >= 0.01 && Math.abs(f) < 100000) t += ` f=${f.toFixed(2)}`;
        w.push(`@${i}=${u}${u !== s ? '/' + s : ''}${t}`);
    }
    L.push('      u32 : ' + w.join('  '));
    const strs = []; let i = 0;
    while (i < p.length) { let e = i; while (e < p.length && p[e] >= 32 && p[e] < 127) e++; if (e - i >= 2) strs.push(`@${i}="${p.toString('latin1', i, e)}"`); i = e + 1; }
    if (strs.length) L.push('      str : ' + strs.join('  '));
    return L.join('\n');
}

const stamp = new Date().toISOString().replace(/[:.]/g, '-');
let connId = 0;
const t0 = Date.now();
const openConns = new Map(); // id -> conn

// Files are opened LAZILY: the client build (1190 local / 1189 steam) is only known once the
// Authenticate(op2) packet is decoded, and we want it in the filename. Until then, writes are
// buffered in memory (the connect banner + the handful of pre-auth frames), then flushed when
// open(label) fires — either from setBuild(...) or the 3s fallback / connection close.
function makeConn(id) {
    let streams = null, opened = false;
    const pending = [];                                    // [target, data] queued until files open
    const q = (target, data) => { if (opened) streams[target].write(data); else pending.push([target, data]); };
    const conn = {
        id, build: null, label: null,
        decLine: s => { q('decOut', s + '\n'); console.log(s); },  // decoded line -> file + console
        hexWrite: s => q('hexOut', s),
        jsonWrite: s => q('jsonOut', s),
        open(label) {
            if (opened) return;
            conn.label = label;
            const base = `session-${stamp}-conn${id}-${label}`;
            streams = {
                hexOut: fs.createWriteStream(path.join(__dirname, `${base}.log`)),
                jsonOut: fs.createWriteStream(path.join(__dirname, `${base}.ndjson`)),
                decOut: fs.createWriteStream(path.join(__dirname, `${base}.decoded.log`)),
            };
            opened = true;
            for (const [t, d] of pending) streams[t].write(d);
            pending.length = 0;
            console.log(`   conn#${id} files -> ${base}.{decoded.log,ndjson,log}`);
        },
        setBuild(build) {
            if (conn.build != null) return;
            conn.build = build;
            const tag = build === 1190 ? ' (local rebuild)' : build === 1189 ? ' (steam/original)' : '';
            conn.decLine(`conn#${id} client build = ${build}${tag}`);
            conn.open(build == null ? 'vUNKNOWN' : `v${build}`);
        },
        end() {
            if (!opened) conn.open('vUNKNOWN');            // flush buffered banner even if never authed
            streams.hexOut.end(); streams.jsonOut.end(); streams.decOut.end();
        },
    };
    openConns.set(id, conn);
    return conn;
}

// Per (conn,dir) frame reassembler.
function makeFramer(id, dir, conn) {
    let buf = Buffer.alloc(0), quietRun = 0;
    const table = dir === 'C2S' ? C2S : S2C;
    const quietList = Object.keys(table).filter(k => table[k].quiet).join('/');
    return chunk => {
        buf = Buffer.concat([buf, chunk]);
        let i = 0;
        while (i + 4 <= buf.length) {
            const blen = buf.readUInt32LE(i);
            if (blen < 2 || blen > 1 << 20) { // resync guard
                conn.decLine(`[+${Date.now() - t0}ms] conn#${id} ${dir} !! bad frame len=${blen} at ${i}, resyncing`);
                buf = buf.subarray(i + 1); i = 0; continue;
            }
            if (i + 4 + blen > buf.length) break; // wait for more
            const op = buf.readUInt16LE(i + 4);
            const payload = buf.subarray(i + 6, i + 4 + blen);
            i += 4 + blen;
            const entry = table[op];
            const name = entry ? entry.name : `op${op}`;
            // The first Authenticate(op2) carries the client build → tag this connection + name its files.
            if (dir === 'C2S' && op === 2 && conn.build == null) {
                let e = 0; while (e < payload.length && payload[e] !== 0) e++;   // skip token cstr
                conn.setBuild(e + 5 <= payload.length ? payload.readUInt32LE(e + 1) : null);
            }
            conn.jsonWrite(JSON.stringify({ t: Date.now() - t0, conn: id, build: conn.build, dir, frame: true, op, name, len: blen, hex: payload.toString('hex') }) + '\n');
            if (entry && entry.quiet) { quietRun++; process.stdout.write('.'); continue; }
            if (quietRun) { conn.decLine(`   (… ${quietRun} quiet ${dir} frames: ${quietList})`); quietRun = 0; }
            let fields = null;
            if (entry && entry.decode) { try { fields = entry.decode(reader(payload)); } catch (e) { fields = `<decode error: ${e.message}>`; } }
            const dump = payload.length <= 48 ? payload.toString('hex').replace(/(..)/g, '$1 ').trim() : payload.subarray(0, 48).toString('hex') + '…';
            conn.decLine(`[+${Date.now() - t0}ms] conn#${id} ${dir} ${name}(${op}) len=${blen}` + (fields ? `  { ${fields} }` : '') + `  | ${dump}`);
            // deep byte-breakdown for opcodes flagged probe, or any with no decoder
            if ((entry && entry.probe) || !fields) conn.decLine(probe(payload));
        }
        buf = buf.subarray(i);
    };
}

const server = net.createServer(client => {
    const id = ++connId;
    const conn = makeConn(id);
    conn.decLine(`\n=== conn#${id} client connected (awaiting build from Authenticate…) ===`);
    // Client connects but never authenticates within 3s -> open files anyway with build unknown.
    const authTimer = setTimeout(() => conn.open('vUNKNOWN'), 3000);
    const upstream = net.connect(SERVER_PORT, SERVER_HOST, () => {});
    const c2sFramer = makeFramer(id, 'C2S', conn);
    const s2cFramer = makeFramer(id, 'S2C', conn);

    function raw(dir, d) {
        conn.hexWrite(`[+${Date.now() - t0}ms] conn#${id} ${dir} ${d.length} bytes\n`);
        conn.jsonWrite(JSON.stringify({ t: Date.now() - t0, conn: id, build: conn.build, dir, len: d.length, hex: d.toString('hex') }) + '\n');
    }
    client.on('data', d => { raw('C2S', d); c2sFramer(d); upstream.write(d); });
    upstream.on('data', d => { raw('S2C', d); s2cFramer(d); client.write(d); });

    let closed = false;
    const close = who => () => {
        clearTimeout(authTimer);
        if (!closed) { closed = true; conn.decLine(`conn#${id} ${who} closed`); }
        client.destroy(); upstream.destroy();
        conn.end();
        openConns.delete(id);
    };
    client.on('close', close('client'));
    upstream.on('close', close('upstream'));
    client.on('error', e => conn.decLine(`conn#${id} client err: ${e.message}`));
    upstream.on('error', e => conn.decLine(`conn#${id} upstream err: ${e.message}`));
});

server.listen(LISTEN_PORT, '127.0.0.1', () => {
    console.log(`Decoding capture proxy on 127.0.0.1:${LISTEN_PORT} -> ${SERVER_HOST}:${SERVER_PORT}`);
    console.log(`Per-connection output files: session-${stamp}-conn<N>-v<build>.{decoded.log,ndjson,log}  (build: 1190=local, 1189=steam)`);
    console.log(`Quiet (collapsed): C2S ${quietLabel(C2S)} | S2C ${quietLabel(S2C)}  — set quiet:false on the opcode to show.`);
    console.log(`Set client [Tcp] Port=${LISTEN_PORT} and play. Multiple clients OK (each -> own conn file). Ctrl+C to stop.`);
});

process.on('SIGINT', () => {
    for (const c of openConns.values()) c.end();
    console.log('\nCapture saved. Bye.');
    process.exit(0);
});
