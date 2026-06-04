# Dreadmyst Server Wire Protocol — mined from Ghidra decompile

Source: `build/deps/ghidra/dump/decompiled.c` (10.5 MB, every server FUN_ decompiled).
Binary is STRIPPED but RTTI leaks `GP_Server_*::vftable` / `GP_Client_*::vftable` class names.
All opcodes are written as the FIRST `FUN_00401d70(buf, N)` in each packet's serialize ("build") method.

## StlBuffer write primitives (confirmed)

| FUN_ | role | width |
|------|------|-------|
| `FUN_00401d70` | write u16 (also the **opcode writer**) | 2 |
| `FUN_00411840` | write u32 / s32 | 4 |
| `FUN_00401de0` | write u8 / bool | 1 |
| `FUN_00421850` | write u8 (byte; also string NUL terminator) | 1 |
| `FUN_00401eb0` | write raw bytes (string body, `(ptr,len)`) | len |
| `FUN_0045cbf0` | write float (reads XMM, 4 bytes) | 4 |
| `FUN_004b8790` | sendPacket(conn, buf) | — |
| `FUN_004b3650` / `FUN_004b37e0` | packet-build/broadcast helpers | — |
| `FUN_00484150` | generic 2-field build helper (used by Validate) | — |

**String encoding** = `FUN_00401eb0(buf, ptr, len)` (raw bytes, NO length prefix) immediately
followed by `FUN_00421850(buf, 0)` (NUL terminator). So strings are **NUL-terminated, not length-prefixed**.
(Std::string SSO: pointer at +off, len at +off+0x14, capacity at +off+0x18; if cap>0xf the pointer is indirected.)

Read primitives (for client->server): string=`FUN_00420330`, s32=`FUN_00411990`, u16=`FUN_004118b0`,
u8=`FUN_00411920`/`FUN_00401e50`, u64/2×u32=`FUN_004841f0`, eraseFront=`FUN_004b4d40`.

---

## SERVER -> CLIENT opcode table

Opcode (dec / hex) -> class : serialize FUN_ @addr. ✓ = matches capture (OPCODES.md).

| Op | Hex | Class | Serialize FUN_ | Notes |
|----|-----|-------|----------------|-------|
| 3 | 0x03 | GP_Server_Validate | FUN_00484150 @00484150 | ✓ auth result |
| 75 | 0x4b | GP_Server_CharacterList | FUN_00484270 @00484270 (also inline build @0x489… line130823) | ✓ |
| 76 | 0x4c | (controller-flag, inline @0x458…) | inline line 85519 | u8 |
| 77 | 0x4d | GP_Server_Player | FUN_0046c700 @0046c700 (opcode via base FUN_00461960, field +0x30=0x4d) | |
| 78 | 0x4e | GP_Server_Npc | FUN_00461960 @00461960 (base; opcode field +0x30=0x4e set @line101247) | ✓ |
| 79 | 0x4f | GP_Server_GameObject | FUN_00461960 (base; field +0x30=0x4f @line93959) | ✓ |
| 80 | 0x50 | GP_Server_SetController | inline @0x458… line 85541 | ✓ u32 guid |
| 81 | 0x51 | (world/status "busy", inline) | line 132356 / 132682 | u8 |
| 82 | 0x52 | GP_Server_ObjectVariable | FUN_004b3870 @004b3870 | ✓ |
| 84 | 0x54 | GP_Server_NotifyItemAdd | inline FUN @0x473… line 110131/110175 | |
| 85 | 0x55 | GP_Server_Inventory | FUN_0046d2a0 @0046d2a0 | |
| 87 | 0x57 | (Server_Validate-like reject, FUN_00489230) | line 131026 | u16 code |
| 88 | 0x58 | (string packet, FUN_0046c3d0) | @0046c3d0 | 1 string |
| 89 | 0x59 | (Server_Validate-like, FUN_00489150) | line 130978 | u16 code |
| 90 | 0x5a | GP_Server_UnitSpline | FUN_0046c0c0 @0046c0c0 | ✓ |
| 91 | 0x5b | GP_Server_ChatMsg | FUN_004203d0 @004203d0 | ✓ |
| 96 | 0x60 | GP_Server_SetSubname | inline @0x4b3… line 172328 (+ FUN line171113,172012,172151) | |
| 97 | 0x61 | (guild, inline) | line 52987 / 56162 | |
| 98 | 0x62 | (CombatMsg variant, floating dmg) | line 144240/144264 | |
| 99 | 0x63 | (CombatMsg variant) | line 144063 | u32+u16 |
| 100 | 0x64 | GP_Server_SpellGo | FUN_0046e0a0 @0046e0a0 | |
| 102 | 0x66 | GP_Server_CombatMsg (primary) | FUN_00411a00 @00411a00 | |
| 103 | 0x67 | (build @0046e5…) | line 105833 | |
| 104 | 0x68 | GP_Server_OpenLootWindow | FUN_0046e660 @0046e660 | |
| 105 | 0x69 | (inline) | line 124085 | |
| 106 | 0x6a | GP_Server_SpellGo (cast-fail/interrupt path) | inline line 113252 / 45214 | u8 |
| 107 | 0x6b | (inline, party/duel) | line 106978,108498,109704,109779,109832 | |
| 108 | 0x6c | GP_Server_UnitAuras | FUN_00467520 @00467520 | |
| 109 | 0x6d | GP_Server_Spellbook | FUN_0046dd70 @0046dd70 | |
| 110 | 0x6e | GP_Server_GossipMenu | FUN_00431380 @00431380 | |
| 111 | 0x6f | GP_Server_QuestList | FUN_0046d620 @0046d620 | |
| 112 | 0x70 | (inline) | line 110463 | |
| 113 | 0x71 | (AvailableWorldQuests-related, inline) | line 112436 | |
| 114 | 0x72 | (inline ack: u32+bool) | FUN_00476090 line 112905 | |
| 115 | 0x73 | (gossip-select ack, inline) | line 49209 | u32 |
| 117 | 0x75 | (inline) | line 131795 | |
| 118 | 0x76 | (inline) | line 110415 | |
| 119 | 0x77 | (inline) | line 113773 | |
| 120 | 0x78 | GP_Server_AvailableWorldQuests | FUN_0046d480 @0046d480 (op build) + build line 112595 | |
| 122 | 0x7a | GP_Server_Spellbook_Update | FUN_0046db10 @0046db10 | |
| 124 | 0x7c | (inline) | line 139245.. | |
| 125 | 0x7d | (inline; QuestList header build 0x7d local_44) | line 69664 / 112694 | |
| 126 | 0x7e | (inline) | line 44064 | |
| 129 | 0x81 | (inline) | line 44123 | |
| 130 | 0x82 | GP_Server_GuildMotd? (inline 0x82) | line 126744,138874,171305 | |
| 131 | 0x83 | (inline) | line 138826 | |
| 132 | 0x84 | (string packet, FUN_0046c410) | @0046c410 (+ line 104050,107764) | 1 string |
| 133 | 0x85 | GP_Server_PartyList | FUN @0x46… line 102534 | |
| 134 | 0x86 | (inline) | line 171305 | |
| 135 | 0x87 | GP_Server_MarkNpcsOnMap | FUN @0x45c… line 89099 | |
| 137 | 0x89 | GP_Server_TradeUpdate (inline) | line 154193 (vftable @156171) | |
| 138 | 0x8a | (inline) | line 155090 | |
| 139 | 0x8b | (guild build) | line 53019 | |
| 140 | 0x8c | (guild build) | line 53068 | |
| 141 | 0x8d | GP_Server_Bank | FUN_0046ce40 @0046ce40 | |
| 143 | 0x8f | GP_Server_QuestList-status / AvailableWorldQuests | FUN_0046c450 @0046c450 + build line 112712 | |
| 144 | 0x90 | GP_Server_UnitSpline (stop variant) | line 89444 | |
| 145 | 0x91 | (inline) | line 135092/135127 | |
| 146 | 0x92 | (inline) | line 3919 | |
| 147 | 0x93 | (inline, MarkNpcsOnMap rows) | line 89626.. | |
| 148 | 0x94 | (inline) | line 2083 | |
| 149 | 0x95 | (inline, several) | line 1531.. , 2384 | |
| 150 | 0x96 | GP_Server_(name string, FUN_0046c080) | @0046c080 (+ PkNotify build line 103851,114940,114974) | 1 string |
| 87/0x57, 89/0x59 | — | Validate-style reject senders | FUN_00489150/FUN_00489230 | u16 result |

Other named vftables seen (guild family, opcodes in 0x5d..0x61 / 0x8b..0x8c range):
GP_Server_GuildAddMember (0x5d? line54847), GP_Server_GuildRemoveMember, GP_Server_GuildOnlineStatus,
GP_Server_GuildRoster, GP_Server_GuildInvite (0x61 line56162), GP_Server_GuildNotifyRoleChange,
GP_Server_OfferDuel, GP_Server_OfferParty, GP_Server_PkNotify (0x96), GP_Server_TradeUpdate (0x89).

### Capture-table reconciliation
All capture-derived opcodes **confirmed correct**:
Validate=3 ✓, CharacterList=75 ✓, Npc=78 ✓, GameObject=79 ✓, SetController=80 ✓,
ObjectVariable=82 ✓, UnitSpline=90 ✓, ChatMsg=91 ✓. **No capture opcode was wrong.**
New/locked: Player=77, NotifyItemAdd=84, Inventory=85, SpellGo=100, CombatMsg=102,
OpenLootWindow=104, UnitAuras=108, Spellbook=109, GossipMenu=110, QuestList=111,
Spellbook_Update=122, AvailableWorldQuests=120, Bank=141, SetSubname=96.

There is **no `GP_Server_NewWorld` RTTI class** in the binary. The EnterWorld handler
(`FUN_0048b500` @0048b500) is a DB loader (queries player_equipment/player_bank, builds the
char) that then spawns the Player object (op 77) + Inventory (85)/Bank(141)/Spellbook(109)/
UnitAuras(108) packets. "NewWorld" in our reconstruction maps to the op-77 Player spawn, not a
dedicated packet; the only standalone enter-time packet is op 0x51 (world-busy/full status, u8).

---

## PACKET FIELD LAYOUTS (serialization order after opcode)

### Base Object serialize — `FUN_00461960` @00461960
Shared by Object/Npc/GameObject/Player. Opcode is read from `this+0x30` (per-subclass).
```
u16  opcode            (this+0x30)
u32  guid              (this+0x4)
f32  x                 (FUN_0045cbf0)
f32  y                 (FUN_0045cbf0)
u32  flags/typeMask    (this+0x18)
list (linked, this+0x14): repeat { u8 varIndex ; u32 value }   // object-variable inline list, NUL-implicit terminated by list end
```

### GP_Server_Npc (op 78) — build FUN @0x469…, serialize = base FUN_00461960 then:
```
<base object 78>
u32  <extra>           (local_4c = *(this+0x1ac)[0])   // e.g. npc template/display id
```
(Immediately followed on the wire by a separate UnitAuras packet (op108) for the same unit.)

### GP_Server_GameObject (op 79) — build FUN_004621a0 @004621a0
```
<base object 79>
u32  entry             (this+0x2c = goTemplate->entry)
```

### GP_Server_SetController (op 80) — inline @0x458…
```
u32  guid
```
(Preceded by op 0x4c {u8} controller-type packet to the same client.)

### GP_Server_ObjectVariable (op 82) — FUN_004b3870 @004b3870
```
u32  objectGuid        (this+0x4)
u8   varIndex          (param_2)
u32  value             (computed via vtbl+0x3c)
```

### GP_Server_NotifyItemAdd (op 84) — inline @0x473…
```
u32  itemGuid / playerGuid   (param_1+0x4)
```
(Broadcast variant adds no extra fields; a second op-84 with just the u32 is sent to self.)

### GP_Server_Inventory (op 85) — FUN_0046d2a0 @0046d2a0  (item stride = 0xe bytes)
### GP_Server_Bank (op 141) — FUN_0046ce40 @0046ce40  (IDENTICAL item layout, stride 0xe)
```
u8   count                  ((end-begin)/0xe)
repeat count {
  u16  slot                 (+0x0)
  u16  itemId/entry         (+0x2)
  u8   <a>  (+0x4)  u8 <b> (+0x5)
  u8   <c>  (+0x6)  u8 <d> (+0x7)
  u8   <e>  (+0x8)  u8 <f> (+0x9)
  u8   <g>  (+0xb)
  u8   <h>  (+0xa)  u8 <i> (+0xc)   // 7×u16 worth of bytes = gems/affix/count/durability
}
```

### GP_Server_OpenLootWindow (op 104) — FUN_0046e660 @0046e660
```
u32  lootObjectGuid    (this+0x4)
u32  <money or flags>  (this+0x8)
u32  count             ((+0x10 - +0xc)/0xe)
repeat count { item entry (0xe-byte item record, same shape as Inventory) }
```

### GP_Server_SpellGo (op 100) — FUN_0046e0a0 @0046e0a0  (VERIFIED vs capture)
```
u32  casterGuid        (this+0x4)
u32  spellId           (this+0x8)
u16  hitCount          (this+0x18)                       // length prefix for the hit list
repeat hitCount { u32 targetGuid (node+0x10) ; u32 value (node+0x14) }   // value = effect result/seed
f32  groundX           (this+0xc)
f32  groundY           (this+0x10)
```
conn5 sample: caster=6 spell=88 hitCount=1 hit={-577, 0x0156d918} ground={0,0} = 26B payload. NOTE: prior "targetGuid? @+0x8 / spellId @+0x18" was wrong — +0x8 is spellId, +0x18 is the hitCount. Matches client stub GP_Server_SpellGo::unpack.
(Cast-fail / interrupt path uses op 0x6a {u8} instead — line 113252.)

### GP_Server_CombatMsg (op 102) — FUN_00411a00 @00411a00
```
u32  attackerGuid      (this+0x4)
u32  targetGuid        (this+0x8)
u8   <flag>            (this+0xc)   (FUN_00401de0)
u16  amount            (this+0xe)
u16  <overkill/abs>    (this+0x10)
u8   school?           (this+0x12)  (FUN_00401de0)
u8   result            (this+0x14)
u8   <field>           (this+0x13)
u8   critical?         (this+0x15)
```
(Variants op 99 {u32 guid, u16} and op 0x62 are floating-combat-text / heal numbers built inline.)

### GP_Server_UnitAuras (op 108) — FUN_00467520 @00467520  (in-mem stride 0x20; WIRE record = 15 bytes)
VERIFIED wire write order (FUN_00411840=u32, FUN_00421850=u8):
```
u32  unitGuid          (this+0x4)
u8   buffCount         ((+0xc-+0x8)/0x20)
repeat buffCount {     // 15 bytes each
  u16  spellId         (rec+0x0)
  u32  maxDuration     (rec+0x8)   // client setDates() derives start/end from maxDuration & elapsedTime
  u32  elapsedTime     (rec+0xc)
  u8   stacks          (rec+0x2)
  u32  casterGuid      (rec+0x4)
}
u8   debuffCount       ((+0x18-+0x14)/0x20)
repeat debuffCount { same 15-byte record (list at this+0x14) }
```

### GP_Server_Spellbook (op 109) — FUN_0046dd70 @0046dd70  (in-mem stride 0x10; WIRE entries VARIABLE-length)
VERIFIED (validated vs conn5 capture: parses 7 spells incl effect pairs):
```
u8   count             ((end-begin)>>4)
repeat count {
  u16  spellId         (entry+0x0)
  u8   rank            (entry+0x2)
  u8   fxCount         ((entry+0x8 - entry+0x4)>>2)   // nested effect vector
  repeat fxCount { u16 min ; u16 max }                // same effect-pair shape as Spellbook_Update (op122)
}
```
NOTE: earlier "entry stride 0x10" referred to the in-memory record; the WIRE is variable (4-byte header + fxCount×4).
### GP_Server_Spellbook_Update (op 122) — FUN_0046db10 @0046db10
```
u16  spellId           (this+0x4)
u8   <rank/flag>       (this+0x6)
u8   count             ((+0xc-+0x8)>>2)
repeat count { u16 ; u16 }    // effect (min,max) pairs
```

### GP_Server_GossipMenu (op 110) — FUN_00431380 @00431380
```
u32  npcGuid           (this+0x4)
u32  textId/menuId     (this+0x8)
u32  countA            ((+0x10-+0xc)>>2)   ; repeat countA { u32 }   // quest ids (available)
u32  countB            ((+0x1c-+0x18)>>2)  ; repeat countB { u32 }   // quest ids (active/complete)
u32  countC            ((+0x28-+0x24)>>2)  ; repeat countC { u32 }   // gossip option ids
u32  countD            ((+0x34-+0x30)/0x14); repeat countD { item record, 0x14-byte stride (u16 itemId + ...) }  // vendor/trainer items
```

### GP_Server_QuestList (op 111) — FUN_0046d620 @0046d620  (quest stride 0x38)
```
u8   count             ((end-begin)/0x38)
repeat count {
  u32  questId         (+0x0)
  u8   state           (+0x4)
  4×   { u8 objCount ; repeat { u32 progress } }   // 4 objective sub-lists (FUN_0046d770, 8-byte pairs)
}
```

### GP_Server_AvailableWorldQuests build (op 143, line112712) — note distinct from FUN_0046c450
```
u32  <a>               (local_48)
u32  0x7d              (local_44, literal tag)
u32  count             ; repeat count { u32 questId }
```
(FUN_0046c450 @0046c450 is the op-0x8f generic "id list": u32,u32,u32 count + count×u32.)
(FUN_0046d480 @0046d480 op 0x78 = u8 count + count×u16 — a u16 id list.)

### GP_Server_UnitSpline (op 90) — FUN_0046c0c0 @0046c0c0  (waypoint stride 8)
```
u32  guid              (this+0x4)
u8   <moveFlags?>      (this+0x10)  (FUN_00401de0)
f32  startX            (FUN_0045cbf0)
f32  startY            (FUN_0045cbf0)
u8   <flag>            (this+0x11)
u16  pointCount        ((+0x18-+0x14)>>3)
repeat pointCount { u32 x ; u32 y }    // path waypoints (fixed-point or int coords)
```

### GP_Server_ChatMsg (op 91) — FUN_004203d0 @004203d0
```
u32  senderGuid        (this+0x4)
u8   chatType          (this+0x8)
str  senderName        (this+0xc, NUL-term)
str  message           (this+0x24, NUL-term)
u16  <a>               (this+0x3c)
u16  <b>               (this+0x3e)
u8   <c>(+0x40) u8 (+0x41) u8 (+0x42) u8 (+0x43) u8 (+0x44) u8 (+0x45)
bool <flag>            (this+0x47)  (FUN_00401de0)
u8   <field>           (this+0x46)
```

### GP_Server_Player (op 77) — FUN_0046c700 @0046c700
```
<base object 77>                      (FUN_00461960; opcode field +0x30 = 0x4d)
f32  <extra coord/orientation>        (FUN_0045cbf0)
str  name                             (this+0x3c, NUL-term)
str  <guild/title>                    (this+0x54, NUL-term)
u8   <a>(+0x38)  u8 <b>(+0x39)  u8 <c>(+0x3a)
u32  <flags>                          (this+0x74)
list (this+0x70): repeat equipment {
  u8   slot            (+0x8 within node)
  u16  itemId          (+0xa)
  u16  <enchant/affix> (+0xc)
  u8 (+0xe) u8 (+0xf) u8(+0x10) u8(+0x11) u8(+0x12) u8(+0x13)
  u8   <durability?>   (+0x15)
  u8   <field>         (+0x14)
}
```
(Player spawn is followed by a UnitAuras (op 108) packet for the player — line 109963/109972.)

### GP_Server_SetSubname (op 96) — inline @0x4b3… line 172328
```
u32  guid              (param_1[1])
str  subname           (NUL-term)
```

### GP_Server_Validate (op 3) — FUN_00484150 @00484150
```
u32  result            (this+0x4)
u64  serverTime        (this+0x8, this+0xc)   // _time64() at send (FUN_00489070)
```
AuthenticateResult codes: success=0, ban/server-full=2 (FUN_00489070 0x3e7< path), version-too-low=4
(min build 0x4a4=1188; client sends 1189). Reject helpers FUN_00489150 (op 0x59) / FUN_00489230
(op 0x57) send {u16 code} for in-world error toasts (e.g. 0x1d "can't do that").

### GP_Server_CharacterList (op 75) — FUN_00484270 @00484270  (char stride 0x24)
```
u32  count             ((end-begin)/0x24)
repeat count {
  str  name            (+0x10..0x18 std::string, NUL-term)
  u8   0               (terminator written explicitly)
  u8   <class/race>    (+0x18)
  u8   <level/gender>  (+0x19)
  u32  <appearance>    (+0x1c)
  ... (0x24-byte record)
}
```
(An equivalent inline builder at line 130823 in the EnterWorld/char path writes the same op 0x4b.)

---

## CLIENT -> SERVER (dispatch FUN_00489310, `switch(opcode-1)`, opcode = case+1)
Handler map already in GHIDRA_FINDINGS.md (ops 1..74). Key read layouts confirmed here:
- **op2 Client_Authenticate** FUN_0048a400: `string token, s32 build, string fingerprint` (server MD5s fingerprint, FUN_00486e10).
- **op4 Client_CharCreate** (read in FUN_0048b500 path line132382): `u8, u8, string name, u8`.
- **op7 Client_GossipQuestAccept** FUN_00432c40: `u32 questId, u32 giverGuid` — **the real quest accept** (VERIFIED vs steam conn1 18-58 → server replies AcceptedQuest(112)). giverGuid may be 0; server resolves giver from the open GossipMenu session.
- **op8** FUN_0048a030: `s32 questId` — ONE int, matches the open quest-offer list (`*(player+0x138)+0x2b0`, entries `[0xf]..[0x10]`, key `entry+4`). A DIFFERENT accept path; the gossip flow never sends it, so our op8 accepts were silently ignored. Use op7.
- **op33 Client_CompleteQuest** FUN_0048a310 → FUN_00432e70: reads 3 ints `[#1 questId, #2 ignored, #3 itemChoiceIdx]`; FUN_00432e70(mgr, #1, #3) matches #1 vs active-quest list (`player+0x24..0x28`), #3 → reward fn FUN_0047ab40. Builds op115 reply w/ questId. **No giver guid used.**
- **op19 Client_AbandonQuest** FUN_0048a140: `s32 questId` — ONE int; emits op117 reply.
- StlBuffer `operator>>` PEEKS; dispatch does `>> opcode` then `eraseFront(2)` (FUN_004b4d40).

## Caveats
- "inline" entries = opcode written directly in a logic function (not a dedicated `GP_*::serialize`);
  class name is then inferred from nearby vftable assignment or context, lower confidence.
- Field *names* are inferred from struct offsets/usage; types (u8/u16/u32/f32/string) are exact
  (taken from the write-primitive called). Byte-level offsets within list records are noted where
  decompilation made them unambiguous.
- Duplicate opcode appearances (e.g. 0x96, 0x8f, 0x51, 0x6b) reflect one serialize + several inline
  call sites or sibling sub-packets; the dedicated `GP_*::serialize` FUN_ is the authoritative layout.
