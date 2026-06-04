# Client source changes vs pristine baseline

Diff of the working client tree `Dreadmyst/DreadmystSource_Client` against the read-only checkout
`DreadmystSource_Client_readonly`. Raw unified patch: `CLIENT_CHANGES.diff` (this folder).

**Policy:** the client source is now READ-ONLY. The edits below are the accumulated rebuild patch set.
Going forward, prefer the `Shared/` layer (`build/src/Shared`, reached via the junction) or keep changes
as a tracked patch over the baseline. The items tagged **[DEBUG]/[JUNK]** should be dropped.

24 files changed, 334 insertions(+), 57 deletions(-), + 1 stray `ziptest.obj`.

## 1. Local-server / config plumbing (the original "run local" goal)
- **Connector.cpp** — auth connect uses `[Auth] Port` (default 80) + `[Auth] Secure` (TLS only if set); was hardcoded HTTPS:443.
- **Login.cpp** — auth host from `[Auth] Host` (was `L"dreadmyst.com"`). Also: login speed (see §7) + dev AutoLogin.
- **Login.h** — `m_autoLoginDone` flag (dev AutoLogin).
- **CharacterSelection.cpp/.h** — dev `[Debug] AutoLogin` auto-enter first char; `m_autoEntered` flag.

## 2. Wire-protocol / dispatch correctness
- **Game.cpp** — *the* dispatch fix: removed the double-consume `eraseFront(sizeof(opcode))` (reconstructed `>>` already consumes) that misaligned every handler. NewWorld(op77) stashes pos/vars/equipment into `s_pendingSelf`; map id taken from op76 (`CharaCreateResult` repurposed) since op77 has none. Inventory(op85) places icons by `slot.slot` (was sequential `i`) + null guard.
- **Inventory.cpp** — SellItem = op12 with `getItemDef()` (was `getEntry()`); SplitItemStack guarded behind `validOpcode` (real opcode unknown).
- **BuffDebuffRenderer.cpp** — CancelBuff guarded behind `validOpcode` (no client serializer in r1189; sending dropped the connection).
- **UnitFrame.cpp** — ResetDungeons guarded behind `validOpcode` (op38 was actually RequestRespawn).
- **World.cpp** — `error()` toast color/code corrections + `NotEnoughLevelupPoints` case; `queryWaypoints` re-enabled (verified); WorldError ordinals.
- **World.h** — `PendingSelf` struct + `s_pendingSelf`.

## 3. Controlled-player spawn bridge (NewWorld has pos but no guid; SetController has guid)
- **World.cpp `setController`** — spawns `ClientPlayer` from `s_pendingSelf` (class/gender/portrait/name + pos + vars + equipment) when the object isn't already present; `s_pendingSelf` definition.
- **World.h** — `PendingSelf`/`PendingEquip` structs (cross-cuts CharacterSelection + Game NewWorld stash above).

## 4. Stat / unit display
- **ClientUnit.cpp** — `MoveSpeedPct` divided by 100 (percent → 1.0-based multiplier).
- **ClientUnit.h** — `getUnitVariable` override forwarding to the object var map (fixed Mana/stat reads returning 0).
- **Equipment.cpp/.h** — opaque dark backing behind the stat sheet; Health row DISPLAYS MaxHealth(0x03) but INVESTS Stat::Health(0x0e) via new `spendVar` param on `attachValue`; value font 14→16; Progression/Experience/LevelupCost rows get no spend buttons; `variableVaueColor` colors the MaxHealth row red.

## 5. Faction / NPC
- **ClientNpc.cpp** — faction read from `npc_template` in `setEntry` (was default 0 → everything looked hostile); removed the guessed var-0x07→faction override that turned enemies green mid-combat.

## 6. Rendering / animation / world
- **ModelScript.h** — per-direction frame lookup rotated `(dir+2)%8` (sprites lagged heading by 90°).
- **Toolbar.cpp** — target-range uses the player's CURRENT position, not the stale (never-echoed) server position.
- **ZoneNotifcation.cpp** — zone derived via `worldPosToTerrainId` (row-major) instead of the imprecise screen-space iso inverse.
- **ClientMap.cpp/.h** — cell-index bounds guards (`>=0`, grid-extent ASSERT) for the sparse cell map; **[DEBUG]** F9 collider overlay (water/wall flag visualizer).

## 7. THIS SESSION (2026-06-03)
- **Application.cpp** — `spawnTimedPopup` skips overlays ≥60s under `DMYST_NO_LOADING_POPUP` (kills 300s "Loading" overlay that gated the UI). *(CMake define lives in `build/client/CMakeLists.txt`, not the client tree.)*
- **Login.cpp** — hard `Sleep(2000)` ("grace period") → `Sleep([Debug] ServerGraceMs)`, default 0 = no artificial login wait.
- **AcceptQuest/CompleteQuest wire layout** (`GamePackets_Stubs.h`) — fixed the "cannot accept quests" bug. The reconstructed layout prepended a `questGiverGuid` field; the server reads NO giver guid on the wire (it knows the giver from the open GossipMenu session). Verified vs server:
  - **AcceptQuest** — CORRECTED: the gossip accept is **op7 GossipQuestAccept** (FUN_00432c40), wire `[questId:u32, giverGuid:u32]`, server replies AcceptedQuest(112). VERIFIED vs steam client (capture conn1 18-58). The earlier op8 theory was wrong: op8 (FUN_0048a030) is a different path the gossip flow never sends, so the client's op8 accepts were silently ignored ("still cannot accept quest"). `GP_Client_AcceptQuest` now builds op7 with questId+giverGuid.
  - **CompleteQuest (op33, FUN_0048a310 → FUN_00432e70)** reads 3 ints: `#1=questId` (match key), `#2=ignored`, `#3=itemChoiceIdx`. Was `[giverGuid, questId, choiceIdx]` → questId slot got giverGuid → dropped. Now `[questId, (giverGuid parked/ignored), itemChoiceIdx]`.
  - **AbandonQuest (op19, FUN_0048a140)** already correct: single int `questId`.
  - The earlier `setQuestGiverGuid(getGossipGuid())` change in `DialogNpc.cpp` did NOT fix it (the field shouldn't exist on the wire); the real fix is the wire order/count above. The setter call is kept only to populate the UI panel.

## 8. Debug instrumentation / junk still in the tree — REMOVE for read-only
- **[DEBUG] Game.cpp** — packet-capture harness writing `pktlog.txt` (the `kWatch` opcode set, raw hex dump before unpack).
- **[DEBUG] LootWindow.cpp** — loot-click dump to `maploaddebug.txt` (+ a trailing-newline/whitespace reflow).
- **[DEBUG] ClientMap.cpp/.h** — F9 collider overlay (keep if useful for map work, else drop).
- **[JUNK] ziptest.obj** — stray MSVC build artifact committed into the source dir; delete.
- Cosmetic-only: LootWindow.cpp whitespace + EOF newline; assorted blank-line tab trims in Game.cpp.

## Notes
- Most §2–§6 items are genuine behavioral fixes living in client `.cpp` logic — they can't move to `Shared/`.
  Realistic options: (a) treat the working tree as the canonical client fork and keep the baseline as
  reference only, or (b) maintain `CLIENT_CHANGES.diff` as the applied patch over the read-only baseline.
- The faithful levelup work this session lives in `Shared/` (`PlayerDefines.h`, `MutualUnit.h`), NOT the
  client — those are unaffected by the read-only rule.
