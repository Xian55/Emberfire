# Opcode table — extracted from the SHIPPED CLIENT binary (Dreadmyst.exe)

Method: Ghidra headless decompile of `Dreadmyst_Server/bin/Dreadmyst.exe` (`dump_client/decompiled.c`).
Each packet's serialize (vtable slot0) starts with the write-u16 helper `FUN_004157a0(<opcode>)`.
Write primitives: `FUN_004157a0`=u16, `FUN_0041c7a0`=u8, `FUN_00415950`=string.
Client→server values fall in 1..74 (server dispatch `switch(opcode-1)`); server→client in 75..150.
The client binary links the shared `GP_Server_*::build`, so both directions are present.

Confidence: **SOLID** = RTTI slot0 ∩ recon agree, or client→server validated vs server handler.
**MED** = RTTI slot0 only (some vtable reads alias adjacent classes — verify per-system via live capture).
**?** = value known to exist but packet not yet named (inline, no RTTI) — resolve in its system phase.

## CLIENT → SERVER (1..74)  — wrong value = server DROPS connection
| op | packet | conf |
|----|--------|------|
| 1  | Mutual_Ping | SOLID |
| 2  | Client_Authenticate | SOLID |
| 4  | Client_CharCreate | SOLID |
| 5  | Client_EnterWorld | SOLID (known) |
| 9  | Client_EquipItem | MED |
| 11 | Client_UseItem | MED |
| 15 | Client_RequestMove | SOLID |
| 16 | Client_RequestStop | SOLID (known) |
| 17 | Client_ChatMsg | SOLID |
| 18 | Client_GuildMotd | SOLID |
| 22 | Client_GuildInviteMember | MED |
| 23 | Client_GuildCreate | SOLID |
| 24 | Client_GuildInviteResponse | MED |
| 25 | Client_GuildPromoteMember | MED |
| 26 | Client_GuildDemoteMember | MED |
| 27 | Client_CastSpell | SOLID (handler FUN_004957d0 = "Spell/Cast") |
| 31 | Client_SetIgnorePlayer | MED |
| 37 | Client_Repair | MED |
| 47 | Client_PartyInviteMember | MED |
| 49 | Client_PartyChanges | MED |
| 52 | Client_OpenTradeWith | MED |
| 59 | Client_UnBankItem | MED |
| 62 | Client_MoveInventoryToBank | MED |
| 67 | Client_UpdateArenaStatus | MED |
| 70 | Client_MOD_MutePlayer | MED |
| 71 | Client_MOD_KickPlayer | MED |
| 72 | Client_MOD_BanPlayer | MED |
| 73 | Client_MOD_WarnPlayer | MED |
| 74 | Client_ReportPlayer | MED |
| 3,6,7,8,10,12,13,14,19,20,21,28,29,30,32-36,38-46,48,50,51,53-58,60,61,63-66,68,69 | inline client sends (DeleteChar, CancelCast, SetSelected, item move/split/sort/sell/destroy/unequip, buy/buyback, bank ops, loot, cancelbuff, req-spell/abilitylist, togglepvp, rolldice, gossip-option, quest accept/complete/abandon, waypoints, respawn, respec, resetdungeons, changechannels, mail, trade sub-ops, guild kick/quit/disband/roster, duel) | ? per-system |

## SERVER → CLIENT (75..150)
| op | packet | conf |
|----|--------|------|
| 3  | Server_Validate | SOLID |
| 75 | Server_CharacterList | SOLID |
| 76 | Server_CharaCreateResult | SOLID (recon) |
| 77 | Server_Player (== NewWorld spawn; NO separate NewWorld class) | SOLID |
| 78 | Server_Npc | SOLID |
| 79 | Server_GameObject | SOLID |
| 80 | Server_SetController | SOLID |
| 82 | Server_ObjectVariable | SOLID |
| 83 | Server_InspectReveal | MED |
| 84 | Server_NotifyItemAdd | recon (RTTI aliased to 103 — verify) |
| 85 | Server_Inventory | SOLID |
| 88 | Server_OfferDuel | MED |
| 90 | Server_UnitSpline | SOLID |
| 91 | Server_ChatMsg | SOLID |
| 94 | Server_GuildAddMember | MED |
| 96 | Server_SetSubname | SOLID |
| 97 | Server_GuildInvite | MED |
| 100| Server_SpellGo | SOLID |
| 102| Server_CombatMsg | SOLID (recon) |
| 104| Server_OpenLootWindow | SOLID |
| 108| Server_UnitAuras | SOLID |
| 109| Server_Spellbook | SOLID |
| 110| Server_GossipMenu | SOLID |
| 111| Server_QuestList | SOLID |
| 120| Server_AvailableWorldQuests | SOLID |
| 122| Server_Spellbook_Update | SOLID |
| 130| Server_QueryWaypointsResponse | MED |
| 132| Server_OfferParty | MED |
| 133| Server_PartyList | SOLID |
| 135| Server_MarkNpcsOnMap | SOLID |
| 139| Server_GuildNotifyRoleChange | MED |
| 140| Server_GuildOnlineStatus | MED |
| 141| Server_Bank | SOLID |
| 143| Server_ChannelInfo | SOLID |
| 150| Server_PkNotify | MED |
| others (76,81,84,87,89,99,103,105-107,112-119,121,124-129,131,134,136-138,142,144-150) | EquipItem, DestroyObject, WorldError, CastStart/Stop, Cooldown, UnitOrientation/Teleport, Bank/OpenBank, quest tally/complete/reward/abandon, guild roster/invite, trade update/cancel, arena, vendor stock, repair cost, exp/spent, learned spell, respawn/socket/empower, channel change, prompt respec | recon + per-system |

Raw extraction: `dump_client/opcodes.txt`. Server-side cross-ref: `recon/GHIDRA_PROTOCOL.md`.
