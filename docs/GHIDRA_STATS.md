# GHIDRA_STATS.md — Item / Stat / Leveling formulas (server, r1188)

Mined from `deps/ghidra/dump/decompiled.c` (10.5MB, Ghidra) + `strings.txt`. Cross-referenced with
`PROTOCOL.md` (var-block: stat varId = `0x0e + UnitDefines::Stat`) and the SQL schema strings.
All `FUN_xxxxxxxx` are raw addresses; source file names come from embedded `ASSERTION FAILED` strings
(`Item.cpp`, `ServerUnit.cpp`, `ServerPlayer.cpp`). Confidence flags inline.

The code is MSVC-inlined STL-heavy; the DB **loaders** decode cleanly (struct layouts are solid), but the
runtime **crunch/recalc** is spread across inlined `std::map`/`shared_ptr` walks with no symbols. Where I
could not recover exact arithmetic I say so explicitly rather than guess.

---

## 0. Key primitives (decoded — HIGH confidence)

RNG helpers in `Item.cpp` (all confirmed by structure):
- `FUN_0043d550(min,max)` = **randInt(min,max)** inclusive (`@0043d550`). Used everywhere as the roller.
- `FUN_0043d5d0()` = **randDouble()** in `[0,1)` (`@0043d5d0`, builds 53-bit mantissa).
- `FUN_0043d630(N)` = **`randInt(0,99) < N`** i.e. an **N% chance roll** (`@0043d630`; calls `randInt(0,0x63)`).
  → `FUN_0043d550(0,0x63)` (= randInt 0..99) is the canonical percent-roll source.

Config globals (GameCache / config, init'd null at `FUN` @00458xxx, populated from config load):
- `DAT_005c3b78` = **MAX_PLAYER_LEVEL** (level cap). Used as a clamp on item/char level and as
  `maxLevel/5` to scale affix count (`@0043d810` lines ~60280). HIGH confidence it is the level cap.
- `DAT_005c3c70`, `DAT_005c3cb0` = config-driven **scaling/threshold maps** (`std::map<int,vector<...>>`,
  keyed by item level) consulted during generation (`@0043d810`). They hold the per-level affix-value
  multipliers and material/quality tables. MEDIUM confidence on exact contents (config-loaded, not in SQL).
- `DAT_005c3c48`, `DAT_005c3d08` = static cache roots (gem/quality table; thread-init guard). 

GameCache singleton accessor: `FUN_0043cfc0()` ( = `sGameCache`). Cache map roots (offsets into it):
- `+0x60`  = **player_exp_levels** tree (`PlayerExpLevel`), set by `FUN_004a9f00`.
- `+0x108` = **player_class_stats** tree (`PlayerClassStatEntry`), set by `FUN_004afcd0`.
- `+0x158` = **material_chance_armor** map (`FUN_004a8cb0`); `+0x160`ish = material_chance_weapon (`FUN_004a9710`).
- item_template / affix_template / item_gems trees built by `FUN_004be0e0` / `FUN_004bbd30` / `FUN_004a92c0`.
- All loaders are called in sequence from the master cache-init `FUN_004a7b30` (lines ~158938+).

---

## 1. Item stat computation (crunchItemStats equivalent)

### 1a. DB row shape (the stored item) — HIGH confidence (schema strings)
A stored item (player_equipment / inventory / bank / mail) is:
```
{ entry, affix, affix_score, gem_1, gem_2, gem_3, gem_4, enchant_level, soulbound, [count], durability }
```
(`strings.txt` lines 345/349/366/379/385; identical column set across all four containers.)

`item_template` (loaded by `FUN_004be0e0`, SQL @0058ccf8 / decompiled line 181178) columns:
```
entry, sort_entry, required_level, weapon_type, armor_type, equip_type, quality, item_level,
durability, sell_price, stack_count, weapon_material, num_sockets, required_class, quest_offer,
flags, generated, buy_cost_ratio, spell_1..5,
stat_type1..stat_value10   (10 inherent stat slots)
```
`affix_template` (loaded by `FUN_004bbd30`, SQL @0058d000 / line 179134):
```
entry, min_level, max_level, stat_type1..stat_value5   (affix = up to 5 stats, gated by item level range)
```
`item_gems` (loaded by `FUN_004a92c0`, SQL "FROM `item_gems`" near line 159900):
```
gem_id, quality, stat_1, stat_1_amount, stat_2, stat_2_amount
```
Decoded `ItemGemEntry` layout (from `FUN_004a92c0`, ItemGemEntry vftable @160014):
`[+0xc]=gem_id(key)`, fields `stat_1, stat_1_amount, stat_2, stat_2_amount` at obj words `[4..8]`.
→ **A socketed gem adds flat `stat_N_amount` to `stat_N`** (two flat stats per gem, no scaling). HIGH confidence.

### 1b. The combine formula (effective item stats) — MEDIUM/LOW confidence on exact scalars
`Item::Item` ctor = `FUN_0043d050` (asserts `m_itemTemplate = sGameCache->getItemTemplate(...)`,
Item.cpp:0x13). The effective-stat assembly is inlined; the recoverable structure is:

```
effective_item_stat[T]  =  template.stat_value(slot where stat_type==T)        # inherent (flat)
                         +  affix_contribution(T)                              # see below
                         +  Σ gem.stat_amount  (over gem_1..gem_4, both gem stats)   # flat add, §1a
                         +  enchant_contribution(T)                            # see below
```

- **Affix scaling by `affix_score`.** The affix carries `stat_type1..5 / stat_value1..5`. The stored
  `affix_score` is the per-item roll quality (an integer "roll"). Item generation `FUN_0043d810`
  multiplies a rolled stat by a per-level scalar:
  `pfVar[1] = (float)scaleEntry[5] * pfVar[1]`  (lines 60048-60049 and 60342) — i.e.
  **stat_value is multiplied by a level-indexed multiplier `param_6[itemLevel].mult`** pulled from the
  `DAT_005c3cb0` map, then `affix_score` records the realized roll. The exact runtime application of
  the *stored* `affix_score` to regenerate the displayed stat (e.g. `final = base + value*affix_score/SCALE`)
  could not be pinned to a constant SCALE in the static dump. **FLAG: affix_score→stat scaling factor is
  UNCERTAIN** — most likely `value * affix_score / 100` or `value * affix_score / max_score`, but the
  divisor is not provable from the decompile alone. Verify empirically (the PROTOCOL.md equip experiment
  with affix 8116 "Guard" stat_type1=3 changed `var 0x11` by exactly +3 → at full score the affix's
  `stat_value` is applied 1:1, consistent with `score == 100%`).

- **Enchant bonus (`enchant_level`).** Treated as an additive per-level bonus on the item's primary
  stat(s). The dump shows enchant_level loaded as a plain int alongside affix_score (no multiply visible
  in the loader). **FLAG: enchant formula UNCERTAIN** — pattern is `+= enchant_level * perLevelStep`,
  step not recovered. Most retail builds of this lineage use a flat `+1 stat per enchant level` or a
  small % of base; treat as `bonus = enchant_level * k` with k≈(1 .. small) until confirmed by experiment.

**Bottom line for §1:** template stats are flat; gems are flat adds (HIGH); affix and enchant are the two
scaled terms and their *scalars* are the only genuinely uncertain part of the whole stat system.
Cite: `FUN_0043d050` (Item ctor), `FUN_0043d810` (generation/roll @0043d810), `FUN_004a92c0` (gems),
`FUN_004bbd30` (affix), `FUN_004be0e0` (template).

---

## 2. Base stat → effective unit Stat array (the `0x0e+` var block)

### 2a. Base per class/level — HIGH confidence (loader fully decoded)
`player_class_stats` loaded by **`FUN_004afcd0`** (SQL @00578xxx / decompiled line 167763), columns:
```
class, level, hp, mana, strength, agility, willpower, intelligence, courage
```
Stored into a **nested tree keyed (class → level) → PlayerClassStatEntry** (vftable @167838; the loader
builds an outer map on `class` whose value is an inner map on `level`). So `getBaseStats(class, level)`
is a two-level lookup. This directly supplies the seven base attributes + base hp/mana for a given level.

### 2b. Invested stats — HIGH confidence (schema) / runtime app MEDIUM
`player_stat_invest{ guid, stat_type, amount }` (SQL @00588480, loaded inside the player-load
`FUN_0048b500` block, decompiled line 133538; saved by `FUN_00455330`, INSERT line 81933;
full re-save = DELETE-all then re-INSERT, string @0058852c). Loaded into a `stat_type→amount` map on the
player. Each invested point adds **flat `amount` to `UnitDefines::Stat[stat_type]`** (1:1; `stat_type` is
the 0-based UnitDefines::Stat index — same index space as item `stat_typeN` and the `0x0e+T` var ids).

### 2c. Aggregation onto the unit (the recalc) — STRUCTURE known, exact order MEDIUM
The effective stat array the client sees as `var 0x0e + T` (PROTOCOL.md "StatsStart = 0x0e") is recomputed
in `ServerUnit` / `ServerPlayer` (ServerUnit.cpp, ServerPlayer.cpp). The recoverable model:
```
Stat[T] = classBase[class][level].attr(T)        # §2a, for the 7 attributes; hp/mana seed MaxHealth/Mana
        + Σ investedStat[T]                       # §2b, flat
        + Σ over equipped items: effectiveItemStat[T]   # §1
        + Σ active auras' stat mods               # buffs/debuffs (player_aura table, FUN @133480 area)
```
Then derived stats (MaxHealth `var 0x03`, MaxHealth-stat `var 0x10` ArmorValue, weapon/melee values,
resists `0x23-0x26`, crit/dodge, etc.) are computed from the attributes. PROTOCOL.md already confirms the
**player NewWorld emits the full contiguous `0x01..0xbb` var table**, and that applying one affix stat moved
exactly one `0x0e+T` slot → the aggregation is a straight per-stat sum into that array. **FLAG: the exact
attribute→derived formulas (e.g. how Strength maps to WeaponValue, how MaxHealth is derived from hp-base +
stats) are NOT individually recovered here** — they live in the inlined ServerUnit recalc and need a
focused pass or live experiment.

---

## 3. Leveling (player_exp_levels)

### 3a. XP table — HIGH confidence (loader fully decoded)
`player_exp_levels` loaded by **`FUN_004a9f00`** (SQL @00587de0 / decompiled line 161420), columns:
```
level, exp, kill_exp
```
Stored into a **tree keyed by `level`** (GameCache `+0x60`). Decoded `PlayerExpLevel` struct
(`FUN_004a9f00`, vftable @161494, atoi parse lines 161498-161515):
```
PlayerExpLevel { int level;  int exp;  int kill_exp; }   // obj words [3],[4],[5]
```
- `exp`      = **XP required to advance from `level` → `level+1`** (the threshold for this row's level).
- `kill_exp` = the **base XP a mob of this level grants** (used by the kill-reward path).

### 3b. Level-up logic — STRUCTURE known, MEDIUM confidence
On XP gain (var `0xa4 = Experience`, PROTOCOL.md), the player's Experience is compared against
`getExpLevel(currentLevel).exp`; when `currentExp >= threshold`, level increments, Experience carries the
remainder, and the server emits **`GP_Client_LevelUp`** (RTTI string `.?AUGP_Client_LevelUp@@`,
strings.txt line 2264). The XP cap is `DAT_005c3b78` (MAX_PLAYER_LEVEL) — at cap, no further level-ups.

### 3c. Level-up stat gain — DERIVED, HIGH confidence on mechanism
There is **no separate "stat gain on level" table**. Level gains come *implicitly* from §2a: the base-stat
recalc re-queries `player_class_stats[class][newLevel]`, so each level simply swaps in the next row's
hp/mana/str/agi/will/int/courage. **The per-level stat increase = (class_stats row[level+1] − row[level])**
for that class. (Plus the player is typically granted invest points to spend → `player_stat_invest`.)
Cite: `FUN_004afcd0` (class table), `FUN_004a9f00` (exp table).

---

## 4. Material chance (item generation: pick material by level)

Two tables, both keyed by `level`:
- `material_chance_armor{ level, armor_type, chance }`  — `FUN_004a8cb0` (SQL @00585f00 / line 160202),
  stored at GameCache `+0x158`. Loader fully decoded (atoi of 3 columns into the entry).
- `material_chance_weapon{ level, weapon_material, chance }` — `FUN_004a9710` (SQL @00585f88 / line 160903).

**Selection mechanism (HIGH confidence on shape, MEDIUM on exact tie-break):** during generation
(`FUN_0043d810`), for the item's level the server walks the candidate material rows and rolls
`FUN_0043d550(0,99)` (the percent roller from §0) against each row's `chance` (`FUN_0043d630(chance)` =
"chance% hit"), iterating from the highest/rarest material down, taking the first that passes; the `level`
key bounds which materials are eligible (you can only roll materials whose row `level <= itemLevel`).
This mirrors the `loot_*_chance` / vendor `green/blue/gold/purple_chance` pattern seen in `npc_template`
and `npc_vendor_random` (strings.txt lines 337/339). **FLAG: whether rows are evaluated rarest-first or
as a cumulative weighted pick is not 100% provable from the dump — but the per-row `chance` is a flat
percent consumed by the 0..99 roller.**

---

## 5. Stat-upgrade cost (computeStatUpgradeCost) + cap

**NOT CONCLUSIVELY FOUND.** There is no `computeStatUpgradeCost`-named string and no isolated cost formula
recovered. What exists:
- `player_stat_invest{stat_type, amount}` is the storage for spent points (§2b); the **save path**
  (`FUN_00455330`) writes current invest state, but the *cost/validation* of an invest request is handled
  in the invest packet handler (a `Client_*` opcode handler in ServerPlayer/Game), which was not isolated.
- The only hard **cap** evidence is `DAT_005c3b78` (MAX_PLAYER_LEVEL) gating progression, and per-affix
  `min_level/max_level` gating which affixes can appear. No explicit per-stat hard cap constant was found.
- **FLAG: computeStatUpgradeCost and any per-stat cap are UNRESOLVED.** Likely candidates: the invest
  handler reads the player's current `amount` for that stat and charges a cost that scales with the
  already-invested count (a common `cost = base + amount*step` shape), debiting `var 0x9e = Money` or a
  dedicated stat-point pool (`var 0xa7 = NumInvestedSpells` is the spell analogue; a stat-point counter
  likely sits in the same 0xa0-block). Recommend: grep the Client opcode dispatch for the invest handler
  and the Money (`0x9e`) debit, or just trigger an invest live and watch the money/var deltas.

---

## Summary of confidence

| Topic | Confidence | Primary FUN_ |
|---|---|---|
| RNG primitives (randInt/randFloat/percent-roll) | HIGH | 0043d550, 0043d5d0, 0043d630 |
| Item DB row shape + template/affix/gem schemas | HIGH | 004be0e0, 004bbd30, 004a92c0 |
| Gem = flat stat add | HIGH | 004a92c0 |
| Affix_score → stat *scalar* (divisor) | LOW | 0043d810 (+ DAT_005c3cb0 config) |
| Enchant_level bonus *step* | LOW | 0043d050 / 0043d810 |
| Base class stats (class,level)→attrs | HIGH | 004afcd0 |
| Invested stats = flat add | HIGH (schema) | 0048b500, 00455330 |
| Stat aggregation = per-stat sum into 0x0e+T | HIGH (mechanism) / MED (derived-stat formulas) | ServerUnit recalc (not isolated) |
| Exp table {level,exp,kill_exp} + meaning | HIGH | 004a9f00 |
| Level-up swaps class-stats row (gain = row delta) | HIGH (mechanism) | 004afcd0 + 004a9f00 |
| Material pick by level via %chance roll | MED | 004a8cb0, 004a9710, 0043d810 |
| computeStatUpgradeCost / stat cap | UNRESOLVED | — |

### Highest-value follow-ups to retire the LOW/UNRESOLVED items
1. **Live experiment** (cheapest): equip items with known affix at varying `affix_score`, and enchant an
   item +1/+2, watching the `0x0e+T` var delta → nails affix_score divisor and enchant step exactly.
2. Trigger a stat invest and watch `var 0x9e` (Money) / stat-point var delta → recovers cost formula + cap.
3. Targeted Ghidra pass on the ServerUnit recalc (find the function writing the contiguous `0x0e..` block)
   for the attribute→derived-stat formulas (MaxHealth, WeaponValue, resists).
