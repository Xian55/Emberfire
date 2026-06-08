#include "stdafx.h"
#include "Game.h"
#include "ContentMgr.h"
#include "Login.h"
#include "Application.h"
#include "Options.h"
#include "Connector.h"
#include "ConfirmMessageBox.h"
#include "CharacterSelection.h"
#include "CharacterCreation.h"
#include "World.h"
#include "ClientPlayer.h"
#include "lua/LuaEngine.h"
#include "lua/LuaEvents.h"
#include "lua/LuaFrameManager.h"
#include "Inventory.h"
#include "ItemIcon.h"
#include "Equipment.h"
#include "ClientNpc.h"
#include "GameChat.h"
#include "GuildRoster.h"
#include "WorldSpellAnimation.h"
#include "ClientMap.h"
#include "UnitFrame.h"
#include "Sprite.h"
#include "LootWindow.h"
#include "VendorNpc.h"
#include "Abilities.h"
#include "QuestOffer.h"
#include "QuestLog.h"
#include "QuestComplete.h"
#include "Minimap.h"
#include "DialogNpc.h"
#include "QuestObjectives.h"
#include "LevelupNotify.h"
#include "MapQuester.h"
#include "ClientGameObj.h"
#include "Toolbar.h"
#include "TradeWindow.h"
#include "Bank.h"
#include "TextLines.h"
#include "ScrollBar.h"

#include "..\Rand.h"
#include "..\StringHelpers.h"
#include "..\SqlConnector\QueryResult.h"
#include "..\Shared\AccountDefines.h"
#include "..\Shared\SpellDefines.h"
#include "..\Shared\GamePacketClient.h"
#include "..\Shared\GamePacketServer.h"
#include "..\Shared\CharacterDefines.h"
#include "..\Shared\Config.h"
#include "..\Shared\MapLogic.h"
#include "..\Shared\QuestDefines.h"
#include "..\Shared\ItemDefines.h"
#include "..\Shared\GuildDefines.h"

#include <assert.h>

Game::Game(RenderObjectHolder& owner, const int id) :
	RenderObjectHolder(&owner, id),
	m_stage(RoNull),
	m_serverTime(0),
	m_guildInviteId(0)
{
	// Multi-input so the active stage AND the persistent Lua UI both receive input.
	setMultiInput(true);

	// One persistent Lua frame manager for every screen (login..world). It outlives stage switches
	// (setStage only destroys the active stage id). Load the default UI + addons once, now.
	addRenderObject(make_shared<LuaFrameManager>(*this, RoLuaRoot));
	sLua->loadAddons();
}

Game::~Game()
{
	if (sApplication->getWindow().isOpen())
	{
		sContentMgr->stopMusic();
		sConnector->cancel();
	}
}

void Game::input()
{
	__super::input();

	if (sApplication->keyUp(sf::Keyboard::Escape) && getRenderObject(RoOptions) == nullptr && sApplication->popKeyUp(sf::Keyboard::Escape))
		toggleOptions(true);

	if (auto confirmBox = sApplication->popConfirmBox({ ConfirmMessageBox::ConfirmCode_GuildInvite, ConfirmMessageBox::ConfirmCode_Respawn, ConfirmMessageBox::ConfirmCode_DuelOffer,
		ConfirmMessageBox::ConfirmCode_RepairConfirm, ConfirmMessageBox::ConfirmCode_PartyOffer, ConfirmMessageBox::ConfirmCode_RespecPrompt, ConfirmMessageBox::ConfirmCode_ArenaOffer }))
	{
		switch (confirmBox->getCode())
		{
			case ConfirmMessageBox::ConfirmCode_ArenaOffer:
			{
				GP_Client_UpdateArenaStatus pk;
				pk.m_enterArena = true;
				sConnector->sendPacket(pk.build(StlBuffer{}));
				break;
			}
			case ConfirmMessageBox::ConfirmCode_RespecPrompt:
			{
				if (confirmBox->getResult() == ConfirmMessageBox::ConfirmBox_Yes)
				{
					GP_Client_Respec pk;
					sConnector->sendPacket(pk.build(StlBuffer{}));
				}

				break;
			}
			case ConfirmMessageBox::ConfirmCode_GuildInvite:
			{
				GP_Client_GuildInviteResponse pk;
				pk.m_guildId = m_guildInviteId;
				pk.m_accept = confirmBox->getResult() == ConfirmMessageBox::ConfirmBox_Yes;
				sConnector->sendPacket(pk.build(StlBuffer{}));
				break;
			}
			case ConfirmMessageBox::ConfirmCode_Respawn:
			{
				GP_Client_RequestRespawn pk;
				sConnector->sendPacket(pk.build(StlBuffer{}));
				break;
			}
			case ConfirmMessageBox::ConfirmCode_DuelOffer:
			{
				GP_Client_DuelResponse pk;
				pk.m_accept = confirmBox->getResult() == ConfirmMessageBox::ConfirmBox_Yes;
				sConnector->sendPacket(pk.build(StlBuffer{}));
				break;
			}
			case ConfirmMessageBox::ConfirmCode_PartyOffer:
			{
				GP_Client_PartyInviteResponse pk;
				pk.m_accept = confirmBox->getResult() == ConfirmMessageBox::ConfirmBox_Yes;
				sConnector->sendPacket(pk.build(StlBuffer{}));
				break;
			}
			case ConfirmMessageBox::ConfirmCode_RepairConfirm:
			{
				if (confirmBox->getResult() == ConfirmMessageBox::ConfirmBox_Yes)
				{
					GP_Client_Repair pk;
					pk.m_confirmed = true;
					sConnector->sendPacket(pk.build(StlBuffer{}));
				}
				break;
			}
		}
	}
}

void Game::render()
{
	__super::render();

	if (sConnector->connected() && ::clock() - sApplication->getLastInput() > 900 * 1000)
	{
		sApplication->logout();
		return;
	}

	if (sConnector->connected() && !sConnector->update())
	{
		sApplication->logout();
	}
	else
	{
		vector<unique_ptr<StlBuffer>> packets;
		sConnector->popReceived(packets);

		for (auto& packet : packets)
			processPacket(*packet);
	}
}

void Game::toggleOptions(const bool v)
{
	const bool exists = getRenderObject(RoOptions) != nullptr;

	// Only play sound when status changes
	if (exists != v)
	{
		if (v)
			sContentMgr->playSound(SfxId::WindowOpen);
		else
			sContentMgr->playSound(SfxId::WindowClose);
	}

	destroyObjectById(RoOptions);

	if (v)
		addRenderObject(make_shared<Options>(*this, RoOptions));
}

/*static*/
string Game::getItemSound(const int itemId)
{
	return sContentMgr->db("item_template").data(itemId, "icon_sound");
}

/*static*/ 
void Game::playItemSound(const int itemId)
{
	// Nothing bad would happen without this check, but may as well try to save ourself some searching if the parameter is null
	if (itemId != 0)
		sContentMgr->playSound(getItemSound(itemId));
}

void Game::setStage(const Ro stage, const bool overrideDuplicate /*= false*/)
{
	if (m_stage == stage && !overrideDuplicate)
		return;

	destroyObjectById(m_stage);
	m_stage = stage;

	if (m_stage == RoLogin)
	{
		addRenderObject(make_shared<Login>(*this, m_stage));
		sLua->fire(LuaEvents::LOGIN_SHOWN, "");

		// Dev auto-login: submit configured creds immediately (the C++ Login's own AutoLogin can't run while
		// it's force-hidden by the Lua login). CharacterSelection's AutoLogin then auto-enters the world.
		if (sConfig->getInt("Debug", "AutoLogin", 0))
		{
			if (auto login = dynamic_pointer_cast<Login>(getRenderObject(RoLogin)))
			{
				string err;
				login->performLogin(sConfig->getString("Debug", "AutoLoginUser", "test"),
				                    sConfig->getString("Debug", "AutoLoginPass", "test"), false, err);
				if (!err.empty())
					blog(Logger::LOG_ERROR, "[autologin] %s", err.c_str());
			}
		}
	}
	else if (stage == RoCharacterSelection)
	{
		addRenderObject(make_shared<CharacterSelection>(*this, m_stage));
		sLua->fire(LuaEvents::CHARSELECT_SHOWN, "");
	}
	else if (stage == RoCharacterCreation)
	{
		addRenderObject(make_shared<CharacterCreation>(*this, m_stage));
		sLua->fire(LuaEvents::CHARCREATE_SHOWN, "");
	}
	else if (stage == RoWorld)
	{
		addRenderObject(make_shared<World>(*this, m_stage));
		sLua->fire(LuaEvents::WORLD_SHOWN, "");   // World registered => the HUD's currentWorld() resolves
	}
	else
		ASSERT(0);

	// Keep the persistent Lua UI rendering above the freshly-added stage (renders last == on top).
	requestMoveToTop(RoLuaRoot);

	toggleOptions(false);
}

void Game::processPacket(StlBuffer& data)
{
	uint16_t opcode;

	if (data.size() < sizeof(opcode))
	{
		blog(Logger::LOG_ERROR, "Processed packet with size less than %d", sizeof(opcode));
		return;
	}

	data >> opcode;
	// NOTE: reconstructed StlBuffer::operator>> CONSUMES (original PEEKED), so the
	// opcode is already removed; the original's explicit eraseFront(sizeof(opcode))
	// here would double-consume 2 payload bytes and misalign every handler.

	if (!GamePacket::validOpcode(opcode))
	{
		blog(Logger::LOG_ERROR, "Received bad opcode %d", opcode);
		return;
	}

	// --- PACKET CAPTURE HARNESS (ground-truth wire bytes for layout RE) ---
	// Dumps raw hex of WATCHED incoming opcodes to pktlog.txt BEFORE unpack, so we can decode the real
	// layout by matching known values (itemId from DB, guid, durability, etc.) instead of trusting the
	// stripped decompile. Edit the watch set per investigation; remove the block when done.
	{
		static const bool kCaptureEnabled = false;   // dev RE capture; OFF by default (opens+writes pktlog.txt
		                                              // per watched packet => heavy disk I/O during streaming).
		static const std::set<int> kWatch = { 90, 103, 107, 111, 113, 127, 135 };
		if (kCaptureEnabled && kWatch.count(opcode))
		{
			std::ofstream dbg("pktlog.txt", std::ios::app);
			const auto& r = data.raw();
			dbg << "RECV op=" << opcode << " size=" << r.size() << " hex=";
			for (size_t i = 0; i < r.size() && i < 96; ++i) { char b[4]; snprintf(b, 4, "%02x", r[i]); dbg << b << " "; }
			dbg << "\n";
		}
	}

	try
	{
		const auto perfT0 = std::clock();   // PERF: time each packet handler, flag the slow ones
		// Maybe set this up as an array of functions or something
		switch (opcode)
		{
			case Opcode::Mutual_Ping: processPacket_Mutual_Ping(data); break;
			case Opcode::Server_QueuePosition: processPacket_Server_QueuePosition(data); break;
			case Opcode::Server_Validate: processPacket_Server_Validate(data); break;
			case Opcode::Server_CharacterList: processPacket_Server_CharacterList(data); break;
			case Opcode::Server_NewWorld: processPacket_Server_NewWorld(data); break;
			case Opcode::Server_CharaCreateResult: processPacket_Server_CharaCreateResult(data); break;
			case Opcode::Server_Player: processPacket_Server_Player(data); break;
			case Opcode::Server_Npc: processPacket_Server_Npc(data); break;
			case Opcode::Server_GameObject: processPacket_Server_GameObject(data); break;
			case Opcode::Server_SetController: processPacket_Server_SetController(data); break;
			case Opcode::Server_Inventory: processPacket_Server_Inventory(data); break;
			case Opcode::Server_Bank: processPacket_Server_Bank(data); break;
			case Opcode::Server_OpenBank: processPacket_Server_OpenBank(data); break;
			case Opcode::Server_EquipItem: processPacket_Server_EquipItem(data); break;
			case Opcode::Server_WorldError: processPacket_Server_WorldError(data); break;
			case Opcode::Server_ObjectVariable: processPacket_Server_ObjectVariable(data); break;
			case Opcode::Server_UnitSpline: processPacket_Server_UnitSpline(data); break;
			case Opcode::Server_DestroyObject: processPacket_Server_DestroyObject(data); break;
			case Opcode::Server_ChatMsg: processPacket_Server_ChatMsg(data); break;
			case Opcode::Server_SetSubname: processPacket_Server_SetSubname(data); break;
			case Opcode::Server_GuildRoster: processPacket_Server_GuildRoster(data); break;
			case Opcode::Server_GuildInvite: processPacket_Server_GuildInvite(data); break;
			case Opcode::Server_GuildAddMember: processPacket_Server_GuildAddMember(data); break;
			case Opcode::Server_SpellGo: processPacket_Server_SpellGo(data); break;
			case Opcode::Server_UnitTeleport: processPacket_Server_UnitTeleport(data); break;
			case Opcode::Server_CombatMsg: processPacket_Server_CombatMsg(data); break;
			case Opcode::Server_CastStart: processPacket_Server_CastStart(data); break;
			case Opcode::Server_CastStop: processPacket_Server_CastStop(data); break;
			case Opcode::Server_NotifyItemAdd: processPacket_Server_NotifyItemAdd(data); break;
			case Opcode::Server_OpenLootWindow: processPacket_Server_OpenLootWindow(data); break;
			case Opcode::Server_UnitOrientation: processPacket_Server_UnitOrientation(data); break;
			case Opcode::Server_Cooldown: processPacket_Server_Cooldown(data); break;
			case Opcode::Server_UnitAuras: processPacket_Server_UnitAuras(data); break;
			case Opcode::Server_Spellbook: processPacket_Server_Spellbook(data); break;
			case Opcode::Server_GossipMenu: processPacket_Server_GossipMenu(data); break;
			case Opcode::Server_AcceptedQuest: processPacket_Server_AcceptedQuest(data); break;
			case Opcode::Server_QuestList: processPacket_Server_QuestList(data); break;
			case Opcode::Server_QuestTally: processPacket_Server_QuestTally(data); break;
			case Opcode::Server_QuestComplete: processPacket_Server_QuestComplete(data); break;
			case Opcode::Server_RewardedQuest: processPacket_Server_RewardedQuest(data); break;
			case Opcode::Server_AbandonQuest: processPacket_Server_AbandonQuest(data); break;
			case Opcode::Server_SpentGold: processPacket_Server_SpentGold(data); break;
			case Opcode::Server_ExpNotify: processPacket_Server_ExpNotify(data); break;
			case Opcode::Server_AvailableWorldQuests: processPacket_Server_AvailableWorldQuests(data); break;
			case Opcode::Server_InspectReveal: processPacket_Server_InspectReveal(data); break;
			case Opcode::Server_SocketResult: processPacket_Server_SocketResult(data); break;
			case Opcode::Server_EmpowerResult: processPacket_Server_EmpowerResult(data); break;
			case Opcode::Server_RespawnResponse: processPacket_Server_RespawnResponse(data); break;
			case Opcode::Server_Spellbook_Update: processPacket_Server_Spellbook_Update(data); break;
			case Opcode::Server_ChatError: processPacket_Server_ChatError(data); break;
			case Opcode::Server_OfferDuel: processPacket_Server_OfferDuel(data); break;
			case Opcode::Server_LvlResponse: processPacket_Server_LvlResponse(data); break;
			case Opcode::Server_AggroMob: processPacket_Server_AggroMob(data); break;
			case Opcode::Server_LearnedSpell: processPacket_Server_LearnedSpell(data); break;
			case Opcode::Server_UpdateVendorStock: processPacket_Server_UpdateVendorStock(data); break;
			case Opcode::Server_RepairCost: processPacket_Server_RepairCost(data); break;
			case Opcode::Server_QueryWaypointsResponse: processPacket_Server_QueryWaypointsResponse(data); break;
			case Opcode::Server_DiscoverWaypointNotify: processPacket_Server_DiscoverWaypointNotify(data); break;
			case Opcode::Server_OfferParty: processPacket_Server_OfferParty(data); break;
			case Opcode::Server_PartyList: processPacket_Server_PartyList(data); break;
			case Opcode::Server_OnObjectWasLooted: processPacket_Server_OnObjectWasLooted(data); break;
			case Opcode::Server_MarkNpcsOnMap: processPacket_Server_MarkNpcsOnMap(data); break;
			case Opcode::Server_TradeUpdate: processPacket_Server_TradeUpdate(data); break;
			case Opcode::Server_TradeCanceled: processPacket_Server_TradeCanceled(data); break;
			case Opcode::Server_GuildOnlineStatus: processPacket_Server_GuildOnlineStatus(data); break;
			case Opcode::Server_GuildNotifyRoleChange: processPacket_Server_GuildNotifyRoleChange(data); break;
			case Opcode::Server_ChannelInfo: processPacket_Server_ChannelInfo(data); break;
			case Opcode::Server_PromptRespec: processPacket_Server_PromptRespec(data); break;	
			case Opcode::Server_ChannelChangeConfirm: processPacket_Server_ChannelChangeConfirm(data); break;
			case Opcode::Server_ArenaQueued: processPacket_Server_ArenaQueued(data); break;
			case Opcode::Server_ArenaReady: processPacket_Server_ArenaReady(data); break;
			case Opcode::Server_ArenaOutcome: processPacket_Server_ArenaOutcome(data); break;
			case Opcode::Server_ArenaStatus: processPacket_Server_ArenaStatus(data); break;
			case Opcode::Server_PkNotify: processPacket_Server_PkNotify(data); break;
		}

		if (const long perfMs = long(std::clock() - perfT0); perfMs >= 3)
			blog(Logger::LOG_INFO, "[perf] op=%d handler took %ldms", opcode, perfMs);
	}
	catch (invalid_argument& e)
	{
		blog(Logger::LOG_ERROR, "Bad packet %s %d", e.what(), opcode);
	}
}

void Game::processPacket_Mutual_Ping(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Mutual_Ping pk;
	pk.unpack(data);

	sConnector->sendPacket(pk.build(StlBuffer{}));
}

void Game::processPacket_Server_QueuePosition(StlBuffer& data)
{
	GP_Server_QueuePosition pk;
	pk.unpack(data);

	sApplication->resetLastInputTime();
	sApplication->clearPopups();
	sApplication->spawnTimedPopup("Position in queue: " + Util::formatMoneyCommas(pk.m_position), 3.0f);
}

void Game::processPacket_Server_Validate(StlBuffer& data)
{
	GP_Server_Validate pk;
	pk.unpack(data);
	m_serverTime = pk.m_serverTime;
	
	switch (pk.m_result)
	{
		case AccountDefines::AuthenticateResult::WrongVersion:
		{
			sConnector->cancel();
			sApplication->spawnPopup("Your game needs to be updated. Try restarting Steam or asking for help on our Discord.", ConfirmMessageBox::ConfirmBoxType::ConfirmBox_Ok, 0);
			sLua->fire(LuaEvents::LOGIN_RESULT, "Your game needs to be updated.");
			break;
		}
		case AccountDefines::AuthenticateResult::BadPassword:
		{
			sConnector->cancel();
			sApplication->spawnTimedPopup("Failed to authenticate game server", 3.0f);
			sLua->fire(LuaEvents::LOGIN_RESULT, "Failed to authenticate game server");
			break;
		}
		case AccountDefines::AuthenticateResult::ServerFull:
		{
			sConnector->cancel();
			sApplication->spawnPopup("The server is full. Try again?", ConfirmMessageBox::ConfirmBoxType::ConfirmBox_YesNo, ConfirmMessageBox::ConfirmBoxCodes::ConfirmCode_RetryLogin);
			sLua->fire(LuaEvents::LOGIN_RESULT, "The server is full. Try again.");
			break;
		}
		case AccountDefines::AuthenticateResult::Banned:
		{
			sConnector->cancel();
			sApplication->spawnTimedPopup("Your account is suspended", 3.0f);
			sLua->fire(LuaEvents::LOGIN_RESULT, "Your account is suspended");
			break;
		}
		case AccountDefines::AuthenticateResult::Validated:
		{
			setStage(RoCharacterSelection);
			sApplication->spawnTimedPopup("Loading", 300.0f);
			break;
		}
	}
}

void Game::processPacket_Server_CharacterList(StlBuffer& data)
{
	// Server will send us a new character list when we're done creating
	if (getRenderObject(RoCharacterCreation) != nullptr)
	{
		setStage(RoCharacterSelection);
		sContentMgr->playSound(SfxId::AlertMissionGet);
	}

	auto cselection = dynamic_pointer_cast<CharacterSelection>(getRenderObject(RoCharacterSelection));
	
	if (cselection == nullptr)
	{
		blog(Logger::LOG_ERROR, "Received character list at bad time");
		return;
	}

	GP_Server_CharacterList pk;
	pk.unpack(data);

	cselection->clearCharacters();

	for (size_t i = 0; i < pk.m_characters.size(); ++i)
	{
		auto& character = pk.m_characters[i];
		cselection->registerCharacter(character.name, PlayerDefines::Classes(character.classId), character.level, character.guid, character.portrait, character.gender);
	}

	sApplication->clearPopups();
}

void Game::processPacket_Server_ChannelInfo(StlBuffer& data)
{
	GP_Server_ChannelInfo pk;
	pk.unpack(data);

	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	auto minimap = dynamic_pointer_cast<Minimap>(world->getRenderObject(World::MinimapObj));
	minimap->setChannelName("Channel " + to_string(pk.m_myChannel + 1));
	minimap->setCachedChannelInfo(pk.m_channelSize, pk.m_channels);
}

void Game::processPacket_Server_NewWorld(StlBuffer& data)
{
	GP_Server_NewWorld pk;
	pk.unpack(data);

	blog(Logger::LOG_INFO, "[timing] +%4ldms  NewWorld received", long(std::clock() - World::s_enterStartClock));

	// Stash the controlled player's spawn pos + var table + equipment (NewWorld has them;
	// SetController spawns the ClientPlayer and applies them).
	World::s_pendingSelf.x = pk.m_x;
	World::s_pendingSelf.y = pk.m_y;
	World::s_pendingSelf.hasPos = true;
	World::s_pendingSelf.vars = pk.m_variables;
	World::s_pendingSelf.equipment.clear();
	for (auto& e : pk.m_equipment)
		World::s_pendingSelf.equipment.push_back({ e.slot, e.item });

	if (getStage() != RoWorld)
	{
		setStage(RoWorld);
		sContentMgr->stopMusic();
		blog(Logger::LOG_INFO, "[timing] +%4ldms  setStage(RoWorld) done (World created)", long(std::clock() - World::s_enterStartClock));
	}

	if (auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld)))
	{
		// op77 (NewWorld/Player) carries NO real map id (m_mapId is aliased to the player guid, which
		// only matched fanadin for guid==1). Use the map id the server sends in op76 (CharaCreateResult
		// byte) just before this packet — stashed in s_pendingSelf.mapId; defaults to 1 (fanadin).
		// TODO confirm op76==mapId with a non-fanadin (dungeon) capture; if op76 is a controller flag,
		// find the true map-id source (see VERIFY_TODO / "real map source").
		world->setMap(World::s_pendingSelf.mapId);

		if (auto gameChat = dynamic_pointer_cast<GameChat>(world->getRenderObject(World::GameChatBox)))
		{
			gameChat->addLine("Welcome to Emberfire.", ChatDefines::Channels::System);
		}
	}
}

void Game::processPacket_Server_SetController(StlBuffer& data)
{
	GP_Server_SetController pk;
	pk.unpack(data);

	blog(Logger::LOG_INFO, "[timing] +%4ldms  SetController received (player spawned)", long(std::clock() - World::s_enterStartClock));

	if (auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld)))
		world->setController(pk.m_guid);

	// Could maybe clear this once the map finishes loading, but
	//  let's let the loading pop up hang until they're also in control and ready to play
	sApplication->clearPopups();

	// NOTE: PLAYER_LOGIN (the moment the Lua HUD reveals + the loading screen lifts) is NOT fired here.
	// The map fades in over ~3s after this (World::m_introAlpha), so the world is still black. World::render
	// fires PLAYER_LOGIN when that intro fade completes — see World.cpp.
}

void Game::processPacket_Server_Player(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_Player pk;
	pk.unpack(data);

	auto plr = make_shared<ClientPlayer>(pk.m_guid, &world->getMap(), static_cast<PlayerDefines::Classes>(pk.m_classId), static_cast<PlayerDefines::Gender>(pk.m_gender), pk.m_portraitId);
	plr->setOrientation(pk.m_orientation);
	plr->setName(pk.m_name);
	plr->setSubName(pk.m_subName);
	plr->setWorldPosition({ pk.m_x, pk.m_y });

	for (auto& itr : pk.m_equipment)
		plr->setEquipment(static_cast<UnitDefines::EquipSlot>(itr.first), itr.second);

	for (auto& itr : pk.m_variables)
		plr->setVariable(static_cast<ObjDefines::Variable>(itr.first), itr.second);

	if (!plr->isAlive())
		plr->finishAnimation();

	world->addClientObject(plr);
}

void Game::processPacket_Server_RespawnResponse(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr || world->myself() == nullptr)
	{
		sApplication->logout();
		return;
	}
	
	GP_Server_RespawnResponse pk;
	pk.unpack(data);

	if (pk.m_success)
	{
		sApplication->clearPopups();
		sContentMgr->playSound(SfxId::Teleport);
	}
}
		
void Game::processPacket_Server_EmpowerResult(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr || world->myself() == nullptr)
	{
		sApplication->logout();
		return;
	}
	
	auto gameChat = dynamic_pointer_cast<GameChat>(world->getRenderObject(World::GameChatBox));

	GP_Server_SocketResult pk;
	pk.unpack(data);

	if (pk.m_success)
	{
		sContentMgr->playSound(SfxId::SysGatheringSuccess);
		world->myself()->playAnimation(UnitDefines::AnimId::Cast);
		gameChat->addLine("You successfully empowered the item.", ChatDefines::Channels::System);
	}
	else
	{
		sContentMgr->playSound(SfxId::AlertSoulstoneEmpty);
		world->myself()->playAnimation(UnitDefines::AnimId::Hit);
		gameChat->addLine("Your attempt to empower the item was unsuccessful.", ChatDefines::Channels::System);
	}
}

void Game::processPacket_Server_SocketResult(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr || world->myself() == nullptr)
	{
		sApplication->logout();
		return;
	}
	
	auto gameChat = dynamic_pointer_cast<GameChat>(world->getRenderObject(World::GameChatBox));

	GP_Server_SocketResult pk;
	pk.unpack(data);

	if (pk.m_success)
	{
		sContentMgr->playSound(SfxId::SysGatheringSuccess);
		world->myself()->playAnimation(UnitDefines::AnimId::Cast);
		gameChat->addLine("You successfully filled a socket.", ChatDefines::Channels::System);
	}
	else
	{
		sContentMgr->playSound(SfxId::AlertSoulstoneEmpty);
		world->myself()->playAnimation(UnitDefines::AnimId::Hit);
		gameChat->addLine("Your attempt to fill a socket was unsuccessful.", ChatDefines::Channels::System);
	}
}

void Game::processPacket_Server_DestroyObject(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_DestroyObject pk;
	pk.unpack(data);

	world->removeClientObject(pk.m_guid);
}

void Game::processPacket_Server_ChatMsg(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	auto gameChat = dynamic_pointer_cast<GameChat>(world->getRenderObject(World::GameChatBox));

	GP_Server_ChatMsg pk;
	pk.unpack(data);

	if (pk.m_channelId == ChatDefines::Channels::AllChat && gameChat->isMuteAllChat())
		return;

	gameChat->recvMsg(pk.m_text, pk.m_fromName, static_cast<ChatDefines::Channels>(pk.m_channelId), pk.m_itemId.m_itemId > 0 ? &pk.m_itemId : nullptr);
	
	if (pk.m_channelId == ChatDefines::Channels::Say ||
		pk.m_channelId == ChatDefines::Channels::Party ||
		pk.m_channelId == ChatDefines::Channels::Yell)
	{
		if (auto unit = dynamic_pointer_cast<ClientUnit>(world->getClientObject(pk.m_fromGuid)))
			unit->chatMsg(pk.m_text, static_cast<ChatDefines::Channels>(pk.m_channelId));
	}

	if (pk.m_channelId == ChatDefines::Channels::SystemCenter)
		world->pushScrollingMessage(pk.m_text, GameChat::getChatColor(ChatDefines::Channels::SystemCenter));

	static time_t lastWhisperSound = 0;

	if (sApplication->timeNowMs() - lastWhisperSound > 120000)
	{
		if (pk.m_channelId == ChatDefines::Channels::Whisper && pk.m_fromGuid != world->getMyselfGuid())
		{
			sContentMgr->playSound(SfxId::AlertAuctionComplete, 0.75f);
			lastWhisperSound = sApplication->timeNowMs();
		}
	}
}

void Game::processPacket_Server_Npc(StlBuffer& data)
{
 	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_Npc pk;
	pk.unpack(data);

	auto npc = make_shared<ClientNpc>(pk.m_guid, &world->getMap());
	npc->setOrientation(pk.m_orientation);
	npc->setEntry(pk.m_entry);
	npc->setWorldPosition({ pk.m_x, pk.m_y });

	for (auto& itr : pk.m_variables)
		npc->setVariable(static_cast<ObjDefines::Variable>(itr.first), itr.second);

	if (!npc->isAlive())
		npc->finishAnimation();

	world->addClientObject(npc);
}

void Game::processPacket_Server_GameObject(StlBuffer& data)
{
 	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_GameObject pk;
	pk.unpack(data);

	auto obj = make_shared<ClientGameObj>(pk.m_guid, pk.m_entry, &world->getMap());
	obj->setWorldPosition({ pk.m_x, pk.m_y });

	for (auto& itr : pk.m_variables)
		obj->setVariable(static_cast<ObjDefines::Variable>(itr.first), itr.second);

	world->addClientObject(obj);
}

void Game::processPacket_Server_CharaCreateResult(StlBuffer& data)
{
	GP_Server_CharaCreateResult pk;
	pk.unpack(data);

	if (auto creation = dynamic_pointer_cast<CharacterCreation>(getRenderObject(RoCharacterCreation)))
	{
		sApplication->clearPopups();
		sApplication->spawnPopup(CharacterFunctions::nameErrorStr(CharacterDefines::NameError(pk.m_result)), ConfirmMessageBox::ConfirmBox_Ok, ConfirmMessageBox::ConfirmCode_Null);
	}
	else
	{
		// Not char-creation context => this is the enter-world op76 that precedes Server_NewWorld(77).
		// Its u8 is the player's map id (1=fanadin, 2=wolf_cave_1, ...); stash it for setMap in NewWorld.
		World::s_pendingSelf.mapId = static_cast<int>(pk.m_result);
	}
}

void Game::processPacket_Server_QuestList(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}
	
	GP_Server_QuestList pk;
	pk.unpack(data);
		
	auto questLog = dynamic_pointer_cast<QuestLog>(world->getRenderObject(World::Interface::QuestLogPanel));
	questLog->clear();

	for (auto& questItr : pk.m_quests)
	{
		questLog->addQuest(questItr.id);
		questLog->setQuestDone(questItr.id, questItr.done);
		
		for (auto& itr : questItr.tallyItems)
			questLog->notifyGainedItem(questItr.id, itr.first, itr.second, true);
		
		for (auto& itr : questItr.tallyNpcs)
			questLog->notifyKilledNpc(questItr.id, itr.first, itr.second, true);
		
		for (auto& itr : questItr.tallyGameObjects)
			questLog->notifyUsedObject(questItr.id, itr.first, itr.second, true);
		
		for (auto& itr : questItr.tallySpells)
			questLog->notifyCastedSpell(questItr.id, itr.first, itr.second, true);
	}
	
	questLog->questHelper_RevealDoneQuests();
	questLog->questHelper_QueueReveal();

	world->queueObjectivesRecalc();

	sLua->fire(LuaEvents::QUEST_LOG_UPDATE, "");   // Lua addon layer: full quest list (re)loaded
}

void Game::processPacket_Server_InspectReveal(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}
	
	GP_Server_InspectReveal pk;
	pk.unpack(data);
}

void Game::processPacket_Server_AvailableWorldQuests(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}
	
	GP_Server_AvailableWorldQuests pk;
	pk.unpack(data);

	auto questHelper = dynamic_pointer_cast<MapQuester>(world->getRenderObject(World::MapQuesterPanel));
	questHelper->setAvailableQuests(pk.m_list);
}

void Game::processPacket_Server_PkNotify(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_PkNotify pk;
	pk.unpack(data);

	auto gameChat = dynamic_pointer_cast<GameChat>(world->getRenderObject(World::GameChatBox));

	gameChat->addLine("You have killed " + pk.m_playerName + ".", ChatDefines::Channels::System);

	sf::Vector2i pos1(sApplication->centerOfScreen().x - 50, sApplication->centerOfScreen().y);
	sf::Vector2i pos2(sApplication->centerOfScreen().x + 50, sApplication->centerOfScreen().y);
	sf::Vector2i pos3(sApplication->centerOfScreen());

	world->pushCombatMessage("PK: " + pk.m_playerName, Util::randomChoice(pos1, pos2, pos3), sf::Color(141, 0, 135, 255), false, 0.75f);
	sContentMgr->playSound(SfxId::AlertIncreased);
}

void Game::processPacket_Server_ExpNotify(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_ExpNotify pk;
	pk.unpack(data);

	auto gameChat = dynamic_pointer_cast<GameChat>(world->getRenderObject(World::GameChatBox));
	
	if (pk.m_amount > 1)
		gameChat->addLine(Util::fmtStr("You gained %d experience points.", pk.m_amount), ChatDefines::Channels::ExpPurple);
	else if (pk.m_amount > 0)
		gameChat->addLine(Util::fmtStr("You gained %d experience point.", pk.m_amount), ChatDefines::Channels::ExpPurple);

	if (pk.m_amount > 0)
	{
		sf::Vector2i pos1(sApplication->centerOfScreen().x - 50, sApplication->centerOfScreen().y);
		sf::Vector2i pos2(sApplication->centerOfScreen().x + 50, sApplication->centerOfScreen().y);
		sf::Vector2i pos3(sApplication->centerOfScreen());

		world->pushCombatMessage("XP: " + to_string(pk.m_amount), Util::randomChoice(pos1, pos2, pos3), sf::Color(141, 0, 135, 255), false, 0.75f);
	}

	if (pk.m_newLevel > 0)
	{
		world->onLevelTo(pk.m_newLevel);

		if (pk.m_newLevel >= 3)
			world->exclaimHint(World::Hint::SpendExp);

		string rankName = sContentMgr->db("player_exp_levels").data(pk.m_newLevel, "name");
		gameChat->addLine(Util::fmtStr("You've been promoted to %s!", rankName.c_str()), ChatDefines::Channels::System);
		gameChat->addLine("Experience points are available to spend on stats and spells.", ChatDefines::Channels::System);

		sContentMgr->queueSound(SfxId::AlertLevelup, 100);
		sContentMgr->queueSound(SfxId::AlertMissionReward, 400);
	}

	sLua->fire(LuaEvents::PLAYER_XP_UPDATE, "");
}

void Game::processPacket_Server_SpentGold(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_SpentGold pk;
	pk.unpack(data);

	sContentMgr->queueSound(getItemSound(ItemDefines::StaticItems::GoldItem), 100);
	auto gameChat = dynamic_pointer_cast<GameChat>(world->getRenderObject(World::GameChatBox));
	gameChat->addLine(Util::fmtStr("You spent %d Gold", pk.m_count), ChatDefines::Channels::System);
}

void Game::processPacket_Server_AbandonQuest(StlBuffer& data)
{	
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}
	
	GP_Server_AbandonQuest pk;
	pk.unpack(data);

	sContentMgr->playSound(SfxId::AlertQuestDelete);

	string questName = sContentMgr->db("quest_template").data(pk.m_questId, "name");

	if (questName.empty())
		questName = "Unknown";
	
	auto gameChat = dynamic_pointer_cast<GameChat>(world->getRenderObject(World::GameChatBox));
	gameChat->addLine(Util::fmtStr("Abandonded quest: %s.", questName.c_str()), ChatDefines::Channels::System);
}

void Game::processPacket_Server_RewardedQuest(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_RewardedQuest pk;
	pk.unpack(data);

	sContentMgr->playSound(SfxId::AlertQuestReward);

	string questName = sContentMgr->db("quest_template").data(pk.m_questId, "name");

	if (questName.empty())
		questName = "Unknown";
	
	auto gameChat = dynamic_pointer_cast<GameChat>(world->getRenderObject(World::GameChatBox));
	gameChat->addLine(Util::fmtStr("%s completed.", questName.c_str()), ChatDefines::Channels::System);
}

void Game::processPacket_Server_QuestComplete(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}
	
	GP_Server_QuestComplete pk;
	pk.unpack(data);
	
	auto questLog = dynamic_pointer_cast<QuestLog>(world->getRenderObject(World::Interface::QuestLogPanel));
	questLog->setQuestDone(pk.m_questId, pk.m_done);
	questLog->questHelper_RevealDoneQuests();
	sLua->fire(LuaEvents::QUEST_LOG_UPDATE, "");   // Lua addon layer: quest done-state changed

	if (pk.m_done)
		world->pushScrollingMessage("Quest Complete", sf::Color(QuestLog::Colors::ObjectiveTextColor));

	world->queueObjectivesRecalc();
}

void Game::processPacket_Server_ArenaQueued(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_ArenaQueued pk;
	pk.unpack(data);

	if (pk.m_joined)
		sContentMgr->playSound(SfxId::ChunCommandCenterEdit);
	else
		sContentMgr->playSound(SfxId::AlertDeleteItem);
}

void Game::processPacket_Server_ArenaStatus(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_ArenaStatus pk;
	pk.unpack(data);

	if (pk.m_hasBegun)
		sContentMgr->playSound(SfxId::StartArena);
	else
		sContentMgr->playSound(SfxId::EnterArena);
}

void Game::processPacket_Server_ArenaOutcome(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_ArenaOutcome pk;
	pk.unpack(data);

	if (pk.m_won)
		sContentMgr->playSound(SfxId::AlertScoreVictory);
	else
		sContentMgr->playSound(SfxId::AlertTimerFail);
}

void Game::processPacket_Server_ArenaReady(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	sApplication->spawnPopup("Your arena match is ready. You must enter or forfeit.", ConfirmMessageBox::ConfirmBox_Ok, ConfirmMessageBox::ConfirmCode_ArenaOffer);
}

void Game::processPacket_Server_QuestTally(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}
	
	auto questLog = dynamic_pointer_cast<QuestLog>(world->getRenderObject(World::Interface::QuestLogPanel));

	GP_Server_QuestTally pk;
	pk.unpack(data);

	switch (pk.m_type)
	{
		case QuestDefines::TallyNpc: questLog->notifyKilledNpc(pk.m_questId, pk.m_entry, pk.m_tally, false); break;
		case QuestDefines::TallyItem: questLog->notifyGainedItem(pk.m_questId, pk.m_entry, pk.m_tally, false); break;
		case QuestDefines::TallyGameObj: questLog->notifyUsedObject(pk.m_questId, pk.m_entry, pk.m_tally, false); break;
		case QuestDefines::TallySpell: questLog->notifyCastedSpell(pk.m_questId, pk.m_entry, pk.m_tally, false); break;
	}

	world->queueObjectivesRecalc();

	sLua->fire(LuaEvents::QUEST_OBJECTIVE_UPDATE, "");   // Lua addon layer: an objective tally changed
}

void Game::processPacket_Server_AcceptedQuest(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}
	
	auto questLog = dynamic_pointer_cast<QuestLog>(world->getRenderObject(World::Interface::QuestLogPanel));
	auto gameChat = dynamic_pointer_cast<GameChat>(world->getRenderObject(World::GameChatBox));

	GP_Server_AcceptedQuest pk;
	pk.unpack(data);

	sContentMgr->playSound(SfxId::AlertQuestGet);

	string questName = sContentMgr->db("quest_template").data(pk.m_questId, "name");

	if (questName.empty())
		questName = "Unknown";

	gameChat->addLine(Util::fmtStr("Quest accepted: %s", questName.c_str()), ChatDefines::Channels::System);
	questLog->trackQuest(pk.m_questId);
	questLog->saveTrackedQuests();

	sLua->fire(LuaEvents::QUEST_LOG_UPDATE, "");   // Lua addon layer: quest accepted / auto-tracked
}

void Game::processPacket_Server_GossipMenu(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_GossipMenu pk;
	pk.unpack(data);

	world->closePanel(World::Interface::MapQuesterPanel);
	world->setSelectedGuid(pk.m_targetGuid);
	world->setGossipGuid(pk.m_targetGuid);

	if (pk.m_questOffers.size() == 1 && pk.m_gossipOptions.empty() && pk.m_questCompletes.empty() && pk.m_vendorItems.empty() && pk.m_gossipEntry == 0)
	{				
		// Just the quest offer
		auto questOffer = dynamic_pointer_cast<QuestOffer>(world->getRenderObject(World::Interface::QuestOfferPanel));
		questOffer->setQuestGiverGuid(pk.m_targetGuid);
		questOffer->setForQuest(pk.m_questOffers[0]);
		world->switchDialogPanel(World::Interface::QuestOfferPanel);
	}
	else if (pk.m_questCompletes.size() == 1 && pk.m_gossipOptions.empty() && pk.m_questOffers.empty() && pk.m_vendorItems.empty() && pk.m_gossipEntry == 0)
	{		
		// Just the quest completion
		auto questComplete = dynamic_pointer_cast<QuestComplete>(world->getRenderObject(World::Interface::QuestCompletePanel));
		questComplete->setQuestGiverGuid(pk.m_targetGuid);
		questComplete->setForQuest(pk.m_questCompletes[0]);
		world->switchDialogPanel(World::Interface::QuestCompletePanel);
	}
	else if (!pk.m_vendorItems.empty() && pk.m_questCompletes.empty() && pk.m_gossipOptions.empty() && pk.m_questOffers.empty() && pk.m_gossipEntry == 0)
	{
		// Just the vendor
		world->switchDialogPanel(World::Interface::VendorNpcPanel);
		world->openPanel(World::Interface::InventoryPanel, false);
	}
	else
	{
		auto dialogNpc = dynamic_pointer_cast<DialogNpc>(world->getRenderObject(World::Interface::DialogNpcPanel));

		if (auto npc = world->getClientObject(pk.m_targetGuid))
		{
			// Not every gossip will have options, clear when starting anew
			dialogNpc->clearGossip();

			// Quest turn in
			for (auto& itr : pk.m_questCompletes)
				dialogNpc->addGossip_QuestComplete(itr);

			// Quest accept
			for (auto& itr : pk.m_questOffers)
				dialogNpc->addGossip_QuestAccept(itr);

			// Gossip options
			for (auto& itr : pk.m_gossipOptions)
				dialogNpc->addGossip_Dialog(itr);

			// Vendor option
			if (!pk.m_vendorItems.empty())
				dialogNpc->addVendorButton();

			dialogNpc->applyText(pk.m_gossipEntry, npc->getName());
			world->switchDialogPanel(World::Interface::DialogNpcPanel);
		}
		else
		{
			world->error(PlayerDefines::WorldError::InvalidTarget);
		}
	}
		
	// May as well prepare the gui object now for easy open later
	if (!pk.m_vendorItems.empty())
	{
		auto inventory = dynamic_pointer_cast<Inventory>(world->getRenderObject(World::Interface::InventoryPanel));
		auto vendorNpc = dynamic_pointer_cast<VendorNpc>(world->getRenderObject(World::Interface::VendorNpcPanel));

		vendorNpc->reset();
		vendorNpc->setLocalMoney(inventory->getMoney());

		for (auto& itr : pk.m_vendorItems)
			vendorNpc->registerItem(itr.m_itemId, itr.m_cost, itr.m_supply);

		sLua->fire(LuaEvents::VENDOR_UPDATE, "");   // Lua addon layer: vendor stock (re)populated
	}
}
		
void Game::processPacket_Server_QueryWaypointsResponse(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_QueryWaypointsResponse pk;
	pk.unpack(data);

	world->closePanel(World::Interface::MapQuesterPanel, false);
	world->openPanel(World::Interface::MapQuesterPanel, false);
	
	sContentMgr->playSound(SfxId::SocialCritical2);

	auto mapQuester = dynamic_pointer_cast<MapQuester>(world->getRenderObject(World::Interface::MapQuesterPanel));
	mapQuester->startWaypointSelection(pk.m_guids);
}
	
void Game::processPacket_Server_MarkNpcsOnMap(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_MarkNpcsOnMap pk;
	pk.unpack(data);

	world->closePanels();
	world->openPanel(World::Interface::MapQuesterPanel);

	auto questHelper = dynamic_pointer_cast<MapQuester>(world->getRenderObject(World::MapQuesterPanel));
	questHelper->queueRevealNpcs(pk.m_npcs);
}

void Game::processPacket_Server_OnObjectWasLooted(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_OnObjectWasLooted pk;
	pk.unpack(data);

	// We already know about this
	if (pk.m_looterGuid == world->getMyselfGuid())
		return;
	
	auto lootWindow = dynamic_pointer_cast<LootWindow>(world->getRenderObject(World::LootWindowPanel));

	if (!lootWindow->isHidden() && lootWindow->getSourceGuid() == pk.m_guid)
	{
		if (auto obj = world->getClientObject(pk.m_guid))
		{
			// Request new view of the loot
			GP_Client_CastSpell packet;
			packet.m_targetGuid = pk.m_guid;
			packet.m_spellId = obj->getType() == MutualObject::Type::GameObject ? SpellDefines::StaticSpells::LootGameObj : SpellDefines::StaticSpells::LootUnit;
			sConnector->sendPacket(packet.build(StlBuffer{}));
		}
	}
}

void Game::processPacket_Server_TradeUpdate(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));
	
	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}
	
	GP_Server_TradeUpdate incoming;
	incoming.unpack(data);
	
	auto tradeWindow = dynamic_pointer_cast<TradeWindow>(world->getRenderObject(World::TradeWindowPanel));

	if (tradeWindow == nullptr)
		return;

	// Clear and refill
	tradeWindow->clearLocalItems();
	tradeWindow->clearRemoteItems();

	int slot = 0;
	for (auto& [itemGuid, items] : incoming.m_myItems)
	{
		for (auto& [itemDef, stackSize] : items)
			tradeWindow->addLocalItem(itemDef, itemGuid, stackSize, slot++);
	}

	slot = 0;
	for (auto& [itemGuid, items] : incoming.m_theirItems)
	{
		for (auto& [itemDef, stackSize] : items)
			tradeWindow->addRemoteItem(itemDef, stackSize, slot++);
	}

	tradeWindow->setLocalGold(incoming.m_myMoney);
	tradeWindow->setRemoteGold(incoming.m_hisMoney);

	// Update ready states
	tradeWindow->setLocalReady(incoming.m_myReady);
	tradeWindow->setRemoteReady(incoming.m_theirReady);
	
	// Set players
	if (auto remotePlayer = dynamic_pointer_cast<ClientPlayer>(world->getClientObject(incoming.m_partnerGuid)))
		tradeWindow->setRemotePlayer(remotePlayer);
	
	if (auto localPlayer = dynamic_pointer_cast<ClientPlayer>(world->getClientObject(world->getMyselfGuid())))
		tradeWindow->setLocalPlayer(localPlayer);
	
	if (!world->isPanelOpen(World::TradeWindowPanel))
		world->openPanel(World::TradeWindowPanel);
}

void Game::processPacket_Server_TradeCanceled(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_TradeCanceled incoming;
	incoming.unpack(data);

	auto tradeWindow = dynamic_pointer_cast<TradeWindow>(world->getRenderObject(World::TradeWindowPanel));

	if (tradeWindow != nullptr)
		tradeWindow->reset();

	world->closePanel(World::TradeWindowPanel);
}

void Game::processPacket_Server_PartyList(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_PartyList pk;
	pk.unpack(data);
	
	world->setParty(pk.m_members, pk.m_leaderGuid);
}
		
void Game::processPacket_Server_OfferParty(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_OfferParty pk;
	pk.unpack(data);
	
	sApplication->spawnPopup(Util::fmtStr("%s group invited you. Accept?", pk.m_inviterName.c_str()), ConfirmMessageBox::ConfirmBox_YesNo, ConfirmMessageBox::ConfirmCode_PartyOffer);
}
		
void Game::processPacket_Server_DiscoverWaypointNotify(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_DiscoverWaypointNotify pk;
	pk.unpack(data);

	sContentMgr->playSound(SfxId::SocialCombo);
	sApplication->spawnPopup("You've discovered a new waypoint!", ConfirmMessageBox::ConfirmBoxType::ConfirmBox_Ok, 0);
}

void Game::processPacket_Server_RepairCost(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_RepairCost pk;
	pk.unpack(data);

	if (pk.m_finished)
	{
		sContentMgr->playSound(SfxId::BlockMetalVar01);
		sContentMgr->queueSound(getItemSound(ItemDefines::StaticItems::GoldItem), 100);
		return;
	}

	if (pk.m_amount <= 0)
		return;

	sApplication->spawnPopup(Util::fmtStr("Repair costs %sg. Accept?", Util::formatMoneyCommas(pk.m_amount).c_str()), ConfirmMessageBox::ConfirmBox_YesNo, ConfirmMessageBox::ConfirmCode_RepairConfirm);
}

void Game::processPacket_Server_UpdateVendorStock(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_UpdateVendorStock pk;
	pk.unpack(data);
	
	auto vendorNpc = dynamic_pointer_cast<VendorNpc>(world->getRenderObject(World::Interface::VendorNpcPanel));
	vendorNpc->updateItemQuantity(pk.m_itemId, pk.m_amount);

	sLua->fire(LuaEvents::VENDOR_UPDATE, "");   // Lua addon layer: a vendor item's stock changed
}
		
void Game::processPacket_Server_Spellbook_Update(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_Spellbook_Update pk;
	pk.unpack(data);
	
	auto abilities = dynamic_pointer_cast<Abilities>(world->getRenderObject(World::Interface::AbilitiesPanel));
	abilities->updateData(pk.m_spellId, pk.m_level, pk.m_bpoints);
	sLua->fire(LuaEvents::SPELLBOOK_UPDATE, "");   // Lua addon layer: a single spell rank changed

	world->refreshToolbarTooltips();

	// For Tomes
	auto inventory = dynamic_pointer_cast<Inventory>(world->getRenderObject(World::Interface::InventoryPanel));
	inventory->refreshTooltips();
}

void Game::processPacket_Server_Spellbook(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}
		
	GP_Server_Spellbook pk;
	pk.unpack(data);

	auto abilities = dynamic_pointer_cast<Abilities>(world->getRenderObject(World::Interface::AbilitiesPanel));
	abilities->setData(pk.m_slots);
	sLua->fire(LuaEvents::SPELLBOOK_UPDATE, "");   // Lua addon layer: full spellbook (re)loaded

	if (world->isEmptyToolbars())
	{
		auto mainToolbar = dynamic_pointer_cast<Toolbar>(world->getRenderObject(World::Interface::Toolbar1));

		for (int i = 0, toolbarSlot = Toolbar::Interface::GameIcon1; i < (int)pk.m_slots.size() && toolbarSlot <= Toolbar::Interface::GameIcon12; ++i)
		{
			if (SpellFunctions::isStaticSpell(pk.m_slots[i].spellId))
				continue;

			while (mainToolbar->getIconEntry(static_cast<Toolbar::Interface>(toolbarSlot)) != 0 && toolbarSlot < Toolbar::Interface::GameIcon12)
				++toolbarSlot;

			if (mainToolbar->getIconEntry(static_cast<Toolbar::Interface>(toolbarSlot)) == 0)
				mainToolbar->createBaseIcon(static_cast<Toolbar::Interface>(toolbarSlot), GameIcon::Type::Spell, pk.m_slots[i].spellId);
		}
	}
}

void Game::processPacket_Server_OpenBank(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	world->closePanel(World::DialogNpcPanel);
	world->closePanel(World::VendorNpcPanel);
	world->closePanel(World::TradeWindowPanel);
	world->openPanel(World::Interface::BankPanel);
}

void Game::processPacket_Server_Bank(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	auto bank = dynamic_pointer_cast<Bank>(world->getRenderObject(World::Interface::BankPanel));

	GP_Server_Bank pk;
	pk.unpack(data);

	bank->clearDrakenedSlot();

	int toolbarSlot = Toolbar::Interface::GameIcon12;
	auto mainToolbar = dynamic_pointer_cast<Toolbar>(world->getRenderObject(World::Interface::Toolbar1));

	for (size_t i = 0; i < pk.m_slots.size(); ++i)
	{
		const auto& slot = pk.m_slots[i];
		auto icon = dynamic_pointer_cast<ItemIcon>(bank->getRenderObject(Bank::Interface::Slot1 + i));
		icon->setItemDef(slot.itemId);
		icon->setStackCount(slot.stackCount);
	}

	sLua->fire(LuaEvents::BANK_UPDATE, "");   // Lua addon layer: bank (re)populated -> refresh the Lua view
}

void Game::processPacket_Server_Inventory(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}
	
	auto inventory = dynamic_pointer_cast<Inventory>(world->getRenderObject(World::Interface::InventoryPanel));

	GP_Server_Inventory pk;
	pk.unpack(data);

	inventory->clearDrakenedSlot();

	int toolbarSlot = Toolbar::Interface::GameIcon12;
	auto mainToolbar = dynamic_pointer_cast<Toolbar>(world->getRenderObject(World::Interface::Toolbar1));

	for (size_t i = 0; i < pk.m_slots.size(); ++i)
	{
		const auto& slot = pk.m_slots[i];
		auto icon = dynamic_pointer_cast<ItemIcon>(inventory->getRenderObject(Inventory::Interface::Slot1 + slot.slot));
		if (icon == nullptr)
			continue;
		icon->setItemDef(slot.itemId);
		icon->setStackCount(slot.stackCount);

		// Consuable items get put onto hotbar, only a few though
		if (world->isEmptyToolbars() && icon->getCastedSpellId())
		{
			if (toolbarSlot >= Toolbar::Interface::GameIcon7)
			{
				while (mainToolbar->getIconEntry(static_cast<Toolbar::Interface>(toolbarSlot)) != 0 && toolbarSlot > Toolbar::Interface::GameIcon7)
					--toolbarSlot;

				if (mainToolbar->getIconEntry(static_cast<Toolbar::Interface>(toolbarSlot)) == 0)
					mainToolbar->createBaseIcon(static_cast<Toolbar::Interface>(toolbarSlot), GameIcon::Type::Item, icon->getEntry());
			}
		}
	}

	// Inventory comes after spellbook
	if (world->popEmptyToolbars())
		mainToolbar->saveCache();

	sLua->fire(LuaEvents::BAG_UPDATE, "");   // Lua addon layer: bags refreshed
}

void Game::processPacket_Server_EquipItem(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_EquipItem pk;
	pk.unpack(data);

	if (auto player = dynamic_pointer_cast<ClientPlayer>(world->getClientObject(pk.m_guid)))
	{
		player->setEquipment(static_cast<UnitDefines::EquipSlot>(pk.m_slot), pk.m_itemId);

		if (world->getMyselfGuid() == pk.m_guid)
		{
			auto equipment = dynamic_pointer_cast<Equipment>(world->getRenderObject(World::Interface::EquipmentPanel));
			equipment->updatePlayer(player);

			if (!pk.m_silent)
				playItemSound(pk.m_itemId.m_itemId);

			sLua->fire(LuaEvents::PLAYER_EQUIPMENT_CHANGED, "");   // Lua addon layer: local player gear changed
		}
	}
	else
	{
		// Might not be a problem, maybe delays due to latency?
	}
}

void Game::processPacket_Server_ChatError(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_ChatError pk;
	pk.unpack(data);

	world->chatError(PlayerDefines::ChatError(pk.m_code));
}
	
void Game::processPacket_Server_UnlockGameObj(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_UnlockGameObj pk;
	pk.unpack(data);
			
	int model = atoi(sContentMgr->db("gameobject_template").data(pk.m_objEntry, "model").c_str());
	sContentMgr->playSound(sContentMgr->db("gameobject_models").data(model, "sound_unlocked"));
}

void Game::processPacket_Server_GuildOnlineStatus(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	auto gameChat = dynamic_pointer_cast<GameChat>(world->getRenderObject(World::GameChatBox));

	GP_Server_GuildOnlineStatus pk;
	pk.unpack(data);

	if (pk.m_online)
	{
		gameChat->addLine(pk.m_playerName + " has come online.", ChatDefines::Channels::System);
		sContentMgr->playSound(SfxId::AlertRollover);
	}
	else
	{
		gameChat->addLine(pk.m_playerName + " has logged off.", ChatDefines::Channels::System);
	}
}

void Game::processPacket_Server_ChannelChangeConfirm(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_ChannelChangeConfirm pk;
	pk.unpack(data);

	if (pk.m_success)
		sApplication->spawnPopup("You've changed channels on the server.", ConfirmMessageBox::ConfirmBox_Ok, ConfirmMessageBox::ConfirmBox_Ok);
	else
		sApplication->spawnPopup("Unable to change to that channel.", ConfirmMessageBox::ConfirmBox_Ok, ConfirmMessageBox::ConfirmBox_Ok);
}

void Game::processPacket_Server_PromptRespec(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));
	
	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_PromptRespec pk;
	pk.unpack(data);
	
	string cost = Util::formatMoneyCommas(pk.m_cost);

	sApplication->spawnPopup("Spend " + cost + "g to respec?", ConfirmMessageBox::ConfirmBox_YesNo, ConfirmMessageBox::ConfirmCode_RespecPrompt);
}

void Game::processPacket_Server_GuildNotifyRoleChange(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	auto gameChat = dynamic_pointer_cast<GameChat>(world->getRenderObject(World::GameChatBox));

	GP_Server_GuildNotifyRoleChange pk;
	pk.unpack(data);

	const std::string rankNameStr = GuildFunctions::rankName((GuildDefines::Rank)pk.m_role);
	gameChat->addLine(pk.m_playerName + "'s rank in the guild is now " + rankNameStr + ".", ChatDefines::Channels::Guild);
	
	// Request an update of it
	if (world->isPanelOpen(World::Interface::GuildPanel))
		sConnector->sendPacket(GP_Client_GuildRosterRequest{}.build(StlBuffer{}));
}

void Game::processPacket_Server_LearnedSpell(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}
	
	auto gameChat = dynamic_pointer_cast<GameChat>(world->getRenderObject(World::GameChatBox));
	auto mainToolbar = dynamic_pointer_cast<Toolbar>(world->getRenderObject(World::Interface::Toolbar1));

	GP_Server_LearnedSpell pk;
	pk.unpack(data);
	
	string spellname = sContentMgr->db("spell_template").data(pk.m_spellId, "name");
	gameChat->addLine("You learned a new ability: " + spellname + ".", ChatDefines::Channels::System);

	// Add to main toolbar at empty slot
	for (int toolbarSlot = Toolbar::Interface::GameIcon1; toolbarSlot <= Toolbar::Interface::GameIcon12; ++toolbarSlot)
	{
		if (mainToolbar->getIconEntry(static_cast<Toolbar::Interface>(toolbarSlot)) == 0)
		{
			mainToolbar->createBaseIcon(static_cast<Toolbar::Interface>(toolbarSlot), GameIcon::Type::Spell, pk.m_spellId);
			mainToolbar->saveCache();
			break;
		}
	}
}
		
void Game::processPacket_Server_AggroMob(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_AggroMob pk;
	pk.unpack(data);

	auto unit = dynamic_pointer_cast<ClientUnit>(world->getClientObject(pk.m_guid));

	if (unit == nullptr)
		return;
	
	if (world->getSelectedGuid() == 0)
		world->setSelectedGuid(pk.m_guid);

	if (!unit->playSoundAggro() && !unit->playSoundAttack())
		unit->playSoundDamaged();
}

void Game::processPacket_Server_LvlResponse(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_LvlResponse pk;
	pk.unpack(data);

	auto equipment = dynamic_pointer_cast<Equipment>(world->getRenderObject(World::Interface::EquipmentPanel));

	if (pk.m_bool)
	{
		auto inventory = dynamic_pointer_cast<Inventory>(world->getRenderObject(World::Interface::InventoryPanel));
		inventory->refreshTooltips();
		equipment->toggleSpendingPoints(false);
		dynamic_pointer_cast<Button>(equipment->getRenderObject(Equipment::Interface::LevelUpExclaim))->setHidden(true);
		dynamic_pointer_cast<Button>(equipment->getRenderObject(Equipment::Interface::LevelUp))->setExclaimNotice(false);
		dynamic_pointer_cast<Button>(world->getRenderObject(World::Interface::EquipmentButton))->setExclaimNotice(false);

		sContentMgr->playSound(SfxId::AlertDp50);
	}
	else
	{
		equipment->clearPendingServerSend();
	}

	// Lua addon layer: spend resolved — "1" lets the Lua Equipment leave its spend UI, "0" keeps it (rejected).
	sLua->fire(LuaEvents::LEVELUP_RESULT, pk.m_bool ? "1" : "0");
}

void Game::processPacket_Server_OfferDuel(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_OfferDuel pk;
	pk.unpack(data);
	
	sApplication->spawnPopup(Util::fmtStr("%s wants to duel. Accept?", pk.m_challengerName.c_str()), ConfirmMessageBox::ConfirmBox_YesNo, ConfirmMessageBox::ConfirmCode_DuelOffer);
}

void Game::processPacket_Server_WorldError(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_WorldError pk;
	pk.unpack(data);

	world->error(PlayerDefines::WorldError(pk.m_code));
}

void Game::processPacket_Server_ObjectVariable(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_ObjectVariable pk;
	pk.unpack(data);

	if (auto object = world->getClientObject(pk.m_guid))
	{
		object->setVariable(static_cast<ObjDefines::Variable>(pk.m_variableId), pk.m_value);

		// Lua addon layer: fire the matching unit event for player/target variable changes.
		const bool isPlayer = world->myself() && pk.m_guid == world->myself()->getGuid();
		const char* token = isPlayer ? "player" : (pk.m_guid == world->getSelectedGuid() ? "target" : nullptr);

		if (token)
		{
			const int maxPowerVar = static_cast<int>(ObjDefines::Variable::StatsStart) + static_cast<int>(UnitDefines::Stat::Mana);
			const char* ev = nullptr;
			switch (static_cast<ObjDefines::Variable>(pk.m_variableId))
			{
				case ObjDefines::Variable::Health:    ev = LuaEvents::UNIT_HEALTH;    break;
				case ObjDefines::Variable::MaxHealth: ev = LuaEvents::UNIT_MAXHEALTH; break;
				case ObjDefines::Variable::Mana:      ev = LuaEvents::UNIT_POWER;     break;
				case ObjDefines::Variable::Level:     ev = LuaEvents::UNIT_LEVEL;     break;
				default: if (pk.m_variableId == maxPowerVar) ev = LuaEvents::UNIT_MAXPOWER; break;
			}
			if (ev)
				sLua->fire(ev, token);
		}

		// XP progression (local player only).
		if (isPlayer && static_cast<ObjDefines::Variable>(pk.m_variableId) == ObjDefines::Variable::Progression)
			sLua->fire(LuaEvents::PLAYER_XP_UPDATE, "");

		// Money (local player only).
		if (isPlayer && static_cast<ObjDefines::Variable>(pk.m_variableId) == ObjDefines::Variable::Money)
			sLua->fire(LuaEvents::PLAYER_MONEY, "");

		// Character-sheet stats (local player): any stat variable, or the equipment-shown extras. Lets the
		// Equipment window refresh on an event instead of polling.
		if (isPlayer)
		{
			const int vid = pk.m_variableId;
			if ((vid >= ObjDefines::Variable::StatsStart && vid <= ObjDefines::Variable::StatsEnd)
			    || vid == ObjDefines::Variable::Experience || vid == ObjDefines::Variable::ArenaRating
			    || vid == ObjDefines::Variable::PkCount)
				sLua->fire(LuaEvents::PLAYER_STATS, "");
		}
	}
}

void Game::processPacket_Server_UnitSpline(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_UnitSpline pk;
	pk.unpack(data);

	auto unit = dynamic_pointer_cast<ClientUnit>(world->getClientObject(pk.m_guid));

	if (unit == nullptr)
		return;

	unit->setServerSpline(pk.m_spline);
	unit->setServerPosition({ pk.m_startX, pk.m_startY });
	
	if (pk.m_slide)
	{
		// The client is never the one to initiate these, so just sync
		unit->setWorldPosition({ pk.m_startX, pk.m_startY });
		unit->setSpline(pk.m_spline, true);
		return;
	}

	if (pk.m_silent)
		return;

	// Be lenient with local splines movement
	if (pk.m_guid == world->getMyselfGuid() && !sConfig->getCache(Options::System_CtmTick))
	{
		bool syncPosition = !unit->hasSpline();

		if (!syncPosition)
		{
			auto incomingLast = !pk.m_spline.empty() ? *(pk.m_spline.end() - 1) : Geo2d::Vector2(pk.m_startX, pk.m_startY);
			auto currentLast = unit->hasSpline() ? *(unit->spline().end() - 1) : unit->getWorldPosition();

			float startDiff = unit->getWorldPosition().getDist({ pk.m_startX, pk.m_startY });
			float endDiff = incomingLast.getDist(currentLast);

			if (startDiff >= 3.f && endDiff > 3.f)
				syncPosition = true;
		}

		if (syncPosition)
		{
			unit->setWorldPosition({ pk.m_startX, pk.m_startY });
			unit->setSpline(pk.m_spline);
		}
		else if (!pk.m_spline.empty())
		{
			// Mirror what the server likely did
			const auto& dest = *(pk.m_spline.end() - 1);
			vector<Geo2d::Vector2> newPath;

			if (MapLogic::checkLosToC(world->getMap(), unit->getWorldPosition(), dest, MapCellT::Unwalkable))
				newPath = { dest };
			else
				MapLogic::constructPathTo(world->getMap(), newPath, unit->getWorldPosition(), dest);

			float pathdistA = MapLogic::getPathLength(unit->getWorldPosition(), newPath);
			float pathdistB = MapLogic::getPathLength({ pk.m_startX, pk.m_startY }, pk.m_spline);
			float pathDiff = abs(pathdistA - pathdistB);

			if (newPath.empty())
			{
				// Our path was too different, use server's 
				unit->setWorldPosition({ pk.m_startX, pk.m_startY });
				unit->setSpline(pk.m_spline);
			}
			else
			{
				// Our path was very similar, use ours
				unit->setSpline(pk.m_spline);
			}
		}
		else
		{
			// The server has no spline, so we shouldn't either
			unit->clearSpline();
		}
	}
	else
	{
		if (unit->getWorldPosition().getDist({ pk.m_startX, pk.m_startY }) >= 0.25f)
			unit->setWorldPosition({ pk.m_startX, pk.m_startY });

		unit->setSpline(pk.m_spline);
	}
}

void Game::processPacket_Server_SetSubname(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_SetSubname pk;
	pk.unpack(data);

	if (auto unit = dynamic_pointer_cast<ClientUnit>(world->getClientObject(pk.m_objectGuid)))
		unit->setSubName(pk.m_name);
}

void Game::processPacket_Server_GuildRoster(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_GuildRoster pk;
	pk.unpack(data);

	if (auto guildRoster = dynamic_pointer_cast<GuildRoster>(world->getRenderObject(World::Interface::GuildPanel)))
	{
		// Save scroll offset before clearing
		int savedScrollOffset = 0;
		if (auto listNames = dynamic_pointer_cast<TextLines>(guildRoster->getRenderObject(GuildRoster::Interface::ListNames)))
		{
			if (auto scrollObj = listNames->getScrollObject())
				savedScrollOffset = scrollObj->getScrollOffset();
		}

		guildRoster->setGuild(pk.m_guildName);
		guildRoster->setMotd(pk.m_motd);
		guildRoster->clearMemberList();

		if (pk.m_guildName.empty())
			guildRoster->setMotd("You are not in a guild.");

		for (auto& itr : pk.m_members)
			guildRoster->addMember(itr.guid, itr.name, static_cast<PlayerDefines::Classes>(itr.classId), static_cast<GuildDefines::Rank>(itr.rank), itr.level, itr.online);

		// Restore scroll offset after repopulating
		if (auto listNames = dynamic_pointer_cast<TextLines>(guildRoster->getRenderObject(GuildRoster::Interface::ListNames)))
		{
			if (auto scrollObj = listNames->getScrollObject())
				scrollObj->setScrollOffset(savedScrollOffset);
		}
	}

	sLua->fire(LuaEvents::GUILD_ROSTER_UPDATE, "");   // Lua addon layer: roster (re)populated
}

void Game::processPacket_Server_GuildInvite(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_GuildInvite incoming;
	incoming.unpack(data);

	if (sApplication->getRenderObject(Application::RoPopupPrompt) == nullptr)
	{
		m_guildInviteId = incoming.m_guildId;
		sApplication->spawnPopup("You have been invited to join the guild " + incoming.m_guildName + ".", ConfirmMessageBox::ConfirmBox_YesNo, ConfirmMessageBox::ConfirmCode_GuildInvite);
	}

	// We're busy, just decline
	else
	{
		GP_Client_GuildInviteResponse pk;
		pk.m_guildId = incoming.m_guildId;
		pk.m_accept = false;
		sConnector->sendPacket(pk.build(StlBuffer{}));
	}
}

void Game::processPacket_Server_OpenLootWindow(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_OpenLootWindow incoming;
	incoming.unpack(data);

	auto lootWindow = dynamic_pointer_cast<LootWindow>(world->getRenderObject(World::LootWindowPanel));
	
	lootWindow->reset();
	lootWindow->setSourceGuid(incoming.m_objGuid);

	// Empty info means to just close it, and it's already closed
	if (!incoming.m_items.empty() || incoming.m_money != 0)
	{
		if (incoming.m_money > 0)
			lootWindow->addMoney(incoming.m_money);

		for (auto& itr : incoming.m_items)
			lootWindow->addItem(itr.first, itr.second);

		world->openPanel(World::LootWindowPanel);

		// Lua addon layer (WoW-style): loot is available -> the Lua view shows + (re)populates. Fires on every
		// non-empty OpenLootWindow, so it also drives the refresh after looting a single item.
		sLua->fire(LuaEvents::LOOT_READY, "");
	}
	else
	{
		world->closePanel(World::LootWindowPanel);   // closePanel fires LOOT_CLOSED
	}
}

void Game::processPacket_Server_NotifyItemAdd(StlBuffer& data)
{
	GP_Server_NotifyItemAdd incoming;
	incoming.unpack(data);

	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	auto gameChat = dynamic_pointer_cast<GameChat>(world->getRenderObject(World::GameChatBox));

	auto& st = sContentMgr->db("item_template");
	auto itemname = ItemIcon::formItemTitle(incoming.m_itemId);
	auto itemquality = atoi(st.data(incoming.m_itemId.m_itemId, "quality").c_str());
	auto itemcolor = ItemIcon::itemColor(static_cast<ItemDefines::Quality>(itemquality));

	Game::playItemSound(incoming.m_itemId.m_itemId);

	string msg = "You receive: [" + itemname + "]";

	if (!incoming.m_looterName.empty())
		msg = "[" + incoming.m_looterName + "] received: [" + itemname + "]";

	if (incoming.m_amount > 1)
		msg += " x" + to_string(incoming.m_amount);

	gameChat->addLineColor(msg, itemcolor, &incoming.m_itemId);
}

void Game::processPacket_Server_CastStop(StlBuffer& data)
{
	GP_Server_CastStop incoming;
	incoming.unpack(data);

	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	auto caster = dynamic_pointer_cast<ClientUnit>(world->getClientObject(incoming.m_guid));

	if (caster == nullptr)
		return;

	if (caster->getCastSpellId() == incoming.m_spellId)
		caster->setCastBar(nullptr, 0, 0);

	// Lua addon layer: notify the player/target cast bar.
	const char* token = caster->isLocal() ? "player"
	                  : (caster->getGuid() == world->getSelectedGuid() ? "target" : nullptr);
	if (token)
		sLua->fire(LuaEvents::UNIT_SPELLCAST_STOP, token);
}

void Game::processPacket_Server_CastStart(StlBuffer& data)
{
	GP_Server_CastStart incoming;
	incoming.unpack(data);

	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	auto caster = dynamic_pointer_cast<ClientUnit>(world->getClientObject(incoming.m_guid));

	if (caster == nullptr)
		return;

	const int flareAnimation = atoi(sContentMgr->db("spell_visual").data(incoming.m_spellId, "unit_cast_animation").c_str());

	if (flareAnimation != 0)
		caster->playAnimation(static_cast<UnitDefines::AnimId>(flareAnimation));

	shared_ptr<WorldSpellAnimation> sa = make_shared<WorldSpellAnimation>(&world->getMap(), incoming.m_spellId, true);
	sa->setSource(caster);
	sa->setTimer(incoming.m_timer);

	if (caster->isLocal() || caster->getGuid() == world->getSelectedGuid())
	{
		// Full volume for myself and my target
	}
	else
	{
		sa->setSoundPoint_Source(sf::Vector2f(caster->getScreenPosition()));
	}

	world->addWorldSpellAnimation(sa);

	if (incoming.m_timer != 0)
	{
		caster->setCastBar(sa, incoming.m_spellId, incoming.m_timer);

		// Lua addon layer: notify the player/target cast bar.
		const char* token = caster->isLocal() ? "player"
		                  : (caster->getGuid() == world->getSelectedGuid() ? "target" : nullptr);
		if (token)
			sLua->fire(LuaEvents::UNIT_SPELLCAST_START, token);
	}
}

void Game::processPacket_Server_CombatMsg(StlBuffer& data)
{
	GP_Server_CombatMsg incoming;
	incoming.unpack(data);

	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	auto victim = dynamic_pointer_cast<ClientUnit>(world->getClientObject(incoming.m_targetGuid));

	if (victim == nullptr)
		return;

	auto caster = dynamic_pointer_cast<ClientUnit>(world->getClientObject(incoming.m_casterGuid));
	const bool someoneElse = caster == nullptr || !caster->isLocal();

	// Location of combat message text
	sf::Vector2i combatMsgLoc(victim->getScreenPosition().x, victim->getScreenPosition().y - victim->mouseableHeight() - 32);		

	// Auras
	// Binary-confirmed (dump_client FUN @0x99081): the spell-NAME branch gates on the 4th trailing byte
	// (== 0 = aura-application announcement) and colors on the 3rd byte. The reconstruction's unpack labels
	// those bytes m_positive (b3) and m_auraEffect (b2). Using !m_periodic (b1) was wrong -> HoT/DoT cast
	// showed "0" and ticks showed the name; this restores the original (name on apply, number on tick).
	if ((incoming.m_spellEffect == SpellDefines::Effects::ApplyAura || incoming.m_spellEffect == SpellDefines::Effects::ApplyAreaAura) &&
		(incoming.m_spellResult == SpellDefines::HitResult::Normal || incoming.m_spellResult == SpellDefines::HitResult::Crit) &&
		!incoming.m_positive)
	{
		string spellname = sContentMgr->db("spell_template").data(incoming.m_spellId, "name").c_str();
		// Beneficial-aura applications carry b1(m_periodic)=1 (Rejuvenation/Determination); harmful=0 (Chains).
		// -> green name for buffs, red for debuffs. (Original keyed b2, which is 0 here -> always red.)
		sf::Color textColor = incoming.m_periodic ? sf::Color(87, 193, 137, 255) : sf::Color::Red;
		float scale = 0.5f;

		// Faded color when we're not the caster
		if (someoneElse)
			textColor.a /= 2;

		world->pushCombatMessage(spellname, combatMsgLoc, textColor, false, 0.5f, victim->getGuid(), false, someoneElse ? 2.f : 1.f);
		return;
	}

	// Damage or Heal
	{	
		const bool isCrit = incoming.m_spellResult == SpellDefines::HitResult::Crit;

		if (isCrit && caster != nullptr)
		{
			shared_ptr<WorldSpellAnimation> sa = make_shared<WorldSpellAnimation>(&world->getMap(), SpellDefines::CritVisual);
			sa->setSource(caster);
			sa->setTarget(victim);
			world->addWorldSpellAnimation(sa);

			if (incoming.m_casterGuid == world->getMyselfGuid() || victim->isLocal() || victim->getGuid() == world->getSelectedGuid() || incoming.m_casterGuid == world->getSelectedGuid())
			{
				// Full volume if im involved in any way
			}
			else
			{
				sa->setSoundPoint_Source(sf::Vector2f(caster ? caster->getScreenPosition() : victim->getScreenPosition()));
				sa->setSoundPoint_Target(sf::Vector2f(victim->getScreenPosition()));
			}
		}
	
		// Visual kit data isn't used for these sounds because the chosen one is dynamic
		if (incoming.m_spellEffect == SpellDefines::Effects::MeleeAtk && caster != nullptr)
		{
			caster->playSoundMelee(*victim, static_cast<SpellDefines::HitResult>(incoming.m_spellResult));
			caster->playSoundAttack();
		}

		if (!incoming.m_periodic)
		{
			if (incoming.m_amount < 0)
			{
				victim->playSoundDamaged();
				victim->flashBrighten(isCrit ? 1.f : 0.5f);
			}

			// Victim animate being struck if not in another animation
			if (victim->getAnimation() == UnitDefines::Stance)
			{
				if (incoming.m_spellResult == SpellDefines::HitResult::Block || incoming.m_spellResult == SpellDefines::HitResult::Parry)
					victim->playAnimation(UnitDefines::Block);
				else if (incoming.m_amount < 0)
					victim->playAnimation(UnitDefines::Hit);
			}
		}

		string textMsg;
		sf::Color textColor;
	
		if (incoming.m_auraEffect == SpellDefines::AuraType::PeriodicBurnMana || incoming.m_spellEffect == SpellDefines::Effects::ManaBurn || incoming.m_spellEffect == SpellDefines::Effects::ManaDrain)
			textColor = sf::Color(131, 0, 83, 255);
		else if (incoming.m_auraEffect == SpellDefines::AuraType::PeriodicRestoreMana || incoming.m_spellEffect == SpellDefines::Effects::RestoreMana || incoming.m_spellEffect == SpellDefines::Effects::RestoreManaPct)
			textColor = sf::Color(1, 94, 219, 255);
		else if (incoming.m_spellEffect == SpellDefines::Effects::MeleeAtk || incoming.m_spellEffect == SpellDefines::Effects::RangedAtk)
			textColor = sf::Color::White;
		else if (incoming.m_amount < 0)
			textColor = sf::Color::Yellow;
		else
			textColor = sf::Color::Green;	

		float decayRate = 1.f;
		float baseScale = someoneElse ? 0.5f : 1.f;
		bool floatSideways = incoming.m_periodic;
		bool pushUpward = true;

		// Solo text for Miss/Resist etc be half size, unless local is the target
		switch (incoming.m_spellResult)
		{
			case SpellDefines::HitResult::Miss: 
				textMsg = "Miss"; textColor = sf::Color::Magenta;
				baseScale /= someoneElse ? 1.f : 2.f;
				break;
			case SpellDefines::HitResult::Evade:
				textMsg = "Evade"; textColor = sf::Color::Magenta; 
				baseScale /= someoneElse ? 1.f : 2.f;
				break;
			case SpellDefines::HitResult::Dodge: 
				textMsg = "Dodge"; textColor = sf::Color::Magenta;
				baseScale /= someoneElse ? 1.f : 2.f;
				break;
			case SpellDefines::HitResult::Immune: 
				textMsg = "Immune"; textColor = sf::Color::Magenta; 
				baseScale /= someoneElse ? 1.f : 2.f;
				victim->playSound(SfxId::SkillDarkballAmmo);
				break;
			case SpellDefines::HitResult::Absorb: 
				textMsg = "Absorb"; textColor = sf::Color::Magenta; 
				baseScale /= someoneElse ? 1.f : 2.f;

				if (!incoming.m_periodic)
					victim->playSound(SfxId::SkillDarkballAmmo);
				break;
			default: 
				textMsg = to_string(abs(incoming.m_amount)); 
				break;
		}
		
		// Faded color when we're not the caster
		if (someoneElse)
			textColor.a /= 2;

		// Dots peel sideways from a little lower original
		if (incoming.m_periodic)
			combatMsgLoc.y += 32;

		// Heals I want to keep on the character model
		if (victim->isLocal() && incoming.m_amount < 1 && !incoming.m_periodic)
		{
			combatMsgLoc.x = sApplication->centerOfScreen().x;
			combatMsgLoc.y = sApplication->centerOfScreen().y + 200;
			baseScale = 1.f;
			decayRate = 1.5f;
			floatSideways = false;
			pushUpward = false;
		}
		else if (someoneElse)
		{
			decayRate = 2.f;
		}

		world->pushCombatMessage(textMsg, combatMsgLoc, textColor, isCrit, baseScale, victim->getGuid(), floatSideways, decayRate, pushUpward);

		switch (incoming.m_spellResult)
		{
			case SpellDefines::HitResult::Resist: 
			{
				uint8_t alpha = textColor.a;
				textColor = sf::Color::Magenta;
				textColor.a = alpha;
				world->pushCombatMessage("Resist", combatMsgLoc, textColor, isCrit, baseScale / 2.f, victim->getGuid(), false, decayRate, pushUpward);
				break;
			}
			case SpellDefines::HitResult::Block:
			{
				uint8_t alpha = textColor.a;
				textColor = sf::Color::Magenta;
				textColor.a = alpha;
				world->pushCombatMessage("Block", combatMsgLoc, textColor, isCrit, baseScale / 2.f, victim->getGuid(), false, decayRate, pushUpward);
				break;
			}
			case SpellDefines::HitResult::Parry:
			{
				uint8_t alpha = textColor.a;
				textColor = sf::Color::Magenta;
				textColor.a = alpha;
				world->pushCombatMessage("Parry", combatMsgLoc, textColor, isCrit, baseScale / 2.f, victim->getGuid(), false, decayRate, pushUpward);
				break;
			}
		}

		// Play message onto the unit frame only if its damage, or if its something that avoids damage entirely 
		if (incoming.m_spellResult == SpellDefines::HitResult::Miss || 
			incoming.m_spellResult == SpellDefines::HitResult::Dodge || 
			incoming.m_spellResult == SpellDefines::HitResult::Immune || 
			incoming.m_spellResult == SpellDefines::HitResult::Absorb ||
			incoming.m_amount != 0)
		{
			if (victim->isLocal())
			{
				auto localFrame = dynamic_pointer_cast<UnitFrame>(world->getRenderObject(World::PlayerUnitFrame));
				localFrame->playMessage(textMsg, textColor);
			}
			else 
			{	
				auto targetFrame = dynamic_pointer_cast<UnitFrame>(world->getRenderObject(World::TargetUnitFrame));

				if (targetFrame->getUnitPtr() != nullptr && targetFrame->getUnitPtr()->getGuid() == incoming.m_targetGuid)
					targetFrame->playMessage(textMsg, textColor);
			}
		}
	}
}

void Game::processPacket_Server_Cooldown(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_Cooldown incoming;
	incoming.unpack(data);
	world->registerExistingCooldown(incoming.m_id, sApplication->timeNowMs(), sApplication->timeNowMs() + __time64_t(incoming.m_totalDuration), sApplication->timeNowMs());
}

void Game::processPacket_Server_UnitAuras(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}
	
	vector<shared_ptr<UnitFrame>> unitFrames =
	{
		dynamic_pointer_cast<UnitFrame>(world->getRenderObject(World::PlayerUnitFrame)),
		dynamic_pointer_cast<UnitFrame>(world->getRenderObject(World::TargetUnitFrame)),
		dynamic_pointer_cast<UnitFrame>(world->getRenderObject(World::Party1UnitFrame)),
		dynamic_pointer_cast<UnitFrame>(world->getRenderObject(World::Party2UnitFrame)),
		dynamic_pointer_cast<UnitFrame>(world->getRenderObject(World::Party3UnitFrame))
	};

	if (unitFrames.empty())
	{
		sApplication->logout();
		return;
	}

	GP_Server_UnitAuras incoming;
	incoming.unpack(data);

	auto unit = dynamic_pointer_cast<ClientUnit>(world->getClientObject(incoming.m_unitGuid));

	if (unit == nullptr)
		return;

	auto setDates = [](vector<GP_Server_UnitAuras::AuraInfo>& auras)
	{
		for (auto& itr : auras)
		{
			itr.startDate = sApplication->timeNowMs() - itr.elapsedTime;
			itr.endDate = sApplication->timeNowMs() + itr.maxDuration - itr.elapsedTime;
		
			if (itr.maxDuration == 0)
				itr.startDate = itr.endDate;
		}
	};
	
	setDates(incoming.m_buffs);
	setDates(incoming.m_debuffs);
	
	unit->setBuffs(incoming.m_buffs);
	unit->setDebuffs(incoming.m_debuffs);

	for (auto& itr : unitFrames)
	{
		// Notify gui elements of the new list
		if (itr->getUnitPtr() != nullptr && itr->getUnitPtr()->getGuid() == incoming.m_unitGuid)
		{
			itr->setBuffs(incoming.m_buffs);
			itr->setDebuffs(incoming.m_debuffs);
		}
	}

	if (incoming.m_unitGuid == world->getMyselfGuid())
	{
		world->setBuffs(incoming.m_buffs);
		world->setDebuffs(incoming.m_debuffs);
	}

	// Lua addon layer: notify the player/target aura rows.
	const char* token = (world->myself() && incoming.m_unitGuid == world->myself()->getGuid()) ? "player"
	                  : (incoming.m_unitGuid == world->getSelectedGuid() ? "target" : nullptr);
	if (token)
		sLua->fire(LuaEvents::UNIT_AURA, token);
}

void Game::processPacket_Server_UnitOrientation(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_UnitOrientation incoming;
	incoming.unpack(data);

	auto unit = dynamic_pointer_cast<ClientUnit>(world->getClientObject(incoming.m_guid));

	if (unit == nullptr)
		return;

	unit->setOrientation(incoming.m_newOri);
}

void Game::processPacket_Server_UnitTeleport(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_UnitTeleport incoming;
	incoming.unpack(data);

	auto unit = dynamic_pointer_cast<ClientUnit>(world->getClientObject(incoming.m_guid));

	if (unit == nullptr)
		return;

	unit->setWorldPosition({ incoming.m_newX, incoming.m_newY });
	unit->setOrientation(incoming.m_newOri);
}

void Game::processPacket_Server_SpellGo(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	GP_Server_SpellGo incoming;
	incoming.unpack(data);

	auto caster = dynamic_pointer_cast<ClientUnit>(world->getClientObject(incoming.m_casterGuid));

	if (caster == nullptr)
		return;
		
	const int flareAnimation = atoi(sContentMgr->db("spell_visual").data(incoming.m_spellId, "unit_go_animation").c_str());

	if (flareAnimation != 0)
		caster->playAnimation(static_cast<UnitDefines::AnimId>(flareAnimation));

	if (caster->getCastSpellId() == incoming.m_spellId)
		caster->setCastBar(nullptr, 0, 0);
	
	for (auto& itr : incoming.m_targets)
	{
		// Travel -> Impact
		auto victim = world->getClientObject(itr.first);

		if (itr.first != 0 && victim == nullptr)
			continue;

		shared_ptr<WorldSpellAnimation> sa = make_shared<WorldSpellAnimation>(&world->getMap(), incoming.m_spellId);
		sa->setSource(caster);
		sa->setTarget(victim);
		
		// Full volume if im involved in any way
		if (!(incoming.m_casterGuid == world->getMyselfGuid() || incoming.m_casterGuid == world->getSelectedGuid()))
		{
			sa->setSoundPoint_Source(sf::Vector2f(caster->getScreenPosition()));
			sa->setSoundPoint_Target(sf::Vector2f(victim ? victim->getScreenPosition() : caster->getScreenPosition()));
		}

		world->addWorldSpellAnimation(sa);
	}

	// Go
	auto sa = make_shared<WorldSpellAnimation>(&world->getMap(), incoming.m_spellId, false, true);
	sa->setSource(caster);
	sa->setTarget(caster);
	sa->setTarget(Geo2d::Vector2(incoming.m_groundX, incoming.m_groundY));
	
	// Full volume if im involved in any way
	if (!(incoming.m_casterGuid == world->getMyselfGuid() || incoming.m_casterGuid == world->getSelectedGuid()))
	{
		if (incoming.m_groundX != 0 || incoming.m_groundY != 0)
		{
			sa->setSoundPoint_Source(sf::Vector2f(incoming.m_groundX, incoming.m_groundY));
			sa->setSoundPoint_Target(sf::Vector2f(incoming.m_groundX, incoming.m_groundY));
		}
		else
		{
			sa->setSoundPoint_Source(sf::Vector2f(caster->getScreenPosition()));
			sa->setSoundPoint_Target(sf::Vector2f(caster->getScreenPosition()));
		}
	}

	world->addWorldSpellAnimation(sa);
}

void Game::processPacket_Server_GuildAddMember(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	auto gameChat = dynamic_pointer_cast<GameChat>(world->getRenderObject(World::GameChatBox));

	GP_Server_GuildAddMember incoming;
	incoming.unpack(data);
	gameChat->recvMsg(incoming.m_playerName + " has joined the guild.", "", ChatDefines::Guild);
	sContentMgr->playSound(SfxId::AlertRollover);
}

void Game::processPacket_Server_GuildRemoveMember(StlBuffer& data)
{
	auto world = dynamic_pointer_cast<World>(getRenderObject(RoWorld));

	if (world == nullptr)
	{
		sApplication->logout();
		return;
	}

	auto gameChat = dynamic_pointer_cast<GameChat>(world->getRenderObject(World::GameChatBox));

	GP_Server_GuildRemoveMember incoming;
	incoming.unpack(data);
	gameChat->recvMsg(incoming.m_playerName + " is no longer in the guild.", "", ChatDefines::Guild);
}