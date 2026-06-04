# tools/capture/ — packet capture + protocol RE tooling

Bun (not node) scripts used to reverse the wire protocol.

- `proxy.js` — TCP MITM proxy: client → `127.0.0.1:PROXY_PORT` → real server (`SERVER_HOST:SERVER_PORT`).
  Logs every byte both directions and decodes framed packets. `bun proxy.js`. Env: `PROXY_PORT` (16500),
  `SERVER_HOST` (127.0.0.1), `SERVER_PORT` (16383). It writes `session-*.ndjson` + `*.decoded.log`
  (gitignored).
- `analyze.js` — `bun analyze.js`: reassembles frames / matches Server-opcode hexdumps vs field reads.
- `*.js` DB miners (cls/enum_dump/npcflag/schema_dump/xtab) — read `game.db` via **`bun:sqlite`**
  (`Database`, `{readonly:true}`); DB path from `EMBERFIRE_GAME_DIR`.

## Capture workflow (how protocol facts were nailed)
Point the client's `[Tcp] Port` at the proxy (e.g. 16500), play, and the proxy logs the wire. Comparing the
same action on the working original client vs the rebuilt one (capture both) is the reliable way to find a
wrong byte/field/enum — that diff drove almost every protocol fix. Decoder field map: `proxy.js`.

Rule: JS here runs on **Bun**. Don't reintroduce `node:sqlite` or `node <script>`.
