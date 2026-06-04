# server/ — future Emberfire game server

Currently a **placeholder** target so the monorepo builds end to end. The real plan is a from-scratch
server that speaks the reconstructed wire protocol (`shared/`) and replaces the leaked game-server
binary we currently reverse-engineer and run from the local game build (`%EMBERFIRE_GAME_DIR%`).

## Reference
- `docs/SERVER_BACKEND.md` — the server architecture/spec recovered from the leaked binary
  (opcodes, DB schema, packet layouts, AI/movement/spell systems).
- `docs/GHIDRA_*.md` — the underlying reverse-engineering findings.
- `shared/` — the wire protocol + game defines both client and server use.

## Not adopted
A separate from-scratch emulator exists elsewhere whose headers contradict our measured data — cross-ref
only, do not import.
