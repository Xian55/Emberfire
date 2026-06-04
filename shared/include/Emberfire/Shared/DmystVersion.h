#pragma once
// Client build number. Confirmed from live capture: Client_Authenticate.m_build == 1189
// (window title renders "r" + DYMST_VERSION). Server r1188 enforces a floor only:
// FUN_0048a400 rejects m_build < 0x4a4 (1188) with Validate result=4. Any build >= 1188
// is accepted (no upper bound), so 1190 passes the handshake.
#define DYMST_VERSION 1190
