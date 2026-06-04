# Dreadmyst Server — WORLD / MOVEMENT / COMBAT logic (mined from Ghidra dump)

Source: `build/deps/ghidra/dump/decompiled.c` (stripped, FUN_ names; RTTI class names + plaintext SQL
retained). Cross-ref: `SERVER_BACKEND.md`, `PROTOCOL.md`. All addresses are file/VA addresses.
Confidence is flagged per section. "→" = "calls/dispatches to". Offsets are byte offsets into the
`this` object unless noted.

---

## 0. Object-model offsets used throughout (ServerUnit / MutualUnit)

Recovered from the move + combat code. ServerUnit `this`:
- `+0x08` = login/AI state-ish / object type tag (==3 means "alive unit, can act"; checked everywhere).
- `+0x68` / `+0x6c` = **current world X / Y** (float). (Read by spline build & ChaseMovement.)
- `+0x88` = **UnitSpline follower object** (the spline-interpolator, see §1.3).
- `+0xa0` / `+0xa4` = **spline point vector** begin / end pointers (each point = 8 bytes = `x:float,y:float`).
  `+0xa0==+0xa4` ⇒ no spline (idle). "Clear spline" = `*(+0xa4) = *(+0xa0)`.
- `+0x100` = **UnitMotionGen** (the motion controller; holds the gen-stack at its `+8`/`+0xc`).
- `+0x138` (on Session) = ServerPlayer pointer.
- `+0x148` = per-mechanic counter array (Fear/Root/etc; indexed by mechanic id, see §1.5).
- `+0x3c`/`+0x40` = **stat vector** begin/end (each stat = 4 bytes int; index by `UnitDefines::Stat`).
  Stat lookup helper is vtable slot `+0x88`: `getStat(idx)` = `(**(this+0)+0x88))(idx)`.
  `stat[2]` (`+0x3c[2]` = byte off 8) is used as the **skill-cap / level divisor** in combat (see §3).
- `+0x50` = template/definition pointer (NpcTemplate/SpellTemplate context). `+0x50 + 0x70` = a
  **flags dword** (immunities & combat-modifier bits; see §3 bit table).

---

## 1. MOVEMENT / PATHING / SPLINE

### 1.1 RequestMove (op15) handler — `FUN_00493ba0`
Reads body `{ x:float, y:float, wasd:u8 }` (StlBuffer peeks then advances `*param_1`):
```
if (session.state != 2 /*in-world*/) return;
if (!player.isAlive()) return;                         // vtbl+0x74
x = readFloat(); y = readFloat();                       // iStack_14, uStack_10
wasdByte = readU8();                                    // FUN_00401e50
if (wasdByte) player.motion[+1] = 1;                    // mark "keyboard/WASD move"
FUN_004a6500(player.motionGen /*+0x100*/, {x,y}, zDefault=0x493c40, 0);  // set PointMovement dest
if (player.spline_begin == player.spline_end) FUN_00473c50(player, 0);   // not moving yet → start
else if (wasdByte)                              FUN_00473c50(player, 1);  // already moving → flag=1
```
So a client move = "set a PointMovement destination, then emit a UnitSpline". `RequestStop` (op16)
clears the spline (`+0xa4 = +0xa0`) and installs IdleMovement.

### 1.2 The motion-generator system (UnitMotionGen) — gen factory + stack
`UnitMotionGen` (unit `+0x100`) owns a **priority-slotted vector of movement generators** at its
`+8`(begin)/`+0xc`(end); each slot = 8 bytes `{genPtr, refcountPtr}`. Slot index == the generator's
**type id**. Each gen object: `[0]`=Ref_count vftable, `[1][2]`=refcounts, `[3]`=gen vftable,
`[4]`=typeId, `[5]`=owner ServerUnit, then type-specific fields.

Generator **type ids** (the slot index, from the factory funcs):
| id | class | factory FUN | extra fields |
|----|-------|-------------|--------------|
| 1 | IdleMovement | FUN_004a6100 (inline at 157666) | — |
| 2 | RandomMovement | FUN_004a5bf0 path | `[6]`=wanderRadius, `[7..9]`=state |
| 3 | PatrolMovement | FUN_004a5bf0 path | `[6]`=waypointList, `[7]`=index/dir |
| 5 | ChaseMovement | FUN_004a57b0..157822 | `[0xb][0xc]`=last target x/y |
| 7 | **PointMovement** | **FUN_004a6500** | `[9][10]`=**dest x/y** (`[7]`=z) |
| 9 | FearMovement | FUN_00481410 case 3 | `[6][7]`=flee-from x/y |
| 10 | SlideMovement | FUN_004a6400 | `[6][7]`=slide x/y |
(ChargeMovement, HomeMovement, ActionMovement, PatrolMovement share the same construction pattern.)

- **Install a gen**: `FUN_004a5e40(motionGen, gen+3, gen)` — drops it into `vec[typeId]`, refcounted.
- **Pick active gen**: `FUN_004a5f00(motionGen)` returns the **highest occupied slot index** (highest
  priority). The per-tick driver `FUN_004a5aa0(motionGen)`:
  ```
  FUN_004b4670(owner.spline /*+0x88*/);     // step the spline (advance + face), §1.3
  active = FUN_004a5f00(motionGen);
  gen = vec[active];
  done = gen->update();                      // gen vtable slot +8  (virtual)
  if (done) { pop gen; if (now empty) install IdleMovement; }
  ```
  *(High confidence on structure; the per-gen `update()` virtual body wasn't isolated by name, but
  for PointMovement it just (re)builds a spline to `[9][10]` and reports done when arrived.)*

### 1.3 Spline build + step
**`FUN_004a6500` = set PointMovement destination.** Allocates a PointMovement (size 0x2c), stores
`dest.x → [9]`, `dest.y → [10]`, `z → [7]`, typeId 7, then `FUN_004a5e40` installs it. It does NOT
itself compute a path — it sets the goal; the gen update + spline build produce the point list.

**`FUN_004a2e00(splineVec, &endpointA, &endpointB)`** = the actual **path/spline point inserter**
(a `std::map`-of-nodes builder; uses `FUN_004a52f0` to push 8-byte `{x,y}` points and `FUN_004a5000`
to insert graph nodes keyed by an int "cost/order" at node `+0x10`). It produces the ordered list of
`{x:float,y:float}` waypoints that becomes `+0xa0..+0xa4`.
> NOTE / UNCERTAIN: the recovered code is the STL-rebuilt node/tree plumbing of the pathing graph,
> not a clean A*. From the structure (two endpoints + an ordered node map keyed by an int at +0x10),
> the server builds a **straight point-to-point spline** for player moves (single segment dest), and
> uses the node map only when intermediate waypoints exist (NPC patrol / obstacle insertion). No
> per-cell A* expansion loop was isolated — for the local reimpl, a straight-line spline to the
> requested dest reproduces observed behaviour (capture: `move 70 92` → spline straight to (70,92)).

**`FUN_004b4670(splineFollower /*unit+0x88*/)` = per-frame spline step.** Fields: `[5]`=current
point ptr, `[6]`=current-iter, `[7]`=end-iter, `[3]`=facing(orient float), `[4]`=last clock.
```
if (cur != end) {
  dt = clock() - lastClock;
  if (dt != 0) {
    lastClock = clock();
    // recompute facing from the segment delta:
    dy = next.y - cur.y;  (and dx via FUN_004b4780 = atan2)
    facing = atan2(dy, dx);  // _CIatan2; wrapped into [0, 2π) via +DAT_00592fe0 if < _DAT_00592e3c
    if (facing changed) notifyOrientation();           // vtbl+8
    FUN_004b56c0(cur, &curIter);                        // advance position toward next point
  }
  return (cur == end);                                  // true when arrived
}
```
So orientation is derived server-side from the spline segment via `atan2`.

### 1.4 GP_Server_UnitSpline serializer (opcode 0x5a = 90) — `FUN_0046c0c0`
Builds the 24-byte+ position/spline packet observed in capture (op90):
```
writeU16(0x5a);                       // opcode 90
writeU32(unit.guid);                  // +4
writeU8 (flagA);                      // +0x10  (walk/teleport mode)
writeFloat(unit.x); writeFloat(unit.y);   // FUN_0045cbf0 x2  (current pos, +0x68/+0x6c via builder)
writeU8 (flagB);                      // +0x11  (e.g. "instant/forced")
writeU16(count = (splineEnd-splineBegin)/8);   // number of spline points
for each point: writeU32(x_bits); writeU32(y_bits);   // float x, float y
```
Builder/sender path: `FUN_00473c50(unit, startedFlag)` assembles a `GP_Server_UnitSpline`
(`vftable`), copies `+0xa0..+0xa4` via `FUN_0047c7b0`, then `FUN_0046c0c0` serializes and
`FUN_004b8790` sends (only if `unit+0x2b4` session ptr != null). `FUN_0045cbf0` = `StlBuffer<<float`.
Deserializer (client side / harness): `FUN_0046c210`.

### 1.5 Movement-affecting mechanics — `FUN_00481410`
A switch on a **mechanic id** (3 = Fear → FearMovement, etc.) gated by the per-mechanic counter array
at `unit+0x148[id]`. On entering a forced-move mechanic it clears the current spline
(`+0xa4=+0xa0`, step once, then `splineFollower->reset()` at `+0x88` vtbl+4) and installs the
override gen (Fear=9). NPC default movement selection is `FUN_004a5bf0`: NPC `move_type` 1 ⇒
Random/Idle (with wander radius, default 5), 2 ⇒ Patrol from its waypoint list.

---

## 2. COORDINATE / CELL MATH (GameMap / ServerMap / MapCellT)

### 2.1 Grid shape — `FUN_004634a0` (GameMap/ServerMap ctor) + resize `FUN_004b5e?0` (@~173900)
- GameMap default: cell vector pre-reserved to **0x9c4 = 2500** entries; width field `+0x10 = 0x32 = 50`
  ⇒ **default map = 50×50 cells**. `+0x28 = 0xf` (small-buffer marker).
- Resize (`@173900`) sizes the cell vector to **`width * width`** (square grid). Each **MapCellT = 0x24
  (36) bytes** (9 dwords), `MapCellT::vftable` at cell `+0x10`. A parallel 0x14(20)-byte vector holds
  per-cell render/texture data. So: **flat cell index = `y * width + x`**, cell stride 36 bytes.
- Cells loaded by `GameMap::loadFromDisk` (`@~174800`, asserts cite
  `C:\…\Shared\GameMap.cpp`): each cell record = `[flags:u8][textureIdx...]`, asserts
  `flags > 0 || textures.size() > 0` and `textureIdx < knownCellTextures.size()`.

### 2.2 World-pos ↔ cell
- `cellIndex(x,y) = floor(y) * width + floor(x)` (one cell == one world unit; positions in capture are
  small floats like 62.7, 92.5 on a 50-ish/100-ish grid → 1:1 world-to-cell, **terrain-to-cell ratio = 1**).
  > UNCERTAIN: a sub-cell terrain ratio (e.g. multiple terrain samples per cell) was not found in the
  > server; the iso/screen projection (`GameMap::computeRawScreenPosition`) is **client-only** and is
  > NOT present in the server binary — the server works purely in world (x,y) float space. For the
  > reimpl, treat positions as continuous world floats; cell = `(int)floor`.
- MapCellT flags live in the cell's flag dword; walkability/`Unwalkable`/`CollideBlock` are bits in
  that field. The pathing node insert (`FUN_004a2e00`) keys nodes by an int at node `+0x10`
  (cost/blocked marker). *(Exact bit values not isolated — flag byte is per-cell from the map file.)*

### 2.3 `map` table columns (SQL dump): `id, name, start_x, start_y, start_o, los_vision`
`los_vision` = the LOS / vision radius used for visibility & aggro range.

---

## 3. COMBAT DAMAGE + HIT RESOLUTION

The spell/attack pipeline: an `Effect_*` object's `apply(caster_ctx, target)` computes a number,
applies it via target vtable, and sends `GP_Server_CombatMsg`. Effect factory = `FUN_0042e3?0`-area
switch (`param_2` = effect type id): **1 = SchoolDamage**, plus WeaponDamage / MeleeAtk / RangedAtk
(vftable assigns at decompiled.c lines 146890 / 147476 / 148124 / 148178).

### 3.1 Shared math primitives
- `FUN_0056b750(lo,hi)` = **int64 → float** (vcvtqq2ps) → result in XMM0. Effect stores its two
  numeric params as int64: at `this+0x18` and `this+0x10`.
- `FUN_004a0e40()` = **uniform RNG roll in [0,1)** (random_device-seeded; returns double in XMM0).
  Used as `if (roll() < chance) …`.
- `FUN_004a19d0(&out, intDelta)` = **skill-delta → chance lookup table** (maps integer
  `attackerSkill - defenderSkill` deltas to probability floats; e.g. delta ≤ −0x27(−39) ⇒ 0x3d4ccccd
  = **0.05f**, larger deltas → higher). This is the level/skill-difference curve for miss/dodge/etc.
- Float constants (read-only `.rdata`, exact values not in code but inferable):
  `DAT_0059304c` ≈ **100.0** (percent divisor), `DAT_00592ed4` ≈ **0.0/min**, `DAT_00592eb0` ≈ base
  chance offset, `DAT_00592fd8` ≈ skill normaliser, `DAT_00592f08` = **crit multiplier**, applied as
  `dmg = dmg * DAT_00592f08` on a crit (HitResult 7).

### 3.2 Effect_SchoolDamage::apply — `FUN_0042cfc0` (spell/magic damage)
```
base   = (int64→float)(effect.value@+0x18) / 100.0 + 0.0;     // effect.value as %
coeff  = (int64→float)(effect.coeff@+0x10);                    // scaling coefficient
dmg    = (base * caster.field@(spellCtx+0xbc) * target.modifier@param2[0x30]) / coeff;
if (dmg < 0) dmg = 0;
damage = (short)dmg;
applyThreat/onDamage(target, damage);                          // FUN_00480720
dealDamage(target, ...);                                       // target vtbl, sends CombatMsg
send GP_Server_CombatMsg{ caster, target, school, damage, … }
```
School damage **does not roll miss/dodge** (auto-hit) but does scale by a caster power field and a
target modifier (resistance/`ModifyDamageRecvPct` aura). *(Resistance is applied as the `param2[0x30]`
target multiplier and/or the school-immunity bit, see §3.5.)*

### 3.3 Effect_WeaponDamage::apply — `FUN_0042e0e0` (melee), variant `FUN_0042e320` (ranged, type 10)
```
hit = FUN_004a1640(caster_ctx, target, weaponSkillId);   // HitResult (see §3.4)
switch (hit) {
  case 1: case 3: case 4: case 8:                         // Miss / Parry / Dodge / Immune
      damage = 0; break;                                  // no damage, just send CombatMsg
  default:
      weaponVal = (int64→float)(effect.value@+0x18) / 100.0 + 0.0;
      coeff     = (int64→float)(effect.coeff@+0x10);
      damage    = weaponVal / coeff;
      if (damage < 1) damage = 1;                         // floor at 1
      if (hit == 7) damage *= DAT_00592f08;               // CRIT multiplier
      else if (hit == 2) damage /= 2;                     // GLANCING = half
      damage = (short)damage;
      if (damage>0 && target has armor stat[1]>0)
          dealDamage(target, **caster, damage, 1);        // target vtbl +0x98
          FUN_00482750(target, -damage);                  // subtract HP
}
onParry/riposte hook (target vtbl +0x5c);
send GP_Server_CombatMsg{ casterGuid, targetGuid, hitResult=hit, amount=damage, school, isPlayer };
```
Ranged (`FUN_0042e320`, type 10) is identical but uses `FUN_004a0fb0` for hit and pulls
**armor mitigation** explicitly: `mitig = targetArmor@(+0x3c node +8 i.e. stat[2]) …` clamped — see §3.4.
Auto-attack (no special effect) ranged uses `FUN_0042d1d0`, base ranged-weapon value from
`target.stat[…]`.

### 3.4 HitResult resolution — `FUN_004a1640` (melee) / `FUN_004a0fb0` (general, w/ armor)
Returns a `HitResult` enum. Each outcome is an **independent RNG roll vs a stat-derived chance, tried
in priority order**; first one that passes wins:

| HitResult | meaning | chance source (target stat off / caster) |
|-----------|---------|------------------------------------------|
| 8 | **Immune** | caster flags `(spellCtx+0x50)+0x70 & 0x40`, or target absorb list |
| 1 | **Miss** | `casterHitSkill@+0x88(skillId)` vs `target.stat` via `FUN_004a19d0` delta curve |
| 2 | **Glancing** (melee 1640) | target stat at MutualObject `+0x8c` / skill-cap |
| 4 | **Dodge** | target dodge `+0x90` (or `+0x74`) / cap |
| 5 | **Parry** | target parry `+0x94` / cap |
| 6 | **Block** | target block `+0x98` / cap |
| 7 | **Crit** | caster crit `+0x7c` (or `+0x78` for type 0xc) / cap; suppressed if flags `& 2` |
| 0 | **Hit** (normal) | fallthrough |

The "cap" divisor is `caster/target stat[2]` (the level-scaled skill cap; `(+0x3c)[2]` = byte off 8).
Each chance = `rawStat / cap`, compared against `FUN_004a0e40()` roll. Flags dword
`(unit/spell+0x50)+0x70` gates results:
`&0x40`=immune, `&2`=no-crit, `&0x4000`=can't-dodge, `&0x8000`=can't-glance, `&0x10000`=can't-miss,
`&0x20000`=can't-block, `&0x1000000`=guaranteed-crit/extra (set in `FUN_00429cd0`). The chosen result
is also pushed to a list at `caster_ctx+0xa4` and broadcast to attacker (`vtbl+0x50`) and target
(`vtbl+0x54`).

`FUN_004a0fb0` (ranged/general) additionally computes **armor mitigation**:
`mitigated = baseDmg * (targetArmorRating / casterPower)`-style, combining `target.stat@+0x80/+0x84`
(armor/resist) over `caster.stat[2]` (skill cap), and **resistances** subtract per-school via the same
divide-by-cap pattern before the crit/glancing roll.

### 3.5 GP_Server_CombatMsg (sent for every swing/spell hit)
Built in ~17 places (e.g. `FUN_0042cfc0`, `FUN_0042e0e0`, `FUN_0042d1d0`); layout (from the locals
populated before `FUN_00411a00`→`FUN_004b3650`→send):
`{ casterGuid:u32, targetGuid/objId:u32, hitResult:u8, amount:i16, school/type:u8, casterIsPlayer:u8,
   flags... }`. (`local_1a`/`local_50` = amount, `local_4c`/`local_1c` = hitResult, `local_54`/`local_24`
   = caster guid, `local_48`/`local_18` = target.) Exact field order to be locked vs a live CombatMsg
hexdump, but the semantic fields above are confirmed.

---

## 4. AURA TICKING (periodic damage / heal / restore-mana)

### 4.1 AuraEffect struct = `player_aura` per-effect block (3 slots/aura)
SQL (`INSERT INTO player_aura …`, string @0x00586ab8) gives the per-effect columns, mapping 1:1 to the
runtime AuraEffect struct (constructed `FUN_0049ea40` for PeriodicDamage, `FUN_0049eb60` for others):

| AuraEffect offset | column | type | meaning |
|---|---|---|---|
| `+0x04` | `m_tickTotal_N` | float | total amount over full duration |
| `+0x08` | `m_tickAmount_N` | float | amount applied **per tick** |
| `+0x0c` | `m_tickAmountTicked_N` | float | running sum already applied |
| `(caster)` | `m_casterGuid_N` | int | source unit guid (struct `[10]`=caster ptr) |
| `+0x14` | `m_numTicks_N` | int | total number of ticks |
| `+0x18` | `m_numTicksCounter_N` | int | ticks done so far |
| `+0x1c` | `m_tickTimer_N` | int | ms accumulator toward next tick |
| `+0x20` | `m_tickIntervalMs_N` | int | ms between ticks |

Header columns: `guid, caster_guid, spell_id, miliseconds(=remaining duration), m_stackCount,
m_spellLevel, m_casterLevel`, then 3× the block above (`_1`/`_2`/`_3`).

Construction (`FUN_0049ea40`): `tickIntervalMs = spellTemplate@(+0x4c)`; **clamped to ≥ 1000 ms if the
template value < 500** (so the minimum effective tick interval is 1000 ms). `[9]` (=`+0x24` region) is
seeded from the incoming amount param; caster stored at `[10]`, target at `[0xb]`.

### 4.2 The tick computation — `FUN_00410df0(auraEffect, &outAmountThisFrame)`
Called every server frame; returns how much to apply this frame (0 if no tick boundary crossed):
```
out = 0;
frameDt = currentFrame.deltaMs;                  // FUN_004011d0()->+0x10
tickTimer += frameDt;
while (tickIntervalMs != 0 && tickTimer >= tickIntervalMs) {
    numTicksCounter += 1;
    tickTimer       -= tickIntervalMs;           // carry remainder
    out             += tickAmount;               // this tick's amount
    tickAmountTicked += tickAmount;
    if (numTicksCounter >= numTicks) {           // FINAL tick: flush rounding remainder
        rem = tickTotal - tickAmountTicked;
        out              += rem;
        tickAmountTicked += rem;
    }
}
*outAmount = out;
return (out != 0);                               // "did we tick this frame"
```
The returned `out` is then applied by the concrete subclass:
- **Aura_PeriodicDamage** (`Aura_PeriodicDamage::vftable`, ctor `FUN_0049ea40`) → deal `out` damage to
  target (no hit roll; school resist may still scale via ModifyDamageRecvPct auras).
- **Aura_PeriodicHeal** (`FUN_0049eb60`, vftable @150357) → restore `out` HP.
- **Aura_PeriodicRestoreMana** (vftable @151673) → restore `out` mana.

So a DoT/HoT/regen aura is fully described by `{ tickAmount, tickIntervalMs(≥1000), numTicks,
tickTotal }`, with `tickTotal = tickAmount * numTicks` and the remainder dumped on the last tick to
correct float rounding. Aura expiry = `miliseconds` (remaining duration) hits 0 (separate from the
tick loop). DELETE on removal: `DELETE FROM player_aura WHERE guid = '%u'` (@0x00586eb8).

---

## 5. KEY FUN_ ADDRESS INDEX
| addr | role |
|------|------|
| FUN_00493ba0 | Client RequestMove (op15) handler |
| FUN_004939f0 | Client RequestStop (op16) handler (clears spline) |
| FUN_004a6500 | set PointMovement destination (type 7) |
| FUN_004a5e40 | install a movement generator into the gen-stack |
| FUN_004a5f00 | pick active (highest-priority) gen slot |
| FUN_004a5aa0 | motion-gen per-tick driver (step spline + run gen update) |
| FUN_004a2e00 | spline/path point-list builder (node map → `+0xa0` points) |
| FUN_004b4670 | per-frame spline step (advance + atan2 facing) |
| FUN_00473c50 | build+send GP_Server_UnitSpline |
| FUN_0046c0c0 | GP_Server_UnitSpline serialize (opcode 90/0x5a) |
| FUN_0046c210 | GP_Server_UnitSpline deserialize |
| FUN_0045cbf0 | StlBuffer << float (writes spline/pos coords) |
| FUN_00481410 | mechanic-driven movement override (Fear etc.) |
| FUN_004a5bf0 | NPC default movement selector (Random/Patrol) |
| FUN_004634a0 | GameMap/ServerMap constructor (50×50 default grid) |
| (~173900) | GameMap cell-vector resize (width², MapCellT=36B) |
| FUN_0042cfc0 | Effect_SchoolDamage::apply (magic damage) |
| FUN_0042e0e0 | Effect_WeaponDamage::apply (melee) |
| FUN_0042e320 | weapon/ranged damage variant (type 10) |
| FUN_0042d1d0 | ranged auto-attack damage |
| FUN_004a1640 | melee HitResult resolution |
| FUN_004a0fb0 | general/ranged HitResult + armor/resist mitigation |
| FUN_004a19d0 | skill-delta → chance lookup table |
| FUN_004a0e40 | uniform RNG roll [0,1) |
| FUN_0056b750 | int64 → float conversion |
| FUN_00480720 | on-damage threat/aura notify |
| FUN_00429cd0 | spell-effect dispatch / per-target CombatMsg loop |
| FUN_0049ea40 | Aura_PeriodicDamage construct (sets tickInterval from template) |
| FUN_0049eb60 | Aura_PeriodicHeal/RestoreMana construct |
| FUN_00410df0 | **AuraEffect periodic tick** (the tick formula, §4.2) |

## 6. Open / uncertain
- Exact A* vs straight-line for player pathing (FUN_004a2e00 internals are STL-graph plumbing; straight
  spline reproduces captured behaviour). **Use straight point-to-point spline for the reimpl.**
- Exact MapCellT walkable bit values (flag byte is map-file driven; not enumerated in code).
- `.rdata` float constant *values* (0059304c/00592ed4/00592eb0/00592fd8/00592f08) inferred, not read;
  dump them from the binary `.rdata` to lock crit multiplier & percent divisor numerically.
- GP_Server_CombatMsg exact field order/width — lock against a live op-CombatMsg hexdump.
