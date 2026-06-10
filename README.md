# Emberfire

Emberfire is a modern C++ monorepo for a retro-style MMO: a rebuilt game **client**, a shared
**wire protocol** library, a placeholder **server** (future, from-scratch), plus a LAN auth server
and protocol-capture tooling.

> Personal, **LAN-only** project. Not affiliated with or endorsed by the original developers, and not for
> redistribution. The bundled game runtime (binaries, content, maps, DB) is **not** in this repo.

## Lua-driven UI & addons

The biggest quality-of-life change over the vanilla Dreadmyst client: the user interface has been
migrated from hardcoded C++ to **Lua**. The default UI (HUD, unit frames, login/character screens,
minimap chrome, dialogs) ships as plain Lua scripts in `client/ui/`, and the client exposes a
**World of Warcraft-style addon API** — `CreateFrame`, `SetPoint`, `SetScript`, `RegisterEvent`,
SavedVariables, `.toc` files, `/reload` — so the user experience can be customized or extended with
WoW-syntax-like addons dropped into an `addons/` folder, no recompile needed.

Addon code runs in a **sandboxed Lua 5.5** environment (whitelisted libraries, no `io`/`os`/file
access, instruction-count watchdog), so a misbehaving addon can't hang or escape the client.

```
shared/   reconstructed wire protocol + game defines (header-only, emberfire::shared)
client/   the rebuilt client: src/ (C++), support layer + deps/, Shared = junction -> ../shared
server/   FUTURE from-scratch server (placeholder target today; spec in docs/SERVER_BACKEND.md)
tools/    capture/ : the TCP proxy + frame decoder used to reverse the protocol
auth/     fake LAN auth server (node) + MySQL schema; secrets via .env (see .env.example)
docs/     reverse-engineering findings (PROTOCOL, OPCODES, GHIDRA_*, SERVER_BACKEND, ...)
cmake/    dependency + junction helpers
```

## Build (Windows, x86)

The client static_asserts 32-bit and links SFML 2.6.1 (x86). Requirements: Visual Studio 2022, CMake ≥ 3.21.

One-time setup — capture your local setup in `.env`, then apply it (this is also how you resume the
project later):

```powershell
copy .env.example .env          # then edit .env: your deps/game paths + DB creds
powershell scripts/setup.ps1    # setx EMBERFIRE_* + (re)writes auth/.env  (open a NEW shell after)
```

`.env` is gitignored and is the single place that holds your machine's setup (see Environment variables).

```powershell
cmake --preset x86-release          # configure (creates the client/Shared junction)
cmake --build --preset x86-release  # build client + server + tests
ctest  --preset x86-release         # run the unit tests
```

Dependencies:
- **SFML 2.6.1 (x86)** and the `zip.dll` runtime are referenced from `EMBERFIRE_DEPS_DIR`
  (override with `-D EMBERFIRE_DEPS_DIR=<dir-with x86/SFML-2.6.1>`). A `vcpkg.json` is provided as an
  opt-in alternate.
- **sqlite** amalgamation, the `zip` header + import lib, and the steam/nvapi/NTSTATUS **stubs** are
  vendored in `client/deps/`.
- **GoogleTest** is fetched at configure time (FetchContent).

## Run

The client needs the game runtime (`content/maps/scripts/game.db` + DLLs), which is **not** committed.
Point a gitignored `run/` (or a junction) at your local game build and launch the exe from there.

## Auth + DB

`auth/` runs the fake login server against MySQL. Copy `auth/.env.example` to `auth/.env`, fill in real
values (the `.env` is gitignored), then `cd auth && bun install && bun start`. Schema: `auth/db_init.sql`.
(The JS tooling runs on [Bun](https://bun.sh) — `bun <script>.js`, not node.)

## Environment variables

All machine-specific paths and credentials come from environment variables — none are committed.

**Build / run** (CMake, `scripts/run.ps1`, `tools/capture/*`). Set persistently with `setx NAME value`
(open a new shell to take effect):

| var | purpose | default |
|-----|---------|---------|
| `EMBERFIRE_DEPS_DIR` | prebuilt deps root — expects `x86/SFML-2.6.1` + `zip/zip.dll` | _(required)_ |
| `EMBERFIRE_GAME_DIR` | game runtime — `content/maps/scripts/game.db`; used to build `run/` and by the capture DB scripts | _(required)_ |

**Auth + MySQL** — live in `auth/.env` (copy from `auth/.env.example`), read by `auth_server.js` and `docker-compose.yml`:

| var | purpose | default |
|-----|---------|---------|
| `AUTH_PORT` | auth server listen port (the patched client hardcodes 80) | `80` |
| `DB_HOST` | MySQL host | `127.0.0.1` |
| `DB_PORT` | MySQL port | `3306` |
| `DB_USER` | MySQL user | `root` |
| `DB_PASSWORD` | MySQL password | _(set it)_ |
| `DB_NAME` | database name | `dreadmyst` |
| `MYSQL_ROOT_PASSWORD` | docker container root password (match `DB_PASSWORD`) | _(set it)_ |

**Capture proxy** (`tools/capture/proxy.js`):

| var | purpose | default |
|-----|---------|---------|
| `PROXY_PORT` | proxy listen port (point the client's `[Tcp] Port` here) | `16500` |
| `SERVER_HOST` | real game-server host to forward to | `127.0.0.1` |
| `SERVER_PORT` | real game-server port | `16383` |

## Secrets

No real secrets are committed. All credentials live in a gitignored `.env`; only `.env.example`
(placeholders) is tracked.
