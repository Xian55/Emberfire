# Emberfire — project guide

Modern C++ (x86) monorepo for a retro-style MMO: a reverse-engineered, rebuilt **client**, a header-only
**shared** wire-protocol library, a placeholder **server** (future), plus a LAN auth server and
protocol-capture tooling. Private project, LAN-only. See `README.md` for the user-facing version.

## Layout (each dir has its own CLAUDE.md)
| dir | what |
|-----|------|
| `shared/` | reconstructed wire protocol + game defines (header-only, `emberfire::shared`) |
| `client/` | the C++ client: `src/` + reconstructed support layer (at `client/` root) + `deps/` |
| `server/` | placeholder target; future from-scratch server (`docs/SERVER_BACKEND.md`) |
| `tools/capture/` | TCP proxy + frame decoder used to reverse the protocol (Bun) |
| `auth/` | fake LAN auth server (Bun) + MySQL schema; secrets via gitignored `.env` |
| `docs/` | reverse-engineering findings (PROTOCOL, OPCODES, GHIDRA_*, SERVER_BACKEND) |
| `cmake/` | dependency + `client/Shared` junction helpers |
| `scripts/` | `setup.ps1` (apply `.env`), `provision.ps1` (make a build runnable), `run.ps1` |

## Build / run / test (Windows, x86)
```powershell
cmake --preset x86-release          # configure (creates the client/Shared junction)
cmake --build --preset x86-release  # client + server + tests
ctest  --preset x86-release         # unit tests
```
The build-dir exe runs in place — POST_BUILD (`scripts/provision.ps1`) seeds assets + junctions next to it.
Launch `build/x86-release/client/Release/emberfire_client.exe`, or `scripts/run.ps1` to deploy to `run/`.

## Conventions / hard rules
- **Paths are env-var driven — never hardcode `F:\...`.** `EMBERFIRE_DEPS_DIR` (prebuilt SFML 2.6.1 x86 +
  `zip.dll`) and `EMBERFIRE_GAME_DIR` (game runtime: content/maps/scripts/game.db). Full list: README.
  `F:\Dreadmyst` is the external runtime/dep provider — **keep it as-is**, reference it, don't move it.
- **JS runs on Bun, not node.** `bun:sqlite` (not `node:sqlite`), `bun <script>.js`, `bun start`.
- **Secrets live only in gitignored `.env` / `auth/.env`.** Committed `*.env.example` are placeholders.
  Never commit secrets, binaries, `build/`, `run/`, `*.db`, or game runtime — `.gitignore` enforces this.
  Re-run the pre-push audit (grep staged content for creds) before any push.
- **Branding is Emberfire**, but the **MySQL db name stays `dreadmyst`** and `docs/` keep provenance.
- **x86 only** — the client `static_assert`s 32-bit. Configure with the x86 preset.
- Commit messages end with: `Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>`.

## Where the hard-won knowledge lives
The wire protocol facts (opcodes, packet layouts, WorldError codes, spell attribute bits) are encoded in
`shared/` headers + locked by `shared/tests`. Reverse-engineering rationale is in `docs/`. When changing a
packet/enum, update the matching test.
