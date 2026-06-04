# Ghidra extraction from Dreadmyst Server.exe (stripped, anchored via strings)
Project: build/deps/ghidra/proj/dmyst. Tooling: GhidraMCP. Binary is STRIPPED (FUN_ names) but some
class vftables are named (RTTI), which leaks packet-class names.

## Protocol core (CONFIRMED from server)
- **StlBuffer semantics**: `operator>>` PEEKS; the caller does `>> opcode` then `eraseFront(2)` to consume.
  (dispatch FUN_00489310: reads `*(ushort*)`, then `FUN_004b4d40(param_1, 2)` = eraseFront.)
  StlBuffer prim funcs: `FUN_00420330`=readString, `FUN_00411990`=readInt32, `FUN_004118b0`=readU16, `FUN_00411920`=readU8.
- **Opcode dispatch** = `FUN_00489310`, `switch(opcode - 1)`. So **opcode = switch_case + 1**. Bad if opcode-1 > 0x96 (≤151).
- **Min client build = 0x4a4 = 1188** (`GP_Client_Authenticate` handler rejects build < 1188 → Validate result 4). Client sends 1189 ✓.
- **GP_Client_Authenticate** (op2, FUN_0048a400) read order: string token, int build, string fingerprint ✓ (matches our reconstruction). Server MD5s the fingerprint (`FUN_00486e10`).
- **Server_Validate sender** = `FUN_00489070(this, resultCode)`. AuthenticateResult codes: version-fail=4, ban/serverfull=2 (the `0x3e7<x` path). (success path returns earlier; success code likely 0 per capture.)

## CLIENT opcode -> handler (opcode = case+1, from dispatch FUN_00489310). op3 is server-only (Validate).
1 FUN_0048a390 Mutual_Ping | 2 FUN_0048a400 Client_Authenticate | 4 FUN_0048ab10 CharCreate |
5 FUN_0048b500 EnterWorld | 6 (inline) CharacterList-req | 7 FUN_00432c40 Gossip/QuestGiver-interact (NOT DeleteChar — touches quest-giver lists + grant-quest FUN_00465e90) |
8 FUN_0048a030 | 9 FUN_004933b0 | 10 FUN_004931c0 | 11 FUN_004926b0 | 12 FUN_0048cfd0 |
13 (inline,2 bytes, FUN_00475600) ChangeChannels | 14 FUN_00493550 | 15 FUN_00493ba0 (RequestMove) |
16 FUN_004939f0 (RequestStop) | 17 FUN_00493f60 | 18 (inline) GP_Client_GuildMotd [vftable] |
19 FUN_0048a140 | 20 FUN_00494d00 | 21 FUN_00495610 | 22 FUN_00494f20 | 23 (inline) GP_Client_GuildCreate [vftable] |
24 FUN_00494e80 | 25 FUN_004952f0 | 26 FUN_00495380 | 27 FUN_004957d0 | 28 (inline sets +0x1c4) |
29 (inline,int) | 30 FUN_00493d20 | 31 FUN_00493670 | 32 (inline u16+int) | 33 FUN_0048a310 |
34 FUN_0047a2b0 | 35 FUN_00484ca0 | 36 FUN_00432400 | 37 FUN_00489fd0 | 38 FUN_00476340 | 39 FUN_00495650 |
40 FUN_00492a30 | 41 FUN_004929a0 | 42 (inline +0x1bd) | 43 (inline loot-all) | 44 FUN_0047c890 |
45 FUN_00492280 | 46 FUN_00491e70 | 47 FUN_004912c0 | 48 FUN_004918f0 | 49 FUN_00491970 | 50 FUN_00495410 |
51 FUN_00495530 | 52 FUN_004909b0 | 53 FUN_0048e0f0 | 54 FUN_0048d060 | 55 FUN_004a30e0 | 56 FUN_004a3320 |
57 FUN_0048dff0 | 58 (inline) | 59 FUN_0048d1f0 | 60 FUN_0048d2f0 | 61 FUN_0048d390 | 62 FUN_0048dee0 |
63 FUN_0048d900 | 64 FUN_0048d6b0 | 65 FUN_0048b2c0 | 66 FUN_0048d590 | 67 (inline) ScriptCmd_QueueArena[vftable] UpdateArenaStatus |
68 FUN_004910a0 | 69 FUN_00490e30 | 70 FUN_0048e220 | 71 FUN_0048e7e0 | 72 FUN_004902c0 | 73 FUN_0048f5f0 | 74 FUN_0048ecd0

=> Client opcodes are 1..74 sequential in enum/dispatch order (Mutual_Ping=1; op3=Server_Validate interleaved, server-only).
This replaces the placeholder Client_* values in GamePacketServer.h. Decompile any handler addr for its exact field layout.

Verified quest C2S layouts (NO giver guid on the wire — server knows the giver from the open GossipMenu session):
- op7 GossipQuestAccept (FUN_00432c40): `[questId:u32, giverGuid:u32]` — **the real quest accept** (VERIFIED vs steam conn1 18-58 → AcceptedQuest(112)). giver may be 0 (server uses open-gossip session).
- op8 (FUN_0048a030): reads 1 int, matches open quest-offer list. A DIFFERENT path — gossip flow never uses it; server ignored our op8 accepts. NOT the gossip accept.
- op33 CompleteQuest (FUN_0048a310 → FUN_00432e70): `[questId:s32, ignored:s32, itemChoiceIdx:s32]` — uses #1 + #3.
- op19 AbandonQuest (FUN_0048a140): `[questId:s32]` — one int.

Other verified C2S layouts (this session):
- op9 EquipItem (unpack FUN_00484d80): `[u8 invSlot, u8 flag, u16 itemEntry, u16 affix, u8×6 identity(score/gems/material), u8, u8]` = 14B. Echoes full item record so server verifies identity, then equips. proxy decEquipItem already matches.
- op35 BuyVendorItem (unpack FUN_00484ca0): `[u16 item, u16 affix, u8 score, u8 pad, u32 count]`. proxy already matches.
- op41 SpellRankInvest (FUN_004929a0): `[u16 spellId, u16 rank]`; spellId validated by FUN_0047a120 (allow-list 0x51/0x52/0x55/0x58/0x7f/0x80 + owned), rank∈1..6, stored in map player+0x244. (was "SpendExp?")
- op7 Gossip/QuestGiver-interact (FUN_00432c40): quest-giver/gossip click; complex, exact wire fields still TODO. Confirmed NOT DeleteChar.

## Server DB schema (from SQL string dump — backend logic, plaintext)
Tables incl: player, player_bank, player_aura, player_spell_cooldown, player_class_stats(class,level,hp,mana,
str,agi,will,int,courage = BASE STAT TABLE), npc_template (full stat cols), item_gems, material_chance_armor/weapon,
loot, gossip, guild, arena_template, map(id,name,start_x,start_y,start_o,los_vision), auth_tokens. See list_strings.

## StlBuffer primitive funcs (confirmed)
- write: u16=FUN_00401d70 (opcode writer), u32=FUN_00411840.
- read: string=FUN_00420330, int32=FUN_00411990, u16=FUN_004118b0, u8=FUN_00411920 / FUN_00401e50, eraseFront=FUN_004b4d40.
- sendPacket=FUN_004b8790. Generic build helper=FUN_00484150.
- **Server->client opcode harvest**: in each GP_Server_*::build the FIRST `FUN_00401d70(buf, N)` arg N = the opcode.
  Grep build/deps/ghidra/dump/decompiled.c for `FUN_00401d70(` near a `GP_Server_*::vftable` assignment.

## Bulk decompile
Full decompile of every function -> `build/deps/ghidra/dump/decompiled.c` (+ functions_index.txt, strings.txt),
via headless Java BulkDump.java on project copy proj2. Self-serve grep for any system.

## Next targets
- Decompile a few client handlers (CharCreate op4, RequestMove op15) to lock exact packet field types.
- Find Server->Client opcode values: each GP_Server_*::build writes a const opcode; find build funcs (xref vftables / the send path).
- crunchItemStats: anchor via player_class_stats + material_chance queries.
- Movement/pathing: op15 handler FUN_00493ba0 -> spline/path logic.
