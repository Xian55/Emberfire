# auth/ — fake LAN auth server + MySQL schema

A tiny HTTP server that emulates the game's web auth: validate a login, mint a token, store it in MySQL;
the game server reads that token to admit the client. Runs on **Bun** (`bun start`).

## Files
- `auth_server.js` — the server. Config + secrets from **env** (`dotenv` loads `auth/.env`):
  `AUTH_PORT` (80 — the patched client hardcodes 80), `DB_HOST/DB_PORT/DB_USER/DB_PASSWORD/DB_NAME`.
  Hardcoded demo accounts (`test/test`, `admin/admin`, `player/player`) — not secrets.
- `db_init.sql` — owns the whole MySQL schema (player, inventory, guild, `auth_tokens`, ...). Run once into
  the `dreadmyst` database (db name kept as `dreadmyst`).
- `docker-compose.yml` — MySQL stack; `MYSQL_ROOT_PASSWORD` from `.env`.
- `.env.example` (committed placeholders) → copy to `.env` (gitignored, real values). Or generate `.env`
  from the root `.env` via `scripts/setup.ps1`.

## Rules / gotchas
- **Secrets only in `auth/.env`** (gitignored). Never hardcode creds in `auth_server.js` / compose / sql.
- JS runs on **Bun** (not node).
- Schema note: `player.account` is **signed `INT`** — char delete soft-deletes via `account = -accountId`,
  which `INT UNSIGNED` rejects ("Out of range value for column 'account'"). Keep it signed.
