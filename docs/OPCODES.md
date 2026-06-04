# Dreadmyst opcode map (uint16 LE). C=client→server, S=server→client, M=mutual.
Confirmed = decoded/verified live. Tentative = observed value+size+context, name from Game.cpp switch/handler shape.

## Confirmed (value ↔ name ↔ layout)
| op | dir | name | payload |
|----|-----|------|---------|
| 1  | M | Mutual_Ping | u64 timestamp; client echoes it (keepalive) |
| 2  | C | Client_Authenticate | token:cstr, build:u32(=1189), fingerprint:cstr |
| 3  | S | Server_Validate | result:u32 (0=ok), serverTime:u32, ?:u32 |
| 4  | C | Client_CharCreate | gender:u8, portrait:u8, name:cstr, classId:u8 |
| 5  | C | Client_EnterWorld | guid:u32 |
| 15 | C | Client_RequestMove | x:float, y:float, wasd:u8 |
| 16 | C | Client_RequestStop | x:float, y:float |
| 75 | S | Server_CharacterList | count:u32, [name:cstr, classId:u8, level:u8, guid:u32, portrait:u8, gender:u8] |
| 76 | S | Server_CharaCreateResult | result:u8 (after CharCreate) |
| 77 | S | Server_NewWorld | mapId:u32, myX:float, myY:float, marker(9), [varId:u8,val:i32]*, name:cstr, gender:u8, portrait:u8, class:u8, equip... |
| 78 | S | Server_Npc | guid:i32, x:float, y:float, marker(9), [varId:u8,val:i32]*, orient:float, entry/trailer (no name) |
| 79 | S | Server_GameObject | object-shaped, ~195B (guid,pos,vars) |
| 80 | S | Server_SetController | guid:u32 (the unit you control) |
| 82 | S | Server_ObjectVariable | guid:i32, variableId:u8, value:i32 |
| 90 | S | Server_UnitSpline | guid:i32, …, dest coords (movement path) |
| 91 | S | Server_ChatMsg | senderGuid:i32, channel:u8, text:cstr (saw "Battle wears on your equipment…") |

## Other client opcodes observed (C2S, from capture; names tentative)
7 (8B), 9 (14B, looks like CastSpell {targetGuid,spellId,x,y,ctm}), 13 (2B), 27 (17B), 29 (4B), 30 (10B), 33 (12B).

## Other server opcodes observed (S2C; size×count from capture, names tentative by switch order/content)
84(4B), 85(638B = Server_Spellbook), 86(18B), 87(2B), 95(4B), 98(10B), 99(6B), 100(26B), 102(17B),
103(16B), 104(33B), 105(8B), 107(12B), 108(6B {guid,u16} orient/flag), 109(65B auras), 110(28B), 111(15B),
112(4B), 113(13B), 120(3B), 141(638B Spellbook_Update?), 143(16B periodic server heartbeat/time).

## Note: enum values are NOT in Game.cpp switch order
e.g. switch lists CharacterList→NewWorld→CharaCreateResult, but actual values are 75/77/76. Values follow
GamePacketServer.h declaration order. Map remaining by: (a) trigger the action and watch the new opcode, or
(b) decode the payload and match the field reads in the matching processPacket_Server_<Name> handler.

## Method to finish the map
The headless client (`build/harness`) prints every opcode+size. Trigger actions to bind names:
move→90, chat→91, take damage→82(Health var), gain xp/gold→82, cast→9/SpellGo, etc. Cross-check payload
shape against the handler at the Game.cpp line for that name.

## Binary-confirmed opcode values (RTTI vtable slot0, `deps/ghidra/dump_client/opcodes.txt`)
These are the real `Opcode` enum values read out of the client binary — authoritative, no capture needed.

Client→Server:
| op | name | | op | name | | op | name |
|----|------|-|----|------|-|----|------|
| 2  | Client_Authenticate | | 23 | Client_GuildCreate | | 49 | Client_PartyChanges |
| 4  | Client_CharCreate | | 24 | Client_GuildInviteResponse | | 52 | Client_OpenTradeWith |
| 9  | Client_EquipItem | | 25 | Client_GuildPromoteMember | | 59 | Client_UnBankItem |
| 11 | Client_UseItem | | 26 | Client_GuildDemoteMember | | 62 | Client_MoveInventoryToBank |
| 15 | Client_RequestMove | | 27 | Client_CastSpell | | 67 | Client_UpdateArenaStatus |
| 17 | Client_ChatMsg | | 31 | Client_SetIgnorePlayer | | 70 | Client_MOD_MutePlayer |
| 18 | Client_GuildMotd | | 37 | Client_Repair | | 71 | Client_MOD_KickPlayer |
| 22 | Client_GuildInviteMember | | 47 | Client_PartyInviteMember | | 72/73/74 | MOD_Ban/Warn/Report |

Server→Client:
| op | name | | op | name | | op | name |
|----|------|-|----|------|-|----|------|
| 75 | Server_CharacterList | | 100 | Server_SpellGo | | 132 | Server_OfferParty |
| 83 | Server_InspectReveal | | 103 | Server_NotifyItemAdd | | 133 | Server_PartyList |
| 85 | Server_Inventory | | 104 | Server_OpenLootWindow | | 135 | Server_MarkNpcsOnMap |
| 88 | Server_OfferDuel | | 109 | Server_Spellbook | | 139 | Server_GuildNotifyRoleChange |
| 91 | Server_ChatMsg | | 111 | Server_QuestList | | 140 | Server_GuildOnlineStatus |
| 94 | Server_GuildAddMember | | 120 | Server_AvailableWorldQuests | | 141 | Server_Bank |
| 96 | Server_SetSubname | | 122 | Server_Spellbook_Update | | 143 | Server_ChannelInfo |
| 97 | Server_GuildInvite | | 130 | Server_QueryWaypointsResponse | | 150 | Server_PkNotify |

### `= ?` resolved by STEAM-client capture (`session-2026-06-01T18-55-22-665Z.ndjson`)
Live wire values from the real Steam client — authoritative:
| op | dir | name | seen | payload len |
|----|-----|------|------|-------------|
| 40 | C2S | Client_LevelUp | x1 | 64 |
| 77 | S2C | Server_Player/NewWorld | x1 | 1117 |
| 78 | S2C | Server_Npc | x137 | 199 |
| 79 | S2C | Server_GameObject | x45 | 195 |
| 90 | S2C | Server_UnitSpline | x1486 | 24 (cnt=1) |
| 95 | S2C | Server_GuildRoster | x1 | 4 |
| 108 | S2C | Server_UnitAuras | x231 | 6 |
| 110 | S2C | Server_GossipMenu | x2 | 57 |
Still unseen (no trade/those actions in this session): Server_Object base, Server_TradeUpdate.

Other steam-confirmed S2C opcodes this session: 84(4B x173), 87 WorldError(2B x251), 98 CastStart(10B),
99 CastStop(6B), 101 UnitTeleport(16B), 105 UnitOrientation(8B), 106(1B), 113 QuestTally(13B), 118(4B),
119 ExpNotify(5B), 122 SpellbookUpdate(16B), 124 LvlResponse(1B), 125 AggroMob(4B), 127 UpdateVendorStock(7B),
129(2B), 134 OnObjectWasLooted(8B), 143 ChannelInfo(16B x3671 = heartbeat).
C2S extras: 38 ResetDungeons(0B), 42 ReqAbilityList?(0B), 58(0B), 66 AltStackLoot(1B).

#### op87 Server_WorldError — CONFIRMED layout + codes (session 02-20-53, combat)
Wire: `code:u16 LE` (len=4 = opcode2 + payload2). NOT i32 (proxy.js had i32→null bug, fixed).
Observed codes ↔ PlayerDefines::WorldError ↔ client toast string (World::error switch):
- 4  ×57  SpellNotReady  → "Spell isn't ready"      (ability on cooldown)
- 12 ×41  OutOfRange     → "Out of Range"
- 6  ×8   LineOfSight    → "Not in Line of Sight"
- 9  ×7   InvalidTarget  → "Invalid Target"
All four map to the opaque (0x9f0c12ff) toast path → enum ordinals 4/6/9/12 capture-confirmed.

### CombatMsg(102) byte analysis — RESOLVED (Ranger combat capture, session 01-18-19, 89 samples)
17B layout fully confirmed. **byte8 = hitResult** ✓ — duel showed 0:72 (all-normal, misleading), but Ranger
PvE shows byte8 ∈ {0:74 Normal, 6:9 Block, 7:9 Crit} with crits carrying ranged damage (e.g. b8=7 amt=-16
effect=31 RangedAtk). So the stub `m_spellResult@byte8` is CORRECT, and the HitResult enum fix
(Normal0/Miss1/Glancing2/Dodge4/Parry5/Block6/Crit7/Immune8) applies to the right byte. byte13 = **effect**
(3/14/30/31 = ApplyAura/WeaponDamage/MeleeAtk/RangedAtk). amount@9 (i16) correct. No code change needed.

### Applied to GamePackets_Stubs.h (Phase B, 2026-06-02)
Fixed stubs to match capture: **Client_ChatMsg(17)** channel u8 + drop item blob; **Server_PartyList(133)**
`leaderGuid:u32, count:u8, members×u32` (was count-first/leader-last → crashed); **Server_TradeUpdate(137)**
base `partnerGuid,myMoney,hisMoney,myReady,theirReady,myCount:u16,[items],theirCount:u16,[items]` (was
groups-first/partner-last → crashed on the 18B empty trade; item entry ~21B best-effort, TODO multi-item).
Verified already-correct: Server_ChatMsg(91), SetSubname(96), OfferDuel(88), OfferParty(132), and the C2S
duel/party/trade/guild/ignore/roll builders.

### Social / trade / duel layouts — confirmed from 2-client capture (`session-2026-06-01T22-03-47`)
Players: "Trade" (guid 4), "Baday" (guid 1). All payloads below are post-opcode body.

Chat:
- **C2S Chat (17)**: `channel:u8, text:cstr [, targetName:cstr]` (target present only for whisper).
- **S2C ChatMsg (91)**: `senderGuid:u32, channel:u8, text:cstr, senderName:cstr`.
- **ChatChannel** (observed): **Whisper=2, Party=4**; 9=system/exp-notice ("Light party, kill XP… reduced").
  → corrects reconstructed ChatDefines (had Party=3/Guild=4). Say/Yell/Guild/Global still need capture.
- **S2C ChatError (89)**: `code:u16` (saw 5,6,7).

Party:
- **C2S PartyInvMember (47)**: `targetName:cstr`.
- **C2S PartyInvResponse (48)**: `accept:u8`.
- **S2C OfferParty (132)**: `inviterName:cstr`.
- **S2C PartyList (133)**: `leaderGuid:u32, memberCount:u8, [memberGuid:u32]×count`.

Duel:
- **S2C OfferDuel (88)**: `challengerName:cstr`.
- **C2S DuelResponse (39)**: `accept:u8`.

Trade:
- **C2S OpenTrade (52)**: `targetName:cstr`.
- **C2S TradeRemoveItem (54)**: `slot:u32`.
- **C2S TradeSetGold (57)**: `gold:u32`.
- **C2S TradeConfirm (56)** / **TradeCancel (55)**: empty.
- **S2C TradeUpdate (137)**: `partnerGuid:u32, …slots(item:u16,affix:u16,…)…, gold, confirmFlags` (18B = empty slots+gold; grows with items, e.g. item 0x133f).
- **S2C TradeCanceled (138)**: empty.

Guild / misc (session 22-14-32):
- **C2S GuildCreate (23)**: `name:cstr`.  **C2S GuildMotd (18)**: `motd:cstr`.  **C2S GuildRosterReq (28)**: empty.
- **S2C GuildRoster (95)**: empty=`u32 0`; populated=`guildName:cstr, motd:cstr, …members{counts, name:cstr, rank}`.
- **S2C SetSubname (96)**: `guid:u32, guildTag:cstr` (text shown under char name).
- **C2S SetIgnore (31)**: `add:u8 (1=add,0=remove), name:cstr`.
- **C2S RollDice (68)**: empty (result broadcast via ChatMsg ch5 System: "Baday rolls 72 (1-100)").
- **C2S YieldDuel (21)**: empty.

ObjectVariable note: **var 0x06 = unit FLAG MASK** (not "PvP" as guessed) — npc guid -36→0x80, player→0x800
(=SpendExpCredit bit). Confirms 0x06 broadcasts a flag mask; exact bit↔name needs an NPC with known
multi-flags (e.g. Malte=0x207) to bind.

### Server_UnitSpline (op90) layout — confirmed from capture (24B, cnt=1):
`guid:u32, silent:u8@4, startX:f32@5, startY:f32@9, slide:u8@13, count:u16@14, [destX:f32, destY:f32]×count`
✅ **CONFIRMED + FREEZE FIX** (session 22-14-32, SELF guid=1): self-movement splines = silent@4=**1**, slide@13=**0**;
NPC splines = both 0. So **byte4=silent** (set on every self-move echo), **byte13=slide**. The post-login freeze was
caused by reading byte4(=silent, always 1 on self) AS slide → permanent isSlidingSpline() → canAct()=false → frozen.
Decoder line 146 (`silent@4, slide@13`) is correct; do NOT swap them.

### ChatChannel enum — ✅ confirmed from capture (C2S op17 byte0 / S2C op91 byte4)
`0=Say, 1=Yell, 2=Whisper, 3=Guild, 4=Party, 5=System (roll result/"Success."), 6=Global/AllChat,
8=notice (durability/exp), 9=RedWarning ("You must be at least level 25…")`.
Corrects reconstructed ChatDefines (had Party=3, Guild=4, System=6). S2C op91 system/notice msgs use senderGuid=0.
