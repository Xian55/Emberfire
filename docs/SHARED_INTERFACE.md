# Reconstructed interface of the missing `..\Shared\` module
Harvested from client usage (read-only) on 2026-05-31. Source: the client source tree (`client/src`).
This is the spec implemented in `shared/include/Emberfire/Shared/*`. **Bold = must byte-match the r1188 server** (values
unknown from source → resolve via live capture, see `../capture/`).

## A. Serialization & transport (protocol core)

### StlBuffer (StlBuffer.h) — reader+writer byte buffer, default-constructible `StlBuffer{}`
- `size_t size() const`
- `void eraseFront(size_t n)` — consume from front
- `StlBuffer& operator>>(T&)` — **reads from front, consumes, throws std::invalid_argument if short**
- `StlBuffer& operator<<(const T&)` — appends; chainable
- `void writeFile(const std::string&)`, `bool readFile(const std::string&)`
- Element types used with >>/<<: uint16_t, uint8_t/unsigned char, int, float, bool, std::string.
- **Unknown (capture): endianness, how std::string is length-prefixed, bool/float width.**
- Note: `getString/getInt32/getGuid/getFloat/getSize/getTextRef` are NOT called by client — the GP_*
  packet classes do their own internal (un)serialization via >>/<< inside build()/unpack().

### GamePacket framework (GamePacketClient.h / GamePacketServer.h)
- `static bool GamePacket::validOpcode(uint16_t)`
- Each packet: `StlBuffer& build(StlBuffer&&)` (writes its opcode + fields, returns ref for sendPacket)
  and `void unpack(StlBuffer&)` (reads fields; opcode already stripped by caller).
- Receive loop (Game.cpp:225): `data >> opcode(uint16_t); data.eraseFront(2); validOpcode; switch.`
- **Opcode is uint16_t-backed enum. Numeric values UNKNOWN; switch order (below) hints sequential.**

### SfSocket / TcpSocketEx (SfSocket.h) — framed packet layer over SFML TCP
- `TcpSocketEx : sf::TcpSocket`; `make_shared<TcpSocketEx>()`; `setBlocking(bool)`;
  `sf::Socket::Status connect(const std::string& addr, unsigned short port, sf::Time timeout)`.
- `SfSocket(shared_ptr<TcpSocketEx>, SfSocket::Type)`; `enum class Type{ SocketType_ClientSide, ... }`.
  `bool update()` (false=disconnected), `void cancel()`, `void sendPacket(StlBuffer&)`,
  `void popReceived(vector<unique_ptr<StlBuffer>>&)`, `bool connected() const`.
- **Framing: non-blocking; SfSocket adds a length prefix, emits whole packets each beginning with the
  2-byte opcode. Exact prefix size/endianness UNKNOWN → capture.**

### Opcode members — exact names known; VALUES unknown
Mutual: `Mutual_Ping`.
Server (Game.cpp:249-322 switch order = ordering hint):
Server_QueuePosition, Server_Validate, Server_CharacterList, Server_NewWorld, Server_CharaCreateResult,
Server_Player, Server_Npc, Server_GameObject, Server_SetController, Server_Inventory, Server_Bank,
Server_OpenBank, Server_EquipItem, Server_WorldError, Server_ObjectVariable, Server_UnitSpline,
Server_DestroyObject, Server_ChatMsg, Server_SetSubname, Server_GuildRoster, Server_GuildInvite,
Server_GuildAddMember, Server_SpellGo, Server_UnitTeleport, Server_CombatMsg, Server_CastStart,
Server_CastStop, Server_NotifyItemAdd, Server_OpenLootWindow, Server_UnitOrientation, Server_Cooldown,
Server_UnitAuras, Server_Spellbook, Server_GossipMenu, Server_AcceptedQuest, Server_QuestList,
Server_QuestTally, Server_QuestComplete, Server_RewardedQuest, Server_AbandonQuest, Server_SpentGold,
Server_ExpNotify, Server_AvailableWorldQuests, Server_InspectReveal, Server_SocketResult,
Server_EmpowerResult, Server_RespawnResponse, Server_Spellbook_Update, Server_ChatError, Server_OfferDuel,
Server_LvlResponse, Server_AggroMob, Server_LearnedSpell, Server_UpdateVendorStock, Server_RepairCost,
Server_QueryWaypointsResponse, Server_DiscoverWaypointNotify, Server_OfferParty, Server_PartyList,
Server_OnObjectWasLooted, Server_MarkNpcsOnMap, Server_TradeUpdate, Server_TradeCanceled,
Server_GuildOnlineStatus, Server_GuildNotifyRoleChange, Server_ChannelInfo, Server_PromptRespec,
Server_ChannelChangeConfirm, Server_ArenaQueued, Server_ArenaReady, Server_ArenaOutcome,
Server_ArenaStatus, Server_PkNotify; (also exist, dispatched elsewhere) Server_UnlockGameObj,
Server_GuildRemoveMember.
Client: one Client_<X> per GP_Client_<X> below.

### Send-side packet field order (client→server) — authoritative serialization order
Handshake first: **`GP_Client_Authenticate { m_userToken:string, m_build:int=DYMST_VERSION, m_fingerprint:string }`** (Login.cpp:153).
Then `GP_Client_CharacterList{}`, `GP_Client_CharCreate{m_gender,m_portrait,m_name:string,m_classId}`,
`GP_Client_EnterWorld{m_playerGuid}`, `GP_Client_DeleteCharacter{m_playerGuid}`,
`GP_Client_CastSpell{m_targetGuid,m_spellId,m_targetPosX,m_targetPosY,m_ctm:bool}`,
`GP_Client_RequestMove{m_wasd,m_destX,m_destY}`, `GP_Client_RequestStop{m_myX,m_myY}`,
`GP_Client_SetSelected{m_guid}`, `GP_Client_ChatMsg{m_channelId,m_text:string,m_targetName:string,m_itemId:ItemDefinition}`,
... (full list in agent A output / regenerate via grep `\.build(StlBuffer`). Empty packets = opcode only.

### Server→Client packet layouts
Field READ order recoverable from each `Game::processPacket_Server_*` handler (Game.cpp). Key:
`Server_CharacterList.m_characters[]` = {name, classId, level, guid, portrait, gender} (Game.cpp:420);
`Server_Validate.m_result` = AccountDefines::AuthenticateResult (Game.cpp:365);
`Server_ObjectVariable` = {m_variableId:int, m_value:int} → setVariable (Game.cpp:1831).

## B. Support libs

### Config (Config.h) — `#define sConfig Config::instance()` (returns Config*)
- `void setSource(const char* path, bool localPath=true)`
- `std::string getString(const char* sec,const char* key,const char* def="")`
- `int getInt(const char*,const char*,int def=0)`; `bool getBool(const char*,const char*,bool def=false)`
- `void setInt(...)`, `void setString(...,const char* val)`
- `void registerCache(int id,const char* sec,const char* key)`; `int getCache(int id)`
- Path logic in Application.cpp:527-581 (cwd config.ini, else %APPDATA%\Dreadmyst\config.ini).

### Util (namespace/class Util) — free helpers, full list:
fmtStr(printf->string), base64_encode(string)->string (Connector.cpp:91; no decode used),
strReplaceAll(string&,from,to), maskHas(mask,flag)->bool (templated), GeoBox2d (nested type ctor(x,y,w,h)),
readLinesFromFile(path,vector&), randomChoice(variadic), formatMoneyCommas(int64)->string,
frand(float,float), trimStr(string&), getFileList(dir,vector&), irand(int,int), cordsInBox(...),
toLowerCase(string)->string, readTextFile(path)->string, toUtf16(string)->wstring, splitStr(in,vec,delim),
parseIntExpression(string)->int, compareNaturalSort(a,b)->bool, brightenColor(sf::Color,factor),
toRoman(int)->string, simulateInputBox(...), sfKeyToString(sf::Keyboard::Key)->string,
roll_chance_i(int)->bool, formatTimeBySeconds(int)->string.

### Md5 (Md5.h): `class MD5{ MD5(); std::string digestString(const std::string&); }` (hex). Only digestString used.

### DmystVersion (DmystVersion.h): `#define DYMST_VERSION <int>` (note spelling "DYMST").
Window title uses "r"+DYMST_VERSION → client is **r1189 ⇒ DYMST_VERSION=1189** (confirm in capture authenticate pkt).

### Singletons: hand-rolled Meyers per class: `static T* instance(){static T t;return &t;} #define sT T::instance()`.
Only `sConfig` (and out-of-scope `sItemDefiner`) come from Shared; sApplication/sContentMgr/sConnector/sKeybinds are in client.

## C. Enums & structs (ItemDefiner.h, *Defines.h, Mutual*/Map*/CooldownHolder)
Full member lists captured in agent-C output (this session). **Regenerate any list via** e.g.
`grep -rhoE 'ChatDefines::[A-Za-z_:]+' *.cpp *.h | sort -u`. Wire-critical (values must match server):
- **AccountDefines::AuthenticateResult** {WrongVersion,BadPassword,ServerFull,Banned,Validated} (switch Game.cpp:365).
- **ObjDefines::Variable** — big int enum; object state synced by raw int id over wire (Server_ObjectVariable).
  StatsStart..StatsEnd must contiguously map UnitDefines::Stat (client does `Stat(var-StatsStart)`).
- **UnitDefines::Stat** — ordered to NumStats; index-iterated; NullStat sentinel.
- **ChatDefines::Channels**, **ItemDefines::Quality**(ordinal), **GuildDefines::Rank**(ordinal),
  **PlayerDefines::Classes/Gender/ChatError/WorldError**, SpellDefines::{Effects,AuraType,HitResult},
  UnitDefines::{Faction,EquipSlot,Direction(8, rotational order)}.
- **ItemDefinition** (ItemDefines.h): first field `uint16_t m_itemId`, then m_affixId,m_affixScore,
  m_enchantLvl,m_durability,m_soulbound,m_gem1..4. Brace-init from id `{m_itemId}`. Carried in packets.
Shared structs: MutualObject (guid/variable store/type/name/subname), MutualUnit (stats/health/mana/
orientation/spline), MapCellT (Flags{Unwalkable,CollideBlock}), GameMap (layers/cells/screen-pos),
MapLogic (static LOS/path), CooldownHolder (__time64_t cooldowns), ItemDefiner (sItemDefiner; DB loaders).
Full method signatures: agent-C output.

## Open wire unknowns to crack from capture (gates everything)
1. StlBuffer encoding: int/uint16 endianness, string length-prefix width+encoding, bool/float width.
2. SfSocket length-prefix framing (size field width/endianness).
3. Opcode integer values (base + ordering/gaps).
4. DYMST_VERSION the server accepts (expected 1189).
5. ObjDefines::Variable + UnitDefines::Stat integer values (from Server_ObjectVariable stream + stat panel).
