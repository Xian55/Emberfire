# VERIFY_TODO — guessed/stubbed values, data-driven resolution

Status: ✅ proven · 🟡 ordinal proven, name needs binary/UI · ❌ open (needs binary or capture)
Source tags: `DB`=game.db query · `SRC`=client C++ · `BIN`=dump_client/decompiled.c · `CAP`=proxy capture

Truth DB path: `%EMBERFIRE_GAME_DIR%\game.db`.
Miners (reusable): `tools\capture\{schema_dump,enum_dump,xtab}.js`.

---

## A. Item enums (`item_template`) → `src\Shared\ItemDefines.h`

| symbol | DB col | proven values | status | note |
|--------|--------|---------------|--------|------|
| Quality | `quality` | 0..6 (7 levels; 0 special e.g. "Yellow Orchid"; 1=850, 2-6 ≈3200 ea) | 🟡 BIN | BIN `FUN_0048d940` is **crafting-grade** names 1=Normal,2=Fine,3=Superior,4=Exquisite,5=Pristine (default Unknown) — a DIFFERENT enum from item rarity. DB `quality` rarity↔name still needs call-site disambiguation (rarity vs grade vs gem grade). |
| ArmorType | `armor_type` | **0..15 = 16 values** | ✅ BIN | `FUN_0048e030` (@0048e030): 0=NullArmor,1=Cotton,2=Leather,3=Brigandine,4=Gambeson,5=Bronze,6=Steel,7=Mythril,8=Emerald,9=Crystal,10=Diamond,11=Titanium,12=Linen,13=Wool,14=Silk,15=Spiritsilk. Reconstructed Cotton(0)..Diamond(14)=**wrong** (off-by-one + wrong names). |
| WeaponMaterial | `weapon_material` | 1..14 (0=none) | ✅ BIN | `FUN_0048dce0` (@0048dce0): 1=Bronze,2=Steel,3=Mythril,4=Emerald,5=Crystal,6=Diamond,7=Titanium,8=Aspen,9=(3-char wood,~Yew @0631cd4),10=Cherry,11=Black Walnut,12=White Oak,13=Hard Maple,14=Hickory. Reconstructed Wep_Aspen(0)..Wep_Diamond(13)=**wrong**. |
| WeaponType | `weapon_type` | 0=none,1=Axe,2=Sword(Dragonslayer),3=Hammer,4=Blade,5=Staff,6=Combat Dagger,7=Magic Rod | 🟡 DB | weapon class → skill. Confirm names BIN. |
| EquipType | `equip_type` | 0=misc,1=Head,2=Neck,3=Chest,4=Waist,5=Legs,6=Feet,7=Hands,8=Finger,9=1H weapon,10=Shield/OffHand,11=2H weapon | 🟡 BIN | BIN `FUN_0048da90` (@0048da90) names: 1=Helm,2=Necklace,3=…(matches DB). Read remaining cases to finish. |
| required_class | `required_class` | 1..4 (only tomes/scrolls set it) | 🟡 DB | 4 player classes 1..4. Bind class names SRC. |
| ItemFlags | `flags` | ✅ **QuestItem=0x01, Skillbook=0x02, GoldValueScales=0x04** | ✅ BIN | editor checkbox order @~0x144591 = bit order; DB confirms Tome=flags 2=Skillbook. |
| num_sockets | `num_sockets` | 0..4 | ✅ DB | literal count, not enum. |

## B. Spell enums (`spell_template`) → `src\Shared\SpellDefines.h`  ⚠ badly misaligned

**Effect** (`effect1/2/3`) — ✅ **DEFINITIVE, from SERVER effect-factory switch** (`dump/decompiled.c` @0x576f00,
`switch(effectType){ case N: new Effect_X }`; case label = effect id; factory funcs FUN_0049de10..e730). **All
ambiguity removed.** NullEffect = default (gaps 9,12,15,16,20,21 unused in this content).

| id | Effect_ class | id | Effect_ class | id | Effect_ class |
|----|----|----|----|----|----|
| 1 | SchoolDamage | 17 | ManaBurn | 32 | LootEffect |
| 2 | Teleport | 18 | Threat | 33 | Kill |
| 3 | ApplyAura | 19 | TriggerSpell | 34 | Gossip |
| 4 | ManaDrain | 22 | InterruptCast | 35 | Inspect |
| 5 | HealthDrain | 23 | SummonObject | 36 | ApplyGemSocket |
| 6 | Heal | 24 | ScriptEffect | 37 | Charge |
| 7 | Resurrect | 25 | KnockBack | 38 | Duel |
| 8 | CreateItem | 26 | ApplyAreaAura | 39 | SlideFrom |
| 10 | SummonNpc | 27 | HealPct | 40 | ApplyOrbEnchant |
| 11 | RestoreMana | 28 | RestoreManaPct | 41 | LearnSpell |
| 13 | Dispel | 29 | TeleportForward | 42 | NearestWayp |
| 14 | WeaponDamage | 30 | MeleeAtk | 43 | PullTo |
|    |  | 31 | RangedAtk | 44 | DestroyGems |
|    |  |    |  | 45 | CombineItem |
|    |  |    |  | 46 | ExtractOrb |

Overrides earlier DB guesses: 17=ManaBurn (not Penance-guess), 18=Threat, 24=ScriptEffect (Multi-Shot),
25=KnockBack, 27=HealPct, 32=LootEffect (not Interact), 45=CombineItem (not CreateItem). effectN_data1 still
carries sub-param (school for 1/14, AuraType for 3, gem id for 36, spellbook idx for 41).

**AuraType** (`effectN_data1` when effect=ApplyAura/ApplyAreaAura) — ✅ **DEFINITIVE from SERVER aura-factory
switch** (`dump/decompiled.c` @0x49d000, `switch(aura->type)`; case=value, factory FUN_0049e800..ff00).
**Fixes the post-login freeze regression.** Every value cross-checked vs DB exemplar:

| val | Aura_ class | DB exemplar | val | Aura_ class | DB exemplar |
|----|----|----|----|----|----|
| 1 | PeriodicDamage | Blight | 13 | SchoolImmunity | Divine Protection |
| 2 | PeriodicHeal | potions | 14 | ModifyDamageDealtPct | Demoralizing Shout |
| 3 | InflictMechanic | Bellowing Roar | 15 | ModifyDamageReceivedPct | Aegis of Valor |
| 4 | ModifyStat | Blessing of Defense | 18 | PeriodicMeleeDamage | |
| 5 | ModifyStatPct | Arrow Flurry | 19 | Model | |
| 6 | AbsorbDamage | Blessed Shield | 20 | PeriodicBurnMana | Ethereal Sting |
| 10 | PeriodicRestoreMana | Mana Potion | 21 | Proc | |
| 11 | ModifyMoveSpeedPct | | 22 | ModifyHealingDealtPct | |
| 12 | MechanicImmune | Dark Resolve | 23 | ModifyHealingRecvPct | |
|    |  |  | 24 | PeriodicHealPct | |
|    |  |  | 25 | PeriodicRestoreManaPct | |

Gaps 7,8,9,16,17,26 → default `Aura_NullAuraType`. → The active `ModifyStat=0` ordering in the reconstructed
header is **wrong**; replace with this. (Confirm freeze gone via op108 capture, Phase E.)

| symbol | DB col | proven | status |
|--------|--------|--------|--------|
| TargetType | `effectN_targetType` | 0,1,2,3,13,14,15,17,19,20 present (matches reconstructed Unit_Caster=1…AreaDst=20) | 🟡 DB plausibly-aligned; names BIN |
| DispelType | `dispel` | 0=Physical,1=Magic,2=Curse,3=Disease(Blight),4=Poison | ✅ DB (aligned w/ reconstructed) |
| DamageSchool (cast) | `cast_school`/`effectN_data1` | ✅ **0=NullMagicSchool,1=Physical,2=Frost,3=Fire,4=Shadow,5=Holy** | ✅ BIN | school-name switch @~0x239600 in decompiled.c; matches effect1.data1 + cast_school exemplars. Reconstructed Physical(0)..Holy(4)=**off-by-one**. |
| magic_roll_school | `magic_roll_school` | 0..5, DIFFERENT meaning (1=Aegis/Holy-heavy) | ❌ separate roll/resist enum, NOT the damage school; confirm semantics BIN |
| SpellAttributes | `attributes` | **64-bit** mask; bit order = string-pool order (see appendix), bits 0..37 | ✅ BIN | **3 DB confirms**: Divine Strike=2→bit1 AutoApproach, food=2²²→bit22 Triggered, gems=2³⁵→bit35 MouseoverTargeting. |
| prevention_type | `prevention_type` | mask 0,1,2,8,16,8208 | ❌ BIN |
| aura_interrupt_flags | `aura_interrupt_flags` | mask 0,32,48,64 | ❌ BIN |
| cast_interrupt_flags | `cast_interrupt_flags` | mask 0,8,12,24,40,64 | ❌ BIN |

## C. NPC enums (`npc_template`) → `src\Shared\NpcDefines.h`

| symbol | DB col | proven | status |
|--------|--------|--------|--------|
| AIType | `ai_type` | 0=Melee, 1=Caster(Battle Mage), 2=Archer(Kerbogh Hunter) | ✅ DB (aligned) |
| MovementType | `movement_type` | 0=Idle,1=Random,2=Patrol | 🟡 DB plausibly-aligned |
| CreatureType | `type` | 0..4 (0,1=beast,2,3,4=humanoid families) | 🟡 DB; names BIN |
| NpcFlags | `npc_flags` | ✅ **0x01=Gossip, 0x02=QuestGiver, 0x04=Vendor, 0x800=SpendExpCredit** | ✅ low bits SOLID | corroborated across 130+ named NPCs (session 22-22-18): 0x1=Guard, 0x3=questgivers, 0x5=vendors(Boltar/Karl), 0x7=all. reconstructed SpendExpCredit=0x20 **wrong** (=0x800). Rare high bits 0x200(Vincent)/0x400 still unknown, **not on wire** — accept DB or server-RE. |
| game_flags | `game_flags` | mask; 136=0x88 (Grave Horror) | ❌ BIN |
| faction | `faction` | 1,2,3 | 🟡 DB; names SRC |

## D. GameObject enums (`gameobject_template`) → `src\Shared\GameObjDefines.h`

| symbol | DB col | proven | status |
|--------|--------|--------|--------|
| GoType | `type` | **1=Container,2=Waypoint,3=Portal/MapChanger,4=Dungeon** | 🟡 DB | reconstructed Container=0 is **wrong**; DB starts at 1. Names BIN. |
| GoFlags | `flags` | ✅ **ShowName=0x01,Uninteractable=0x02,RenderOnFloor=0x04,Lockpicking=0x08,Lv2xForLockpick=0x10,NoMouseoverBrighten=0x20** | ✅ BIN | editor order @~0x121142 = bit order; DB confirms Aspen Chest=9=ShowName+Lockpicking, Strongbox=25=+Lv2x. Reconstructed had Lockpicking=0x10=**wrong**. |

## E. Quest enums (`quest_template`) → `src\Shared\QuestDefines.h`

| symbol | proven | status |
|--------|--------|--------|
| QuestFlagRepeatable | `flags` 1 = "The Arena (Repeatable)" | ✅ DB (0x01 confirmed) |
| TallyType | objective implied by which req column set (req_npc/req_item/req_go/req_spell) | 🟡 SRC | column order ⇒ TallyNpc/Item/GameObj/Spell; confirm order in Game.cpp quest handler |

## F. Unit stats/skills (`player_class_stats` columns) → `src\Shared\UnitDefines.h`

Column order = authoritative Stat then Skill order:
`HP, Mana, Strength, Agility, Willpower, Intelligence, Courage` then skills `Staves, Maces, Axes, Swords, Ranged, Daggers, Wands, Shields`.
→ ✅ matches reconstructed skill order (Staff=29,Mace=30,Axe=31,Sword=32,Ranged=33,Dagger=34,Wand=35,Shield=36). Stat block already locked at object-var `0x0e + Stat`.

---

## G. Packet layouts → `src\Shared\GamePackets_Stubs.h` (Phase B, from Game.cpp)
Read each `Game::processPacket_Server_*` field-read order. Priority (previously broke):
UnitSpline(90) ❌→ · NotifyItemAdd(103) ❌→ · Inventory/Bank(85/141) ❌→ · BuyVendorItem(35) ✅ (GHIDRA FUN_00484ca0: u16,u16,u8,u8,u32) · EquipItem(9) ✅ (FUN_00484d80) · AcceptQuest(8)/CompleteQuest(33)/AbandonQuest(19) ✅ (see GHIDRA_FINDINGS) · op41 SpellRankInvest ✅ (FUN_004929a0) · PartyChanges/LevelUp/UpdateArenaStatus ❌→ · remainder.

## H. Opcode values → `recon\OPCODES.md` (BIN done, CAP remainder)
`dump_client\opcodes.txt` gives RTTI-confirmed values (folded into OPCODES.md). Still `= ?` (inlined, need CAP):
Client_LevelUp · Server_Player · Server_Npc · Server_Object · Server_GameObject · Server_GossipMenu · Server_GuildRoster · Server_UnitAuras · Server_UnitSpline · Server_TradeUpdate.

## I. Client-side enums (Phase C, from SRC headers — values present in source)
ChatChannel(GameChat.h) · SpellHitResult(CombatMessage.h) · AuraRender(BuffDebuffRenderer.h) · EquipSlot/Skill(Equipment.h) · UnitAnimation(ClientUnit.h) · UnitFrame stats(UnitFrame.h).

---

## Appendix — Phase D string-pool / switch findings (`dump_client`)

Name-mapping switches found in `decompiled.c` (param = enum int → returns name):
- **ArmorType** `FUN_0048e030` — full 0..15 (above).
- **WeaponMaterial** `FUN_0048dce0` — full 1..14 (above).
- **Quality(grade)** `FUN_0048d940` — 1=Normal,2=Fine,3=Superior,4=Exquisite,5=Pristine,default=Unknown.
- **EquipType** `FUN_0048da90` — 1=Helm,2=Necklace,3=…(finish).
- **MagicSchool** `@~0x239600` — 0=NullMagicSchool,1=Physical,2=Frost,3=Fire,4=Shadow,5=Holy. ✅

**Enum NAME SETS** recovered from the editor string pool (`strings.txt` 0x63d8xx–0x63e5xx). These are the
complete authoritative names; VALUE↔name for effects/auras bound via DB exemplars (pool order ≠ value order).

- **SpellEffect** names: Teleport, SchoolDamage, Charge, ApplyAura, DestroyGems, ApplyGemSocket, ExtractOrb,
  CombineItem, ApplyOrbEnchant, HealthDrain, ManaDrain, Resurrect, SummonNpc, CreateItem, Dispel, RestoreMana,
  ManaBurn, WeaponDamage, TriggerSpell, Threat, SummonObject, InterruptCast, KnockBack, ScriptEffect, HealPct,
  ApplyAreaAura, TeleportForward, RestoreManaPct, RangedAtk, MeleeAtk, LootEffect, SlideFrom, Inspect,
  NearestWp, LearnSpell. (DB-bound values: 1=SchoolDamage,2=Teleport,3=ApplyAura,14=WeaponDamage,36=ApplyGemSocket,
  40=ApplyOrbEnchant,41=LearnSpell,44=DestroyGems,45=CombineItem,46=ExtractOrb; ?17,18,24,25,27 still open.)
- **AuraType** names: PeriodicDamage, PeriodicHeal, PeriodicMeleeDamage, InflictMechanic, PeriodicHealPct,
  ModifyStatPct, ModifyStat, ModifyResistance, AbsorbDamage, PeriodicRestoreMana, PeriodicTriggerSpell,
  PeriodicBurnMana, PeriodicRestoreManaPct, MechanicImmunity, ModifyMoveSpeedPct, ModifyDmgDealtPct,
  SchoolImmunity, ModifyHealingDealtPct, ModifyDmgReceivedPct, ModifyMeleeSpeedPct, ModifyHealingRecvPct,
  ModifyRangedSpeedPct. **DB-locked values: 1=PeriodicDamage, 2=PeriodicHeal, 3=InflictMechanic, 4=ModifyStat**
  (the freeze fix). Rest (5,6,10,12,13,14,15,16,20,26) bind to remaining names — confirm via op108 capture.
- **SpellAttributes** (bit = index, 0-based, ✅ confirmed): 0 CantTargetSelf, 1 AutoApproach, 2 CantCrit,
  3 CanTargetDead, 4 TargetsGround, 5 IgnoreArmor, 6 IgnoreInvulnerability, 7 TargetsItem, 8 IgnoreResistances,
  9 IgnoreLOS, 10 ImpossibleDodge, 11 ImpossibleBlock, 12 ImpossibleParry, 13 ImpossibleMiss, 14 NoSpellBonus,
  15 NoHealBonus, 16 NotInCombat, 17 NoThreat, 18 OnePerTarget, 19 OnePerCaster, 20 SameStackForAllCasters,
  21 Passive, 22 Triggered, 23 TargetNotInCombat, 24 NoGoLock, 25 AnimLockStart, 26 IgnoreStun,
  27 DontStopCastingSound, 28 IgnoreSleep, 29 IgnoreIncapacitated, 30 IgnoreFear, 31 IgnoreConfused,
  32 TargetPlayersOnly, 33 IgnorePolymorph, 34 PersistsThroughDeath, 35 MouseoverTargeting, 36 NotInDungeon,
  37 NotInArena.
- **Mechanic** (CC) names: (Stun), Confused, Pacify, Silence, Snare, Sleep, Incapacitated, Polymorph, Disrupt.
- **UnitAnimation** names: Stance, Swing, Shoot, Cast, CastAlt, Spawn, CritDie, Die (+ Run/Hit/Block).
- **Stat** names: ArmorValue, MeleeCooldown, WeaponValue, RangedCooldown, RangedWeaponValue, RangedCritical,
  MeleeCritical, DodgeRating, SpellCritical, ResistFrost, BlockRating, ResistShadow, ResistFire, StaffSkill,
  ResistHoly, AxesSkill, MaceSkill, RangedSkill, SwordSkill, WandSkill, DaggerSkill, NpcMeleeSkill, ShieldSkill,
  ParryChanceBonus, NpcRangedSkill, DodgeChanceBonus, BlockChanceBonus. (order = pool, not value; cross-bind to
  0x0e+Stat object-var lock.)
- **NpcFlags** names: Gossip, Vendor, QuestGiver, LevelToCredit, TalkCredit, SpendExpCredit. DB-bound bits:
  0x01 Gossip, 0x02 QuestGiver, 0x04 Vendor, 0x800 SpendExpCredit; 0x200/0x400 likely TalkCredit/LevelToCredit.
- **NpcGameFlags** names: NoAggro, AggroZone, NoTaunt, NoFight, NoMove, NoAssist, NoCallAssist, NoMelee.
  136=0x88 ⇒ NoTaunt(0x80)+NoMelee(0x08). Bit order: verify in NpcTemplateEditor.
- **Spell interrupt flags** names: Movement, TakeDamage, StartCasting, DisruptMechanic, TakeDamage_Direct.
  Bit order pending editor (DB cast_interrupt 8 dominant = likely TakeDamage_Direct=0x08).
- **ConditionType** names (→ Conditions.h): Source_Player_HasItemEquipped, Source_Player_HasItemInBag,
  Source_Player_LevelGreaterThan, Source_Player_HasItemBagOrEquipped, Source_Unit_HasAura, Target_*…,
  Quest_PlayerHasQuest, Quest_PlayerHasOrDidQuest, Quest_PlayerHasQuestForItem, Quest_PlayerHasQuestFinishedInLog,
  Quest_PlayerTurnedInQuest.
- **ZoneType** names: Friendly, PlayerDefault, Hostile, Neutral (reconstructed had Friendly/Neutral/Hostile/Contested
  → likely Friendly,PlayerDefault?,Hostile,Neutral; confirm).
- **AIType** names: MeleeAI, ArcherAI, CasterAI — DB-bound 0=Melee,1=Caster,2=Archer (above).

Editor checkbox-creation order = bit order (confirmed pattern). Pinned this way:
- **ItemFlags** @0x144591: QuestItem=0x01, Skillbook=0x02, GoldValueScales=0x04. ✅
- **GoFlags** @0x121142: ShowName=0x01, Uninteractable=0x02, RenderOnFloor=0x04, Lockpicking=0x08,
  Lv2xForLockpick=0x10, NoMouseoverBrighten=0x20. ✅

- **ProcFlags** @0x253478 (same editor idiom; 🟡 no DB cross-check): bit0 HolderTookDamage, bit1 HolderDealtDamage,
  bit2 HolderWasImmune, bit3 HolderDodged; `RemoveCharge` is a separate proc-TYPE field. Reconstructed had
  DealtDamage=0x01/TookDamage=0x02 swapped — editor order says TookDamage first.

Still OPEN in D — **blocked**: NpcFlags high bits / NpcGameFlags / spell interrupt flag bit-orders.
Their name strings (Gossip/Vendor/NoAggro/…) live in a data table NOT referenced by the *partial* client
`decompiled.c` (NpcTemplateEditor wasn't fully decompiled). Resolve via: full SERVER dump
(`deps/ghidra/dump/decompiled.c`) flag-test sites, or Phase E capture. DB already pins NpcFlags low bits
(0x01 Gossip, 0x02 QuestGiver, 0x04 Vendor, 0x800 SpendExpCredit) and NpcGameFlags 136=0x88=NoTaunt+NoMelee.
Also open: item **rarity** Quality (0..6) name set (≠ crafting grade); effect ordinals 17,18,24,25,27.

## Appendix 2 — Server dump (`deps/ghidra/dump`) findings

- **Effect enum** — definitive map from effect-factory `switch` @~0x576f00 (see §B table). Class set =
  `Effect_*` (SchoolDamage,Teleport,ApplyAura,ManaDrain,HealthDrain,Heal,Resurrect,CreateItem,SummonNpc,
  RestoreMana,Dispel,WeaponDamage,ManaBurn,Threat,TriggerSpell,InterruptCast,SummonObject,ScriptEffect,
  KnockBack,ApplyAreaAura,HealPct,RestoreManaPct,TeleportForward,MeleeAtk,RangedAtk,LootEffect,Kill,Gossip,
  Inspect,ApplyGemSocket,Charge,Duel,SlideFrom,ApplyOrbEnchant,LearnSpell,NearestWayp,PullTo,DestroyGems,
  CombineItem,ExtractOrb,NullEffect).
- **AuraType enum** — definitive from aura-factory `switch(aura->type)` @~0x49d000 (see §B table).
- **SQL column order = struct/load order** (authoritative for field layouts; from server SELECT strings):
  - `npc_template`: entry,name,model_id,model_scale,min_level,max_level,faction,type,npc_flags,game_flags,
    gossip_menu_id,movement_type,path_id,ai_type,mechanic_immune_mask,resistance_holy/frost/shadow/fire,
    strength,agility,intellect,willpower,courage,armor,health,mana,weapon_value,melee_skill,ranged_skill,
    ranged_weapon_value,melee_speed,ranged_speed,loot_{green,blue,gold,purple}_chance,custom_loot,
    custom_gold_ratio,shield_skill,bool_elite,bool_boss,leash_range,spell_primary,spell_1..4_{id,chance,interval,cooldown,targetType}.
  - `gameobject_template`: entry,flags,required_item,required_quest,type,data1..data11.
  - `quest_template`: entry,min_level,flags,prev_quest1..3,provided_item,req_item1..4,req_npc1..4,req_go1..4,
    req_spell1..4,req_count1..4,rew_choice1..4_item,rew_choice1..4_count,rew_item1..4,rew_item1..4_count,
    rew_money,rew_xp,start_script,complete_script,start_npc_entry,finish_npc_entry.
    → **TallyType** order inferred: req_item / req_npc / req_go / req_spell (4 objective kinds).
  - `gossip`/`gossip_option`: have condition1..3{,_value1,_value2,_true} + (option) required_npc_flag,click_new_gossip,click_script.
  - `loot`: entry,lootId,item,chance,count_min,count_max,condition1..2{...}.
  - Runtime `player_aura` row = guid,caster_guid,spell_id,miliseconds,m_stackCount,m_spellLevel,m_casterLevel,
    then 3× tick blocks {m_tickTotal,m_tickAmount,m_tickAmountTicked,m_casterGuid,m_numTicks,m_numTicksCounter,
    m_tickTimer,m_tickIntervalMs} → informs op108 UnitAuras layout (Phase B/E).
- **Server strings are name-stripped** for flag enums: NpcFlags high bits / NpcGameFlags / spell interrupt-flag
  bit-orders are NOT recoverable from either dump → **Phase E capture only** (op82 vars, npc spawn flags on wire),
  or accept DB-derived low bits. Remaining open after full crunch: those flag bit-orders + item **rarity**
  Quality (0..6) names + AuraType gaps (Drunk=16, Reincarnation=26 fall to NullAuraType).

## Appendix 3 — Phase E live capture (Steam client, `session-2026-06-01T18-55-22-665Z.ndjson` ONLY)

⚠ Only this one session is confirmed Steam-client origin. Other `session-*.ndjson` are ambiguous (may be our
decompiled client) — do NOT use them. New Steam captures available on request.

**Opcodes resolved** (folded into OPCODES.md): LevelUp=40(C2S), Player/NewWorld=77, Npc=78, GameObject=79,
GuildRoster=95, UnitSpline=90, UnitAuras=108, GossipMenu=110. Still unseen this session: Server_Object base,
Server_TradeUpdate (no trade performed).

**ObjectVariable IDs** (op82, 2334 samples) — confirmed by value behavior:
- ✅ 0x9e Money (2844→2919, changes on buy/sell), 0xa4 Experience (391→438 rising), 0xa3 Progression (175→222)
- ✅ 0xb8 Lootable (0/1), 0xb9 GossipStatus (0/3 — matches GossipAvailable=3), 0x05 InCombat (0/1)
- ✅ stat block present at 0x0f+ (0x0f=234,0x10=545,... = Health/Mana/Armor/Str/Agi/... per 0x0e+Stat lock)
- 0xb7 Boss(0), 0xba Moderator(0), 0xbb(0/1 new var), 0xb4 ReactionMask(2), 0xad CombatRating(0/2)
- **LEAD**: var **0x06** carries mask values [128, 2048, 4] = exact npc_flag bit values (4=Vendor, 2048=SpendExpCredit)
  → 0x06 likely echoes a flag mask to the client; a capture of NPCs with known flags could finally bind the
  NpcFlags high bits on the wire.

**op90 UnitSpline layout** confirmed (24B, cnt=1): `guid:u32, flagA@4, startX:f32, startY:f32, flagB@13, count:u16, points[8×count]`.
All NPC samples had flagA=flagB=0 → silent vs slide NOT disambiguated (need a player self-slide sample).

**Session 2** (`session-2026-06-01T22-03-47`, 2 Steam clients, 4 conns; players "Trade"=guid4, "Baday"=guid1)
— resolved the social/trade/duel opcodes (full layouts in OPCODES.md):
- ✅ **ChatChannel**: Whisper=2, Party=4, 9=system/exp-notice → **corrects** reconstructed ChatDefines (Party was 3, Guild 4).
- ✅ Chat C2S(17)=channel:u8,text,[target]; S2C ChatMsg(91)=senderGuid:u32,channel:u8,text,senderName.
- ✅ Party: Inv(47)=name, InvResp(48)=accept:u8, OfferParty(132)=name, PartyList(133)=leaderGuid:u32,count:u8,[guid]×.
- ✅ Duel: OfferDuel(88)=name, DuelResponse(39)=accept:u8.
- ✅ Trade: OpenTrade(52)=name, RemoveItem(54)=slot:u32, SetGold(57)=gold:u32, Confirm(56)/Cancel(55)=empty,
  TradeUpdate(137)=partnerGuid:u32+slots+gold, TradeCanceled(138)=empty. → resolves Server_TradeUpdate (was `=?`).
- ChatError(89)=code:u16.

**Session 3** (`session-2026-06-01T22-14-32`, SELF guid=1) — closed the freeze + chat + guild:
- ✅ **op90 silent/slide FREEZE FIX confirmed**: self-move splines = silent@4=1, slide@13=0; NPC=0,0.
  Reading byte4(silent) as slide caused permanent slide→freeze. Keep silent@4, slide@13.
- ✅ **ChatChannel**: 0=Say,1=Yell,2=Whisper,3=Guild,4=Party,5=System,6=Global/AllChat,8=notice,9=RedWarning.
- ✅ Guild/social opcode layouts (GuildCreate/Motd/RosterReq/Roster, SetSubname, SetIgnore, RollDice, YieldDuel) — see OPCODES.md.
- ✅ **var 0x06 = unit flag mask** (not PvP): npc→0x80, player→0x800. Still need Malte (0x207) to bind NpcFlags bit-order.

**Session 4** (`session-2026-06-01T22-22-18`, town, 130+ NPCs) — **negative result + corroboration**:
- ❌ **var 0x06 ≠ npc_flags** — 0 for all town NPCs in op78 spawn despite non-zero DB npc_flags. Template
  NpcFlags are NOT broadcast; 0x06 is a dynamic mask (mostly 0; was 0x80/0x800 transiently earlier). Dead lead.
- ✅ **NpcFlags low bits SOLID** via 130 named NPCs joined to DB: 0x1=Gossip(Guard), 0x2=QuestGiver, 0x4=Vendor
  (0x5=Boltar/Karl vendors, 0x3=questgivers, 0x7=Aedis/Huldria). bit0/1/2 confirmed by behavior+name.
- Client derives vendor/gossip/quest from gossip_menu_id + GossipStatus(var 0xb9) + op110, not a flags var.
- Only rare high bits (0x200 Vincent, 0x400) remain — not on wire → accept DB / server-RE. **NpcFlags closed.**

### Capture requests (to close remaining gaps — most other open items are template/local-render, NOT on wire)
1. **silent/slide byte** (the freeze cause): control player, trigger a Charge / knockback / slide / forced-move on
   YOUR character, capture op90 self-splines where a flag byte ≠ 0 → bind which of byte4/byte13 = `slide`.
2. **NpcFlags high bits**: capture spawns (op78/op82 var 0x06) of NPCs known to be Vendor+QuestGiver+SpendExp
   (e.g. Malte npc_flags=519=0x207, "Spend Experience Points"=2048) → confirm 0x06 = flag mask + bit order.
3. **Server_TradeUpdate / Server_Object** opcodes: perform a player-to-player trade → bind those last opcodes.
Not capturable (resolve elsewhere): NpcGameFlags/interrupt bit-order (template only), item rarity Quality names
(client-side tooltip render → client-dump dig), AuraType-on-wire (op108 sends spell id only — already server-locked).

## J. Pure stubs (impl must match server; not enum bindings)
MapLogic A*/LOS · GameMap iso projection · ItemDefiner stat crunch · MutualUnit stat-mod formula · Util RNG/expr-eval. Resolve from BIN `decompiled.c` formulas (recon/GHIDRA_*.md have leads).
