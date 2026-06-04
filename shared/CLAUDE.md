# shared/ — reconstructed wire protocol + game defines

Header-only library `emberfire::shared` (CMake INTERFACE target). Canonical headers in
`include/Emberfire/Shared/*.h`. Consumers reach them through the `client/Shared` junction so the headers'
internal `..\support.h` includes resolve to the support layer at `client/` (see `client/CLAUDE.md`).

## What's here
The framed-packet protocol (`GamePacket*`, `GamePackets_Stubs.h`), opcodes (`Opcode` enum in
`GamePacketServer.h`), and the game defines (`SpellDefines`, `ItemDefines`, `PlayerDefines`,
`UnitDefines`, `GameDefines`, `StlBuffer`, ...). All reconstructed by RE — treat values as ground truth
that took real work to nail; don't "simplify" them.

## Locked facts (tested in `shared/tests` — keep tests green)
- `spell_template.attributes` is a **64-bit** field — `enum Attributes : unsigned long long` (a 32-bit
  enum truncates `1ull<<35` to 0). Cursor/targeting bits: `TargetsGround=33`, `TargetsItem=35`,
  `MouseoverTargeting=38`.
- `Opcode::Server_SocketResult = 121` (NOT 147 — that's an arena op). `Client_UseItem=11`,
  `Server_EquipItem=86`.
- `GP_Client_UseItem` target slots default to **255** (0xff = "no target"); 0 means "inventory slot 0".
- `ItemDefinition::operator==` is **entry-only** (VendorNpc relies on it); use `equalsExact()` for full
  field comparison (gem/enchant/durability changes) — that's what makes live updates show.
- `PlayerDefines::WorldError` codes are binary-confirmed (e.g. 43 NoEmptySockets, 47 WrongClass, 50
  ItemIsSoulBound, 51/52 Arena/Dungeon). Full table came from the client `World::error` switch.

## Rule
Change a packet layout, opcode, or enum value → update the matching test in `tests/`. Verify against a
proxy capture or the `docs/` decompile, never intuition.
