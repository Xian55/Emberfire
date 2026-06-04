# Dreadmyst Server backend — architecture spec (reverse-engineered from Dreadmyst Server.exe)
Source: Ghidra RTTI class list + decompiled handlers + SQL string dump. Binary stripped of function names
but RETAINS RTTI class names + plaintext SQL — so the class taxonomy below is ground truth.

## Process model
TcpListener accepts → per-connection **Session** (the dispatch `this` at +0xa0 = login state: 0=new,1=authed,2=in-world;
+0x138 = ServerPlayer; +0x80 = sf::TcpSocket; +0x90 = accountId; +0x98 = playerGuid). DB via SqlConnector
(MySQL for accounts/players, SQLite game.db for static content) with QueuedQuery/QueuedCallback (async query queue).
World runs per-map threads (Runtime::MapThreadInfo).

## Networking
- `GamePacket` base; `GP_Client_*` (parsed in dispatch FUN_00489310, opcode=case+1, 1..74) and `GP_Server_*` (built+sent).
- `SfSocket`(framing [u32 len][u16 opcode][body]) over `TcpSocketEx : sf::TcpSocket`.
- Build = write opcode literal then fields (e.g. GP_Server_Validate::build FUN_00484150 writes op 3, result, time).
- StlBuffer: `>>` peeks, eraseFront consumes. prims: readString FUN_00420330, readInt32 FUN_00411990, readU16 FUN_004118b0, readU8 FUN_00411920/FUN_00401e50.

## Game object hierarchy (RTTI)
`MutualObject`(shared w/ client) → `ServerObject` → `ServerUnit` → { `ServerPlayer`, `ServerNpc` }.
`ServerGameObj` → Container / MapChanger / Waypoint / NullType. `ServerMap`(:GameMap), `ServerTerrainCell`(:MapCellT),
`ServerParty`, `Guild`/`GuildPlayer`, `Arena`. `MutualUnit` shared base for unit stats/health/mana/spline.

## Movement system (= the client's UnitSpline source; our stubbed pathing/spline)
`UnitMotionGen` drives motion via swappable generators:
`IdleMovement, RandomMovement, PatrolMovement, PointMovement, ChaseMovement, FearMovement, HomeMovement,
ChargeMovement, SlideMovement, ActionMovement`. Server computes the path → sends `GP_Server_UnitSpline`
(guid + points); client just interpolates. Client RequestMove (op15 {x:float,y:float,wasd:u8}) sets dest
(FUN_004a6500) then server motion-gen produces the spline.

## Spell system
`Effect` base + ~40 `Effect_*` (WeaponDamage, SchoolDamage, Heal, HealPct, HealthDrain, ManaBurn/Drain, ApplyAura,
ApplyAreaAura, ApplyGemSocket, ApplyOrbEnchant, Charge, CombineItem, CreateItem, DestroyGems, Dispel, Duel,
ExtractOrb, Gossip, Inspect, InterruptCast, Kill, KnockBack, LearnSpell, Loot, MeleeAtk, RangedAtk, NearestWayp,
PullTo, RestoreMana/Pct, Resurrect, ScriptEffect, SlideFrom, SummonNpc, SummonObject, Teleport/Forward, Threat,
TriggerSpell). `Aura`/`AuraEffect` + `Aura_*` (PeriodicDamage/Heal/RestoreMana, ModifyStat/Pct, AbsorbDamage,
InflictMechanic, MechanicImmune, Model, ModifyDamage/HealingDealt/Recv Pct, ModifyMoveSpeedPct, Proc, SchoolImmunity).
SpellTemplate loaded from spell_template. Effects map to SpellDefines::Effects, Auras to AuraType (our enums).

## NPC AI
`NpcAi` base + `NpcAi_Melee`, `NpcAi_CasterArcher`. NpcTemplate has spell_1..4 (id/chance/interval/cooldown/targetType).

## Scripting
`ScriptCmd` base + ScriptCmd_{CastSpell, KillCredit, LocateNpc, OpenBank, PromptRespec, QueueArena, Talk}. ScriptDb loaded from `scripts` table.

## DB content loaders (each loads a table → map of *Template/*Entry; from SQL strings)
ItemTemplate(item_template), ItemAffix(affix_template), ItemGemEntry(item_gems), SpellTemplate(spell_template),
NpcTemplate(npc_template), NpcDb(npc), NpcAiEntry(npc_ai), NpcGroupEntry(npc_groups), NpcModels, NpcVendorEntry(npc_vendor),
NpcVendorRandom(npc_vendor_random), NpcWaypointEntry(npc_waypoints), QuestTemplate(quest_template),
GameObjectTemplate(gameobject_template), GameObjectDb(gameobject), LootEntry(loot), GossipDb/GossipOptionDb(gossip/_option),
GraveyardDb(graveyard), DialogDb(dialog), MapTemplate(map), DungeonTemplate(dungeon_template), ArenaTemplate(arena_template),
ZoneTemplateDb(zone_template), ModelInfoDb(model_info), Condition(conditions), CombineItemsDb(combine_items),
**PlayerClassStatEntry(player_class_stats: class,level→hp,mana,str,agi,will,int,courage = BASE STATS)**,
PlayerExpLevel(player_exp_levels), PlayerCreateItemEntry(player_create_item), MaterialChance (armor/weapon).
Base-stat loader = FUN_004afcd0 → tree keyed (class,level).

## Player persistence (MySQL `player` + child tables, from SQL dump)
player (guid,account,name,class,gender,portrait,level,hp_pct,mp_pct,xp,money,position,map,home,pvp,progression,
combat_rating,num_invested_spells...), player_inventory/equipment/bank (slot,entry,affix,affix_score,gem_1..4,
enchant_level,soulbound,count,durability), player_spell, player_aura (full tick state), player_spell_cooldown,
player_quest_status, player_stat_invest, player_waypoints, player_mail, account_* , auth_tokens (token, 60s TTL).

## Auth flow (FUN_0048a400)
Client_Authenticate{token,build,fingerprint} → reject build<1188 (Validate result 4) → MD5 fingerprint store →
`SELECT user_id,token FROM auth_tokens WHERE token=? AND created_at > NOW()-60s` → set accountId → Validate(result, time).

## To reconstruct the server (roadmap)
1. Net layer: Session + dispatch (have opcode→handler map) + GP_* (un)pack (decompile handlers for layouts).
2. DB layer: SqlConnector (have, reconstructed client-side) + all loaders (SQL is known; map rows→structs).
3. Object/world: ServerObject/Unit/Player/Npc + ServerMap + variable system (ObjDefines::Variable, known).
4. Movement: UnitMotionGen + generators (decompile ChaseMovement/PointMovement for path algo).
5. Spells/auras/effects: Effect_*/Aura_* (decompile a few; map to SpellTemplate columns).
6. Stat calc: base (player_class_stats) + invest + item (crunchItemStats) + auras.
Each system is enumerable from RTTI; decompile representative funcs to fill formulas.
