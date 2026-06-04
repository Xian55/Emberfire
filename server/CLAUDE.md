# server/ — future from-scratch game server

**Placeholder today** — `src/main.cpp` just prints "not implemented" so the target builds. The real plan is
a from-scratch server that speaks the `shared/` wire protocol and replaces the leaked game-server binary we
currently reverse-engineer (run from `%EMBERFIRE_GAME_DIR%`).

## When implementing
- The recovered server architecture/spec is in `docs/SERVER_BACKEND.md` (opcodes, DB schema, packet
  layouts, AI/movement/spell systems), backed by `docs/GHIDRA_*.md`.
- Link `emberfire::shared` for the protocol; new code may use C++20.
- Add tests under `tests/` (GoogleTest, already wired).
- A separate from-scratch emulator exists elsewhere whose headers contradict measured data — cross-ref
  only, do NOT import it.
