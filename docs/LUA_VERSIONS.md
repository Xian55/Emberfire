# Lua versions & flavors — why Emberfire uses native modern Lua (decision record)

Source: deep-research pass (web search → fetch → adversarial verification → synthesis), early 2026.
Primary sources cited inline. This is a *rationale* doc, not protocol ground-truth — figures may drift.

## TL;DR
World of Warcraft and Roblox stay on the **Lua 5.1 lineage** by deliberate choice, not inertia.
The post-5.1 changes are real improvements, but two of them cost an *embedder* dearly: `_ENV`
breaks the C **API/ABI**, and the 5.3/5.4 numeric changes **silently alter results** of already-
shipped scripts. For hosts with a huge script corpus (WoW addons, Roblox experiences) or that run
untrusted code (Roblox), those costs outweigh the gains — so they freeze or fork. Modern Lua is a
**deliberate tradeoff** (cleaner semantics + 64-bit ints, paid for in compatibility), *not bloat*.

Emberfire has **no legacy addon corpus**, so the migration cost is ~0 → modern native Lua is the
right call, and **LuaBridge3 binds to any of 5.1–5.5 / LuaJIT / Luau / Ravi**, so the binder never
forces the version.

## Flavor comparison at a glance

| | Host lang | Lua ver | Speed | Sandbox | Niche |
|---|---|---|---|---|---|
| PUC-Lua 5.4 | C | 5.4 | interpreter | DIY | generic embed |
| LuaJIT | C | 5.1 | JIT (fastest) | DIY | perf-critical |
| Luau | C++ | 5.1+fork | fast interp | hardened, built-in | untrusted code at scale |
| MoonSharp | C#/.NET | 5.2 | slowest (managed interp) | opt-out stdlib modules | C#/Unity games |

## What changed, Lua 5.1 → 5.4
Source: <https://www.lua.org/versions.html>
- **5.2 (2011):** `goto`/labels; **`_ENV`** (environments become a compile-time upvalue, *replacing*
  `setfenv`/`getfenv`); `bit32` lib; yieldable `pcall`; `__len` for tables; ephemeral tables.
- **5.3 (2015) — the big break:** **integer/float subtype** (64-bit integers as a distinct number
  type); native bitwise ops `& | ~ << >>`; floor division `//`; `utf8` lib; `string.pack`/`unpack`.
- **5.4 (2020):** generational GC mode; to-be-closed vars `local x <close>`; `<const>`; new PRNG;
  **removed string→number coercion in arithmetic**; integer-overflow literals become floats; numeric
  `for` control vars no longer wrap.

None of this is junk. PUC-Lua 5.4 is still tiny. The problem is *migration*, not size.

## The catch — two costs
**A. ABI/API break (`_ENV`).** 5.2 replaced the per-function environment mechanism with `_ENV`, an
implicit upvalue resolved at compile time. Any host/script relying on `setfenv`/`getfenv` for sandbox
or module isolation breaks, and the C-side contract shifts. Ref: <https://leafo.net/guides/setfenv-in-lua52-and-above.html>

**B. Silent behavior changes (5.3 integers, 5.4 coercion).** These don't error — they change results,
which is worse for migration. `3/2 == 1.5` but `3//2 == 1`; `print(1)` ≠ `print(1.0)`; serialization
/ hashing / `tostring` output shift. 5.4 makes `"10" + 5` stricter in arithmetic; numeric `for` stops
wrapping on overflow. Documented in the Lua 5.4 manual §8.1 (incompatibilities), mirrored at
<https://cyevgeniy.github.io/luadocs/08_incompatibilities/intro.html>. For a shipped game with
thousands of community addons, "silently different results" is the real disincentive.

## LuaJIT — frozen at 5.1
LuaJIT stays **API+ABI-compatible with Lua 5.1 by design** (<https://luajit.org/extensions.html>,
<https://en.wikipedia.org/wiki/LuaJIT>). That promise *forbids* `_ENV` — you can't keep the 5.1 ABI
and adopt the 5.2 environment model. LuaJIT cherry-picks safe 5.2/5.3 *library* extensions behind
flags, but the language core stays 5.1. So the freeze is structurally entailed by the ABI guarantee,
not a whim. (Maintainer-bandwidth / ecosystem-fragmentation factors exist too, but the ABI reason is
the core technical one.) LuaJIT's dominance is much of why "5.1 is the lingua franca."

## World of Warcraft Lua
- **Version: Lua 5.1.1**, since **Patch 2.0.1 (2006-12-05)**; *"Only a subset of version 5.1 of the
  official Lua specification is implemented."* Ref: <https://warcraft.wiki.gg/wiki/Lua> (pre-2.0 used 5.0).
  Still 5.1 ~20 years on — the addon ecosystem is the anchor.
- **Sandbox = subset + taint, not a hardened VM.** No `io`, no `os` file/process access, no
  `package`/`require`/file loading. **Taint propagation** marks code secure (Blizzard) vs tainted
  (addon); taint spreads via data flow and **protected actions** (cast/target/move in combat) refuse
  to run from tainted code. This is *gameplay-integrity* sandboxing (stop combat automation), not
  memory-safety sandboxing. Ref: <https://wowpedia.fandom.com/wiki/Secure_Execution_and_Tainting>.
- WoW's reason to stay on 5.1 is almost purely **addon backward-compat**; taint sits on top and is
  version-agnostic.

## Roblox Luau
A **fork of Lua 5.1** with a from-scratch rewritten compiler + register VM (not a patched PUC build).
Keeps 5.1 as baseline but **selectively backports** later features and adds its own.
Refs: <https://luau.org/why/>, <https://github.com/luau-lang/luau>.

**Compatibility decisions** — <https://luau.org/compatibility/>:

| Feature | Luau | Stated reason |
|---|---|---|
| `goto` (5.2) | Rejected | "complicates the compiler, makes control flow unstructured" |
| `_ENV` (5.2) | Rejected | keeps `getfenv`/`setfenv` (community depends on them) |
| `bit32` (5.2) | Adopted | preferred over native bit operators |
| 64-bit integer subtype (5.3) | **Rejected** | "backwards compatibility and performance implications"; single number type |
| native bitwise ops (5.3) | **Rejected** | `bit32` "provides superior functionality" |
| floor division `//` (5.3) | Adopted | — |
| `utf8` library (5.3) | Adopted | — |
| `string.pack`/`unpack` (5.3) | Adopted | — |
| to-be-closed `<close>` (5.4) | Rejected | "syntax inconsistent with attributes; no strong use cases"; ships `<const>` |
| `__gc` metamethod | Rejected | sandboxing + memory-safety + isolation problems → tag-based destructors |
| generational GC (5.4) | Deferred | — |

Note the pattern: **Luau rejected exactly the two things that hurt embedders most** — the integer
subtype (silent numeric break + perf) and `_ENV` (sandbox/compat break) — while taking the safe
library additions. That is the whole thesis in one table.

**Added beyond modern Lua:**
- **Gradual structural type system** — opt-in annotations, *static analysis only* (type errors are
  warnings; no runtime enforcement). <https://create.roblox.com/docs/luau/type-checking>
- **Performance** — rewritten optimizing compiler + register VM, generally faster than PUC 5.1.
- **Hardened sandbox** (<https://luau.org/sandbox/>): removes `io`, `package`, most of `debug` (keeps
  `traceback`/`info`), `dofile`/`loadfile`, `string.dump`, `load`, `module`; trims `os` to
  `clock/date/difftime/time`; `collectgarbage` limited to `"count"`; `loadstring` rejects bytecode.
  VM-level: global table, every library table, and the string metatable are **read-only** (blocks
  assignment, `rawset`, `setmetatable`); each script gets a **fresh global table** that `__index`-
  chains to builtins. No `__gc`. **Explicitly NOT guaranteed:** sandbox "isn't formally proven";
  **no memory limit by default** (a script can exhaust host address space); CPU protection is a
  cooperative interrupt that can't preempt a single long-running C call. Host-exposed C functions
  remain the host's security responsibility.

Roblox forks rather than upgrades because it must run millions of users' scripts *safely + fast*, and
neither goal is served by 5.3/5.4 semantics.

## MoonSharp — different platform, not for us
A Lua interpreter written **entirely in C#** for .NET / Mono / Xamarin / Unity3D. Implements **Lua
5.2** ("99% compatible," minus weak tables). Interpreted, no JIT. 3-clause BSD.
Ref: <https://www.moonsharp.org/about.html>.
- Reason it exists: zero-native-code .NET interop — runs on AOT/IL2CPP (iOS/consoles) where you can't
  ship a native Lua `.dll`, and binds directly to CLR objects. Killer feature for Unity/C# games.
- **Non-starter for Emberfire:** the client is C++ (x86) with LuaBridge + native Lua. MoonSharp means
  dragging in a .NET/Mono runtime; LuaBridge can't bind to it; it's the slowest option (managed
  interpreter, no JIT); weak tables missing; sandbox is coarse (opt-out stdlib modules). Only relevant
  if the client ever became .NET — which contradicts the x86-C++ `static_assert` foundation.

## Verdict — tradeoff, not bloat
Every post-5.1 feature is individually defensible (64-bit ints fix a real correctness gap; `utf8` /
`string.pack` fill holes; generational GC cuts pauses). What modern Lua *is*, is **less compatible**.
Giant embedders don't move because: (1) they value a frozen API/ABI over new features (`_ENV` alone
disqualifies ABI-stable hosts → LuaJIT); (2) shipped script ecosystems make silent semantic changes
unacceptable (5.3 ints, 5.4 coercion); (3) untrusted-code hosts need sandbox + perf guarantees upstream
doesn't prioritize (→ Roblox forks). "Bloat without improvement" is the wrong frame; it's "improvement
the embedder can't afford to migrate to."

## Decision for Emberfire
Our situation differs in the one way that matters: **no legacy script corpus to protect** — the Lua
UI layer is being written fresh. That removes cost B (silent migration breakage) almost entirely.
- **LuaBridge3 doesn't force the version** — it transparently targets PUC-Lua **5.1 → 5.5** + **LuaJIT
  2.1+** + **Luau** + **Ravi** from one source tree (<https://kunitoki.github.io/LuaBridge3/Manual.html>).
  Binder choice is decoupled from runtime choice; the core can be swapped later without rewriting bindings.
- **Modern PUC-Lua 5.4 is a net win for us** — correct 64-bit integers (entity IDs, item GUIDs,
  timestamps — exactly what a game client passes around) + better GC, at zero migration cost. The
  integer subtype that scares WoW/Roblox is a *benefit* here.
- **Pick by constraint:** trusted addons (our case) → modern PUC-Lua 5.4. Untrusted third-party
  addons → need Luau-style sandboxing (read-only globals, stripped io/os/package, interrupt CPU guard);
  PUC gives none of that OOTB. Raw script throughput the bottleneck → LuaJIT (5.1, JIT) — unlikely for
  UI handlers.
- **Security perimeter = the host C surface.** Both Luau docs and WoW's taint model agree: the sandbox
  only covers the *language*; every C function exposed via LuaBridge is ours to make safe. Lock down
  what the Lua UI can call into the client.

**Bottom line:** the migration to a modern native Lua was correct — without a legacy addon corpus, the
compatibility costs that pin WoW/Roblox to 5.1 don't apply, and modern Lua's integer + GC improvements
are wins. Treat the LuaBridge C surface as the real attack surface; only reach for Luau/sandboxing if
the UI is ever opened to untrusted third-party scripts.

## Caveats
- Luau's compat/sandbox/why pages are first-party — design facts verifiable, value adjectives
  ("only", "state-of-the-art") are theirs. Luau's sandbox is explicitly **not formally proven** and
  gives **no** memory/termination guarantee.
- Version numbers current early 2026 (LuaBridge3 lists up to PUC 5.5.0, Luau 0.713, Ravi 1.0-beta11).
- A Luau RFC proposes **opt-in explicit 64-bit integers** (<https://rfcs.luau.org/type-long-integer.html>),
  which could soften the "rejects integers" finding later.

## Sources
- <https://www.lua.org/versions.html> · <https://luajit.org/extensions.html>
- <https://luau.org/compatibility/> · <https://luau.org/sandbox/> · <https://luau.org/why/>
- <https://create.roblox.com/docs/luau/type-checking> · <https://github.com/luau-lang/luau>
- <https://warcraft.wiki.gg/wiki/Lua> · <https://wowpedia.fandom.com/wiki/Secure_Execution_and_Tainting>
- <https://kunitoki.github.io/LuaBridge3/Manual.html> · <https://www.moonsharp.org/about.html>
- Lua 5.4 manual §8.1 (incompatibilities), mirrored: <https://cyevgeniy.github.io/luadocs/08_incompatibilities/intro.html>
- <https://leafo.net/guides/setfenv-in-lua52-and-above.html>
