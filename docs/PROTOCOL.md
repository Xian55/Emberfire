# Dreadmyst wire protocol — decoded from live capture (2026-05-31)
Source: `../capture/session-2026-05-31T14-51-41-101Z.*` (test/test, chars Baday+Xii, full session).
Decoder: `../capture/analyze.js`. Reassembly was clean (350 C2S + 1444 S2C frames, 0 desync) — high confidence.

## Framing  (SfSocket)
`frame = [uint32 LE bodyLen][body]`, `body = [uint16 LE opcode][payload]`, `payload len = bodyLen-2`.
bodyLen counts opcode+payload, NOT the 4-byte length prefix. Stream-reassemble per direction before splitting.

## StlBuffer encoding  (operator>> / operator<<)
Natural C++ type widths, **little-endian**:
- `uint8_t`/`bool` → 1 byte; `uint16_t` → 2; `int`/`uint32_t`/guid → 4; `float` → 4 (IEEE-754 LE).
- `std::string` → **null-terminated C-string** (bytes until 0x00; no length prefix).
No length/type tags; fields are read back-to-back in the same order they were written.

## DYMST_VERSION = 1189   (server r1188 accepts it; from Client_Authenticate.m_build)

## Confirmed opcodes (value → name/meaning)
- 1  `Mutual_Ping` — 8-byte payload (ms timestamp), client echoes it back. Both directions.
- 2  `Client_Authenticate` (C2S) = `{ token:cstr, build:u32(=1189), fingerprint:cstr }`.
- 3  `Server_Validate` (S2C) = `{ result:u32, serverTime:u32, ?:u32 }` (result=0 ⇒ success here).
- 75 `Server_CharacterList` (S2C) = `{ count:u32, [ name:cstr, classId:u8, level:u8, guid:u32, portrait:u8, gender:u8 ] * count }`.
  Decoded: Baday(cls1 lvl5 guid1 portrait0x62 gender0), Xii(cls3 lvl1 guid2 portrait0x33 gender0).

## Opcode ranges observed (names via Game.cpp switch order, to finalize during impl)
- **Client (C2S)** opcodes seen: 1,2,4,5,7,9,13,15,16,27,29,30,33. So Client block ≈ 2..~70 interleaved with Mutual.
  - 15 (0xf) x295 — most frequent C2S = movement (`{x:float, y:float, wasd:u8}`-ish) → Client_RequestMove.
  - 16 (0x10) x18 — `{float,float}` → likely Client_RequestStop.
- **Server (S2C)** opcodes seen: 3, then a dense run 75..113+ = world/state packets:
  76,77(1050B world/new-world blob),78(Player),79(Npc),80,82(x156 freq),84,85(638B),86,87,90(0x5a x454 — most
  frequent, 24B, position/spline update),91(durability chat msg),95,98,99,100,102,103,104,105,107,108,109,110,111,112,113.
  Exact name↔value: cross-map by decoding each `processPacket_Server_*` handler field order vs first-payload hexdump
  (analyze.js prints first occurrence of every opcode).

## Implications for reconstruction (GO confirmed)
StlBuffer + SfSocket + GamePacket framework + Opcode enum can be implemented directly from the above.
A headless harness can: HTTP-auth (token) → TCP connect → send Client_Authenticate(token,1189,fp) →
receive Server_Validate(3) → (Client_CharacterList) → decode Server_CharacterList(75). Protocol risk retired.

## ★ GO PROVEN (2026-05-31)
`build/harness/harness.cpp` (MSVC, raw winsock+winhttp, no SFML) compiled from this spec:
authenticated via node auth, TCP-connected, sent Client_Authenticate(token,1189,fp), received
Server_Validate(result=0), decoded Server_CharacterList = Baday/Xii/Yii (matches DB). Protocol risk retired.

## Enter-world opcode observations (headless client, entered as guid 3)
Client: Authenticate=2, CharCreate=4 `{gender:u8,portrait:u8,name:cstr,classId:u8}`, EnterWorld=5 `{guid:u32}`,
RequestMove=15 `{x:float,y:float,wasd:u8}`, RequestStop=16 `{x:float,y:float}`.
Server after EnterWorld(5): 76=CharaCreateResult/EnterAck(1B), 77=NewWorld/map(1063B), 78=Player(199B),
79=Npc(195B), 80(4B), 82=ObjectVariable?(9B, very frequent), 85=Spellbook(638B), 90=UnitSpline/pos(24B),
95(4B), 108=var-update(6B, frequent), 109=auras(65B), 111, 120, 141, 143=server heartbeat(16B, periodic stream).
Sequencing: must wait for Server_Validate+CharacterList before sending EnterWorld (premature enter ⇒ server drops).
Headless client stays connected through the heartbeat stream — keepalive OK.

## Object serialization (decoded from op77/op78, headless client)
World objects share a layout:
`Unit/Player(78) = guid:i32, x:float, y:float, [marker: i32, u8, i32 (9 bytes)], [varId:u8, value:i32]*, orient:float, trailer:i32`
`NewWorld(77) = mapId:u32, myX:float, myY:float, [9-byte marker], [varId:u8,value:i32]*, name:cstr, gender:u8, portrait:u8, classId:u8, equipment...`
Variables are an `ObjDefines::Variable`-keyed sparse list (ids seen: 0x01..0x0d, 0xa1..0xad, 0xb0..0xbb).
Confirmed: **var 0x02 = Level** (Yii=1, a lvl5 mob=5). **var 0x01 ≈ MaxHealth** (lvl1 mob=60, lvl5 mob=375).
NPC/object runtime guids are negative-looking (0xfffffdxx). Decoded live: map=3, my pos=(62.7,92.5), level=1, name=Yii.
(Minor: greedy var-parse occasionally off-by-one on the orient field; refine by reading an explicit varCount.)

## ObjDefines::Variable ids — labeled (DB-cross-referenced across Yii lvl1 & Baday lvl5)
- 0x01 = **Health** (current): Yii 60, Baday 169 (=0.445*384 ✓)
- 0x02 = **Level**: Yii 1, Baday 5 ✓ (matches DB)
- 0x03 = **MaxHealth**: Yii 60, Baday 384
- 0x04 = **Mana** (current): Yii 45, Baday 163
- 0x0a = **MoveSpeedPct**: 100 for both (=normal speed)
- 0x05-0x09, 0x0b-0x0d = 0 for both players (combat/pvp/flags — undetermined)
- 0xa1-0xad, 0xb0-0xbb blocks (seen on NPC op78, 34-35 vars total) = StatsStart.. block → UnitDefines::Stat
  (Strength/Agility/resistances/etc); not emitted in self-NewWorld low block. Map later via stat panel.
- Movement verified: `move 70 92` as Yii → server Spline *MINE* to x=70, and DB position_x/y persisted to (70,92).
Method to label the rest: trigger an action (take damage, gain xp, get money) and watch op82 ObjVar {guid,varId,value}.

## Stat block → UnitDefines::Stat — LOCKED by equip experiment (StatsStart = 0x0e)
The **player** NewWorld sends the FULL contiguous var table 0x01..0xbb (NPCs send a sparse subset).
**Equip experiment:** applied affix 8116 "Guard" (stat_type1=3=ArmorValue) → ONLY `var 0x11` changed
(32→35), so **ArmorValue = var 0x11**. **StatsStart = 0x0e** and `varId = 0x0e + UnitDefines::Stat index`.
**Layout LOCKED by op77 raw dump** (2026-06-03 conn22-v1190 MAGETWO L5 mage, GEARED so ArmorValue>0;
+ conn6-v1189): `0x0e=0(WeaponValue, caster no melee wpn)`, `0x0f=355(=MaxMana 0x04)`, `0x10=215(=MaxHealth
0x03)`, `0x11=10(ArmorValue)`, `0x12=5(Str)`, `0x13=18(Agi)`, `0x14=29(Will)`, `0x15=28(Int)`, `0x16=11(Cou)`,
`0x17=12(Reg)`, `0x18=28(Med)`. Anchors: `0x1a=2000=MeleeCooldown`, `0x27=Bartering`, `0x23-0x26=Resists`.
⚠ The op40 SPEND-id order ≠ var order: spend ids = Health1,Mana2,ArmorValue3,Str4,…,Med10 (Health/Mana
swapped vs vars). Swap lives in `GP_Client_LevelUp::spendId`, NOT the enum. (A prior draft put ArmorValue at
0x0e → getStat read one var low → whole stat panel shifted one row. Fixed in UnitDefines.h; trust this dump.)
| varId | Stat | varId | Stat | varId | Stat |
|---|---|---|---|---|---|
|0x0e|WeaponValue|0x18|Meditate|0x22|ParryRating|
|0x0f|Mana|0x19|MeleeValue|0x23|ResistFrost|
|0x10|Health|0x1a|MeleeCooldown|0x24|ResistFire|
|0x11|ArmorValue|0x1b|RangedValue|0x25|ResistShadow|
|0x12|Strength|0x1c|RangedCooldown|0x26|ResistHoly|
|0x13|Agility|0x1d|MeleeCritical|0x27|Bartering|
|0x14|Willpower|0x1e|RangedCritical|0x28|Lockpicking|
|0x15|Intelligence|0x1f|SpellCritical|0x29|Fortitude|
|0x16|Courage|0x20|DodgeRating|0x2a|StaffSkill (Staves)|
|0x17|Regeneration|0x21|BlockRating|0x2b+|MaceSkill, AxesSkill, … (UnitDefines::Stat order)|

Confirmed non-stat object vars (DB cross-referenced, Baday): **0x9e=Money(1430), 0xa3=Progression(157),
0xa4=Experience(124), 0xa7=NumInvestedSpells(5)**. Core (earlier): 0x01=Health, 0x02=Level, 0x03=MaxHealth,
0x04=Mana, 0x0a=MoveSpeedPct. (Note: stat_type is the 0-based UnitDefines::Stat index; the affix/item
`stat_typeN` columns use the same indexing.)

### Parser note (fixed in harness)
Object var region = read [varId:u8, value:i32] pairs until exactly 8 bytes remain (orientation:float + trailer:i32).
Earlier greedy "id<=0x0d||id>=0xa1" parsing mis-read the orientation float's low byte (e.g. 0x3f4e6b**a1**) as a
phantom var 0xa1 — fixed by the length-based bound.

## To finalize per-packet layouts
Run `bun analyze.js` → for each Server opcode, match its first-payload hexdump against the field reads in the
corresponding `Game::processPacket_Server_<Name>` (Game.cpp). The capture is the ground truth for field order/width.
