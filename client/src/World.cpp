#include "stdafx.h"
#include "World.h"
#include "Application.h"
#include "ClientMap.h"
#include "ClientPlayer.h"
#include "Connector.h"
#include "ContentMgr.h"
#include "ContentMgr.h"
#include "DialogNpc.h"
#include "Equipment.h"
#include "Game.h"
#include "lua/LuaFrameManager.h"
#include "lua/LuaEngine.h"
#include "ItemIcon.h"
#include "SpellIcon.h"
#include "GuildRoster.h"
#include "Inventory.h"
#include "Keybinds.h"
#include "QuestLog.h"
#include "QuestOffer.h"
#include "Button.h"
#include "SpriteRo.h"
#include "TextBox.h"
#include "Abilities.h"
#include "Toolbar.h"
#include "UnitFrame.h"
#include "VendorNpc.h"
#include "GameChat.h"
#include "WorldSpellAnimation.h"
#include "Text.h"
#include "Sprite.h"
#include "LootWindow.h"
#include "Options.h"
#include "ClientObjectOverhead.h"
#include "CastBar.h"
#include "Minimap.h"
#include "ZoneNotifcation.h"
#include "QuestComplete.h"
#include "DialogPanel.h"
#include "QuestObjectives.h"
#include "LevelupNotify.h"
#include "ToolbarXp.h"
#include "MapQuester.h"
#include "TradeWindow.h"
#include "Bank.h"

#include "..\Rand.h"
#include "..\StringHelpers.h"

#include "..\Shared\Config.h"
#include "..\Shared\MapLogic.h"
#include "..\Shared\GamePacketClient.h"
#include "..\SqlConnector\QueryResult.h"
#include "..\SqlConnector\SqlConnector.h"

#include <assert.h>

// Bridged "self" info (see World.h). Lives across the char-select -> NewWorld -> SetController hop.
World::PendingSelf World::s_pendingSelf;

World::World(Game& owner, const int id) :
	BuffDebuffRenderer(*this, &owner, id),
	m_game(owner),
	m_topcenterStartPct(1.25f),
	m_introAlpha(-1.f)
{
	setMultiInput(true);
	sKeybinds->load();
	m_map = make_shared<ClientMap>(*this, Interface::MapObject);
	addRenderObject(m_map);
	m_lastIsRenderingNativeDPI = sApplication->isRenderingOriginalDpi();
	
	m_cameraDrag = make_unique<DraggableNode>();
	m_cameraDrag->addAnchor(&m_cameraOffset);
	m_cameraDrag->addAnchor(&m_map->getCameraRef());
	m_cameraDrag->setMouseDragButton(sf::Mouse::Middle);
	m_cameraDrag->setDragSpeed(0.66f);

	cacheQuests();
	cacheGossipOptions();

	// Minimap
	addRenderObject(make_shared<Minimap>(*this, Interface::MinimapObj));

	// Quest Objectives
	addRenderObject(make_shared<QuestObjectives>(*this, Interface::ObjectivesObj));

	// CastBar
	addRenderObject(m_castBar = make_shared<CastBar>(*this, Interface::PlayerCastBar));

	// Game Chat
	addRenderObject(make_shared<GameChat>(*this, Interface::GameChatBox));

	// Toolbars
	addRenderObject(make_shared<SpriteRo>(*this, sContentMgr->spawnSprite("toolbar_base.png"), Interface::ToolbarSprite));
	m_toolbars.push_back(make_shared<Toolbar>(*this, Interface::Toolbar1));
	m_toolbars.push_back(make_shared<Toolbar>(*this, Interface::Toolbar2));
	m_toolbars.push_back(make_shared<Toolbar>(*this, Interface::Toolbar3));

	for (auto& itr : m_toolbars)
		addRenderObject(itr);

	addRenderObject(make_shared<ToolbarXp>(*this, Interface::ToolbarXpObj));
	
	// Unit frames
	addRenderObject(make_shared<UnitFrame>(*this, Interface::PlayerUnitFrame, sf::Vector2i()));
	
	auto targetUnitFrame = make_shared<UnitFrame>(*this, Interface::TargetUnitFrame, sf::Vector2i());
	targetUnitFrame->setFrameStyle(UnitFrame::FrameStyle::MyTarget);
	addRenderObject(targetUnitFrame);
	
	auto partyFrame = make_shared<UnitFrame>(*this, Interface::Party1UnitFrame, sf::Vector2i());
	partyFrame->setFrameStyle(UnitFrame::FrameStyle::PartyMember);
	m_partyFrames.push_back(partyFrame);
	addRenderObject(partyFrame);

	partyFrame = make_shared<UnitFrame>(*this, Interface::Party2UnitFrame, sf::Vector2i());
	partyFrame->setFrameStyle(UnitFrame::FrameStyle::PartyMember);
	m_partyFrames.push_back(partyFrame);
	addRenderObject(partyFrame);

	partyFrame = make_shared<UnitFrame>(*this, Interface::Party3UnitFrame, sf::Vector2i());
	partyFrame->setFrameStyle(UnitFrame::FrameStyle::PartyMember);
	m_partyFrames.push_back(partyFrame);
	addRenderObject(partyFrame);

	m_partyMembers.resize(m_partyFrames.size());

	// Toolbar interface buttons
	addRenderObject(make_shared<Button>(*this, "toolbar_social", Interface::SocialButton));
	addRenderObject(make_shared<Button>(*this, "toolbar_quests", Interface::QuestsButton));
	addRenderObject(make_shared<Button>(*this, "toolbar_options", Interface::OptionsButton));
	addRenderObject(m_equipmentButton = make_shared<Button>(*this, "toolbar_character", Interface::EquipmentButton));
	addRenderObject(make_shared<Button>(*this, "toolbar_inventory", Interface::InventoryButton));
	addRenderObject(make_shared<Button>(*this, "toolbar_spells", Interface::SpellsButton));

	// ZoneNotifcation
	addRenderObject(m_zoneIndicator = make_shared<ZoneNotifcation>(*this, Interface::ZoneNotify));

	// Panels
	addRenderObject(m_equipment = make_shared<Equipment>(*this, Interface::EquipmentPanel));
	addRenderObject(make_shared<QuestLog>(*this, Interface::QuestLogPanel));
	addRenderObject(make_shared<GuildRoster>(*this, Interface::GuildPanel));
	addRenderObject(make_shared<Inventory>(*this, Interface::InventoryPanel));
	addRenderObject(make_shared<Bank>(*this, Interface::BankPanel));
	addRenderObject(make_shared<Abilities>(*this, Interface::AbilitiesPanel));
	addRenderObject(make_shared<QuestOffer>(*this, Interface::QuestOfferPanel));
	addRenderObject(make_shared<QuestComplete>(*this, Interface::QuestCompletePanel));
	addRenderObject(make_shared<DialogNpc>(*this, Interface::DialogNpcPanel));
	addRenderObject(make_shared<VendorNpc>(*this, Interface::VendorNpcPanel));
	addRenderObject(make_shared<TradeWindow>(*this, Interface::TradeWindowPanel));
	addRenderObject(make_shared<LootWindow>(*this, Interface::LootWindowPanel));
	addRenderObject(m_mapQuester = make_shared<MapQuester>(*this, Interface::MapQuesterPanel));
	
	// Level up notify window
	addRenderObject(make_shared<LevelupNotify>(*this, Interface::LevelupNotifyObj));
	addRenderObject(m_spendExpButton = make_shared<Button>(*this, "spend_xp_notify", Interface::SpendExpButtonObj, sf::Vector2i((sApplication->sW() / 2) - 86, sApplication->sH() - 271)));
	m_spendExpButton->setHidden(true);
	
	// Waypoint button
	addRenderObject(m_waypointButton = make_shared<Button>(*this, "waypoint_activate", Interface::WaypointButton, sf::Vector2i((sApplication->sW() / 2) - 82, sApplication->centerOfScreen().y + 150)));
	m_waypointButton->setHidden(true);

	// Misc
	addRenderObject(makeBtnBindOnly(make_shared<Button>(*this, "generic_highlight_small", Interface::ButtonTargetSelf)));
	addRenderObject(makeBtnBindOnly(make_shared<Button>(*this, "generic_highlight_small", Interface::ButtonTargetParty1)));
	addRenderObject(makeBtnBindOnly(make_shared<Button>(*this, "generic_highlight_small", Interface::ButtonTargetParty2)));
	addRenderObject(makeBtnBindOnly(make_shared<Button>(*this, "generic_highlight_small", Interface::ButtonTargetParty3)));

	// Auras
	m_auraSpacing.x = 45;
	m_auraSpacing.y = 90;
	m_auraButtonFrame = "gameicon40";
	m_aurasOffset.x = -100;
	m_aurasOffset.y = 15;
	BuffDebuffRenderer::setDirection(BuffDebuffRenderer::Direction::RightLeft);
	m_auraAnchor = getRenderObject(Interface::MinimapObj)->getTopLeftCornerRef();
	registerAuraObjs(&m_auraAnchor);

	// Lua addon layer: root holder for Lua-created frames. World is multi-input, so Lua frames coexist
	// with the game UI and receive input; added late => high Z (renders above the game UI).
	addRenderObject(make_shared<LuaFrameManager>(*this, Interface::LuaFrameRoot));
	sLua->loadAddons();   // load addons/ now that the frame manager exists

	//togglePanel(Interface::BankPanel);
}

World::~World()
{
	sLua->saveAddons();   // persist addon SavedVariables before leaving the world

	if (sApplication->getWindow().isOpen())
		sContentMgr->stopAmbience();
}

void World::input()
{
	if (inputPointerAction())
		return;

	if (getRenderObject(Interface::CtxMenuCurrent) == nullptr && sApplication->getCurrentPromptInput() == 0)
	{
		// We have to check these before running input on our children otherwise they'll consume the escape keybind
		if (m_myself != nullptr && m_myself->getCastSpellId() != 0 && sApplication->popKeyUp(sf::Keyboard::Escape))
		{
			GP_Client_CancelCast packet;
			sConnector->sendPacket(packet.build(StlBuffer{}));
		}
		else if (getGrabbedIcon() != nullptr && (popEscapeKeyUp() || sApplication->mouseUp(sf::Mouse::Right)))
		{
			setGrabbedIcon(nullptr);
			sContentMgr->playSound(SfxId::WindowTargetClose);
			return;
		}
		else if (getSelectedGuid() != 0 && popEscapeKeyUp())
		{
			setSelectedGuid(0);
		}
	}
	
	updateButtonBind(Interface::QuestsButton, "QuestLog");
	updateButtonBind(Interface::EquipmentButton, "Equipment");
	updateButtonBind(Interface::InventoryButton, "Inventory");
	updateButtonBind(Interface::SpellsButton, "SpellBook");
	updateButtonBind(Interface::SocialButton, "GuildRoster");
	updateButtonBind(Interface::ButtonTargetSelf, "TargetSelf");
	updateButtonBind(Interface::ButtonTargetParty1, "TargetParty1");
	updateButtonBind(Interface::ButtonTargetParty2, "TargetParty2");
	updateButtonBind(Interface::ButtonTargetParty3, "TargetParty3");

	// Keybindings for buttons on the tool bars are named such as "ActionBar11"
	updateActionBarBind(Interface::Toolbar1, 1, 12);
	updateActionBarBind(Interface::Toolbar2, 13, 24);
	updateActionBarBind(Interface::Toolbar3, 25, 36);
	
	__super::input();

	// Prompt check after __super::input() because our stored objects might be the current prompt
	if (sApplication->getCurrentPromptInput() == 0 && !sConfig->getCache(Options::System_CtmTick))
		wasd();

	switch (popFirstButtonId())
	{
		case Interface::EquipmentButton: togglePanel(Interface::EquipmentPanel); break;
		case Interface::QuestsButton: togglePanel(Interface::QuestLogPanel); break;
		case Interface::SocialButton: togglePanel(Interface::GuildPanel); break;
		case Interface::InventoryButton: togglePanel(Interface::InventoryPanel); break;
		case Interface::SpellsButton: togglePanel(Interface::AbilitiesPanel); break;
		case Interface::OptionsButton: m_game.toggleOptions(true); break;
		case Interface::SpendExpButtonObj: launchSpendExp(); break;
		case Interface::WaypointButton: queryWaypoints(); break;
		case Interface::ButtonTargetSelf: selectUnitByButtonId(Interface::ButtonTargetSelf); break;
		case Interface::ButtonTargetParty1: selectUnitByButtonId(Interface::ButtonTargetParty1); break;
		case Interface::ButtonTargetParty2: selectUnitByButtonId(Interface::ButtonTargetParty2); break;
		case Interface::ButtonTargetParty3: selectUnitByButtonId(Interface::ButtonTargetParty3); break;
	}

	m_spendExpButton->setHidden(!m_equipmentButton->isExclaimNotice());

	if (m_equipment->isHidden())
		m_equipment->toggleSpendingPoints(false);
	else
		m_spendExpButton->setHidden(true);

	if (mouseInWorld() && getGrabbedIcon() == nullptr)
	{
		// Note that the mouse being in the world doesn't mean it's not moused over an object which is why this is done after the map is input
		if (sApplication->mouseUp(sf::Mouse::Left) && !sConfig->getCache(Options::System_StickyTick))
		{
			setSelectedGuid(0);
			sApplication->clearMouseUp();
		}
		else if (sApplication->mouseUp(sf::Mouse::Right) && sConfig->getCache(Options::System_CtmTick))
		{
			if (canMove())
			{
				sf::Vector2f location;
				m_map->getCellWorldPosRelative(location.x, location.y, { sApplication->mousePosf().x, sApplication->mousePosf().y });

				// Client-side prediction: the server does not echo a movement spline for
				// the local player's own moves, so build the path locally (mirrors wasd()).
				const Geo2d::Vector2 dest(location.x, location.y);
				std::vector<Geo2d::Vector2> path;

				if (MapLogic::checkLosToC(*m_map, m_myself->getWorldPosition(), dest, MapCellT::Unwalkable))
					path = { dest };
				else
					MapLogic::constructPathTo(*m_map, path, m_myself->getWorldPosition(), dest);

				if (!path.empty())
					m_myself->setSpline(path);

				requestMove(location, false);
				sApplication->clearMouseUp();
			}
		}
	}

	//if (mouseInWorld())
		tabTargeting();

	updateCameraDrag();

	if (m_grabbedIcon != nullptr && m_grabbedIcon->wasActivatedByDragging() && m_grabbedIcon->wasReleasedDragging())
	{
		m_grabbedIcon->resetWasReleaseActivated();
		m_grabbedIcon->resetWasActivatedByDragging();
		setGrabbedIcon(nullptr);
	}

	BuffDebuffRenderer::processRightClickedId(popFirstRightClickButtonId());
}

void World::render()
{
	m_lastMousedGuid = 0;
	updateGuiPositions();
	CooldownHolder::updateCooldowns(sApplication->timeNowMs());

	// Camera follows me around
	if (m_myself != nullptr && m_map != nullptr)
		cameraFollow();

	if (m_myself == nullptr || m_map == nullptr || !m_myself->isLocal() || !m_map->isReady())
	{
		// We might just be debugging an empty world object
		if (sConnector->connected())
		{
			sApplication->spawnTimedPopup("Loading", 0.5f);
			return;
		}
	}

	if (m_myself != nullptr)
	{
		m_map->setSoundOrigin(float(m_myself->getScreenPosition().x), float(m_myself->getScreenPosition().y));
		m_map->setLosOrigin(m_myself->getWorldPosition());
		m_castBar->setUnit(m_myself);

		if (m_nextCastSound.first != 0 && m_nextCastSound.first == m_myself->getCastSpellId())
		{
			sContentMgr->playSound(m_nextCastSound.second);
			m_nextCastSound = {};
		}

		// Low HP tutorial
		if (m_myself->isInCombat() == false && m_myself->getHealthPct() < 0.5f && sApplication->getTutorialStatus("Rest") == false)
		{
			if (auto btn = dynamic_pointer_cast<Toolbar>(getRenderObject(Interface::Toolbar1))->findIconByEntry(SpellDefines::StaticSpells::SleepRest))
			{
				btn->setExclaimNotice(true);
				pushCombatMessage("Low Health!", btn->getTopLeftCornerRef(), sf::Color::Red, true);
			}

			sApplication->setTutorialStatus("Rest", true);
		}

		// Waypoint button
		if (m_myself->getWaypointStandingGuid() != 0 && !isPanelOpen(Interface::MapQuesterPanel))
			m_waypointButton->setHidden(false);
		else
			m_waypointButton->setHidden(true);
	}

	// The the map is open, the zone indicator will get rendered with the rest of the other objects, otherwise, it's on top as seen below
	if (!isPanelOpen(Interface::MapQuesterPanel))
		registerSkipRenderingObjThisFrame(m_zoneIndicator->getId());

	__super::render();

	// Draw but also update/fade scrolling messages
	if (!m_topCenterMsgs.empty())
		drawTopCenterMsgs();

	if (!m_combatMessages.empty())
		drawCombatMsgs();

	if (m_grabbedIconCopy != nullptr)
	{
		m_grabbedIconCopy->getTopLeftCornerRef() = sApplication->mousePos(true);
		m_grabbedIconCopy->attemptRender();
		sApplication->queueCursor("cursor_grabbedicon.png");
	}

	if (m_introAlpha > 0.f)
	{
		sf::RectangleShape rectangle;
		rectangle.setSize(sf::Vector2f(sf::Vector2i(sApplication->sW(), sApplication->sH())));
		rectangle.setFillColor(sf::Color(0, 0, 0, uint8_t(255.f * min(m_introAlpha, 1.f))));
		sApplication->canvas().draw(rectangle);
		m_introAlpha -= sApplication->delta();
	}

	if (m_recalcObjectives && !(m_recalcObjectives = false))
		dynamic_pointer_cast<QuestObjectives>(getRenderObject(World::Interface::ObjectivesObj))->recalc();

	if (!isPanelOpen(Interface::MapQuesterPanel))
		m_zoneIndicator->attemptRender();

	if (m_groundActionSpell != 0 && mouseInWorld())
		sApplication->queueCursor("cursor_ground_action.png");
	else if (m_itemAction != nullptr)
		sApplication->queueCursor("cursor_ground_action.png");

	updateFadingObjects();
	updatePartyFrames();
}

bool World::popEscapeKeyUp()
{
	if (sApplication->popKeyUp(sf::Keyboard::Escape))
	{
		auto gameChat = dynamic_pointer_cast<GameChat>(getRenderObject(World::GameChatBox));
		ASSERT(gameChat);
		gameChat->setInUse(false);
		return true;
	}
	
	return false;
}

void World::updatePartyFrames()
{
	// If we don't have a party then just totally hide all of this stuff
	if (m_partyLeaderGuid == 0 || m_partyMembers.size() < m_partyFrames.size())
	{
		for (auto& itr : m_partyFrames)
		{
			itr->setOffline(false);
			itr->setUnit(nullptr);
		}

		return;
	}

	// Keep this up to date
	for (size_t i = 0; i < m_partyMembers.size(); ++i)
	{
		auto& frameObj = m_partyFrames[i];
		int guid = m_partyMembers[i];
		auto unit = dynamic_pointer_cast<ClientUnit>(getClientObject(guid));

		if (guid != 0 && unit == nullptr)
			frameObj->setOffline(true);

		frameObj->setUnit(unit);
	}
}

void World::queryWaypoints()
{
	if (m_myself == nullptr)
		return;

	// op45/op46/op130 VERIFIED correct vs the steam client's working travel (query wpGuid -> op130
	// count:u16 ids -> op46 nearby+target -> UnitTeleport). One known caveat: querying the SPAWN
	// waypoint specifically makes the server throw -> ECONNRESET; querying a normal/discovered waypoint
	// works. Re-enabled now that the packets are proven.
	GP_Client_QueryWaypoints packet;
	packet.m_nearbyWaypointGuid = m_myself->getWaypointStandingGuid();
	sConnector->sendPacket(packet.build(StlBuffer{}));
}

void World::launchSpendExp()
{
	togglePanel(Interface::EquipmentPanel);
	//m_equipment->toggleSpendingPoints(true);
	m_equipmentButton->setExclaimNotice(false);
	dynamic_pointer_cast<Button>(m_equipment->getRenderObject(Equipment::Interface::LevelUp))->setExclaimNotice(true);
	//sContentMgr->playSound("chun_command_center_edit.ogg");
}
		
bool World::inputPointerAction()
{
	if (m_groundActionSpell != 0 && m_map != nullptr && mouseInWorld())
	{
		if (sApplication->mouseUp(sf::Mouse::Left))
		{
			GP_Client_CastSpell packet;
			packet.m_spellId = m_groundActionSpell;
			packet.m_targetGuid = m_lastMousedGuid;
			m_map->getCellWorldPosRelative(packet.m_targetPosX, packet.m_targetPosY, { sApplication->mousePosf().x, sApplication->mousePosf().y });
			sConnector->sendPacket(packet.build(StlBuffer{}));
			m_groundActionSpell = 0;
			return true;
		}
		else if (sApplication->mouseUp(sf::Mouse::Right) || popEscapeKeyUp())
		{
			m_groundActionSpell = 0;
			return true;
		}
	}
	else if (m_itemAction != nullptr)
	{
		if (popEscapeKeyUp())
		{
			m_itemAction = nullptr;
			return true;
		}

		if (m_map != nullptr && mouseInWorld() && (sApplication->mouseUp(sf::Mouse::Right) || sApplication->mouseUp(sf::Mouse::Left)))
		{
			m_itemAction = nullptr;
			return true;
		}

		if (sApplication->mouseUp(sf::Mouse::Right) || sApplication->mouseDown(sf::Mouse::Right))
			sApplication->clearMouseDown();
	}

	return false;
}

void World::updateFadingObjects()
{
	for (auto itr = m_fadingObjects.begin(); itr != m_fadingObjects.end();)
	{
		itr->second.second -= sApplication->delta();

		if (itr->second.second <= 0.f)
		{
			// Deselect this unit once it's finally being removed for real
			if (itr->second.first->getGuid() == getSelectedGuid())
				setSelectedGuid(0);

			m_map->removeWorldRenderable(itr->second.first);
			itr = m_fadingObjects.erase(itr);
		}
		else
		{
			itr->second.first->setAlphaPct(itr->second.second);
			++itr;
		}
	}
}

void World::cameraFollow()
{
	const auto desiredCameraPosition = sf::Vector2f(getControllerCameraPosition()) + sf::Vector2f(m_cameraOffset.x, m_cameraOffset.y);
	const float distanceAway = m_map->getCameraRef().getDist(Geo2d::Vector2(desiredCameraPosition.x, desiredCameraPosition.y));
	
	// Extra room when moving the camera around quickly
	if (m_cameraDrag->isGrabbed())
		m_map->setPaddingRatio(10);
	else
		m_map->setPaddingRatio(2);

	if (distanceAway >= m_map->readyDistance())
	{
		m_map->getCameraRef() = Geo2d::Vector2(desiredCameraPosition.x, desiredCameraPosition.y);
	}
	else if (distanceAway > 15.0f)
	{
		float moveAmount = distanceAway * sApplication->delta() * 2;
		auto extrudePos = Geo2d::extrude(m_map->getCameraRef().x, m_map->getCameraRef().y, desiredCameraPosition.x, desiredCameraPosition.y, min(moveAmount, distanceAway));
		m_map->getCameraRef() = extrudePos;
	}
}

void World::requestMove(const sf::Vector2f& worldPos, const bool wasd)
{
	if (m_myself == nullptr)
		return;
	
	// Early client side InterruptCauses::Movement
	if (m_myself->getCastSpellId() != 0)
	{
		auto& sv = sContentMgr->db("spell_template");
		int castSpellFlags = atoi(sv.data(m_myself->getCastSpellId(), "cast_interrupt_flags").c_str());		
		int movementFlag = 1 << (int(SpellDefines::InterruptCauses::Movement) - 1);

		if (castSpellFlags & movementFlag)
			m_myself->setCastBar(nullptr, 0, 0);
	}

	GP_Client_RequestMove packet;
	packet.m_wasd = wasd;
	packet.m_destX = worldPos.x;
	packet.m_destY = worldPos.y;
	sConnector->sendPacket(packet.build(StlBuffer{}));
}

void World::updateGuiPositions()
{
	if (m_lastScreenSize == sApplication->getWindow().getSize() && m_lastIsRenderingNativeDPI == sApplication->isRenderingOriginalDpi())
		return;

	m_lastIsRenderingNativeDPI = sApplication->isRenderingOriginalDpi();
	m_lastScreenSize = sApplication->getWindow().getSize();

	auto toolbarSpriteRo = dynamic_pointer_cast<SpriteRo>(getRenderObject(Interface::ToolbarSprite));
	auto toolbarSprite = toolbarSpriteRo->getRaw();
	const sf::Vector2i toolbarSpritePos((sApplication->sW() / 2) - int(toolbarSprite->getTexture()->getSize().x / 2), sApplication->sH() - int(toolbarSprite->getTexture()->getSize().y));
	toolbarSpriteRo->updateTopLeftCorner(toolbarSpritePos);

	getRenderObject(Interface::Toolbar1)->updateTopLeftCorner(toolbarSpritePos + sf::Vector2i(151, 11));
	getRenderObject(Interface::Toolbar2)->updateTopLeftCorner(toolbarSpritePos + sf::Vector2i(151, -36));
	getRenderObject(Interface::Toolbar3)->updateTopLeftCorner(toolbarSpritePos + sf::Vector2i(151, -83));

	getRenderObject(Interface::EquipmentButton)->updateTopLeftCorner(toolbarSpritePos + sf::Vector2i(36, 33));
	getRenderObject(Interface::QuestsButton)->updateTopLeftCorner(toolbarSpritePos + sf::Vector2i(72, 33));
	getRenderObject(Interface::OptionsButton)->updateTopLeftCorner(toolbarSpritePos + sf::Vector2i(108, 33));
	getRenderObject(Interface::SocialButton)->updateTopLeftCorner(toolbarSpritePos + sf::Vector2i(705, 33));
	getRenderObject(Interface::InventoryButton)->updateTopLeftCorner(toolbarSpritePos + sf::Vector2i(741, 33));
	getRenderObject(Interface::SpellsButton)->updateTopLeftCorner(toolbarSpritePos + sf::Vector2i(777, 33));
	getRenderObject(Interface::ToolbarXpObj)->updateTopLeftCorner(toolbarSpritePos + sf::Vector2i(104, 56));
	getRenderObject(Interface::SpendExpButtonObj)->updateTopLeftCorner(sf::Vector2i((sApplication->sW() / 2) - 86, sApplication->sH() - 271));
	getRenderObject(Interface::WaypointButton)->updateTopLeftCorner(sf::Vector2i((sApplication->sW() / 2) - 82, sApplication->centerOfScreen().y + 150));
	
	// CastBar
	int castBarY = sApplication->centerOfScreen().y + 150;

	if (castBarY > sApplication->sH() - 250)
		castBarY = sApplication->sH() - 250;

	getRenderObject(Interface::PlayerCastBar)->updateTopLeftCorner(sf::Vector2i((sApplication->sW() / 2) - 160, castBarY));

	// Game Chat
	int gameChatX = max(0, (sApplication->sW() / 2) - 1100);
	getRenderObject(Interface::GameChatBox)->updateTopLeftCorner(sf::Vector2i(gameChatX, sApplication->sH() - 250));

	// Minimap
	int minimapX = sApplication->sW() - 260;
	getRenderObject(Interface::MinimapObj)->updateTopLeftCorner(sf::Vector2i(minimapX, 15));

	// Auras
	m_auraAnchor.x = min(sApplication->sW() - 260, (sApplication->sW() / 2) + 700);

	// Panels
	refreshPanelXpos();
	auto anyPanel = dynamic_pointer_cast<WorldPanel>(getRenderObject(Interface::EquipmentPanel));	
	dynamic_pointer_cast<WorldPanel>(getRenderObject(Interface::EquipmentPanel))->getTopLeftCornerRef().y = anyPanel->worldPanelYPos();
	dynamic_pointer_cast<WorldPanel>(getRenderObject(Interface::QuestLogPanel))->getTopLeftCornerRef().y = anyPanel->worldPanelYPos();
	dynamic_pointer_cast<WorldPanel>(getRenderObject(Interface::GuildPanel))->getTopLeftCornerRef().y = anyPanel->worldPanelYPos();
	dynamic_pointer_cast<WorldPanel>(getRenderObject(Interface::AbilitiesPanel))->getTopLeftCornerRef().y = anyPanel->worldPanelYPos();
	dynamic_pointer_cast<WorldPanel>(getRenderObject(Interface::InventoryPanel))->getTopLeftCornerRef().y = anyPanel->worldPanelYPos();
	dynamic_pointer_cast<DialogPanel>(getRenderObject(Interface::QuestOfferPanel))->resetPosition();
	dynamic_pointer_cast<DialogPanel>(getRenderObject(Interface::QuestCompletePanel))->resetPosition();
	dynamic_pointer_cast<DialogPanel>(getRenderObject(Interface::DialogNpcPanel))->resetPosition();
	dynamic_pointer_cast<DialogPanel>(getRenderObject(Interface::VendorNpcPanel))->resetPosition();
	dynamic_pointer_cast<DialogPanel>(getRenderObject(Interface::BankPanel))->resetPosition();
	
	// Unit frame
	int myUnitFrameX = max(50, (sApplication->sW() / 2) - 800);
	getRenderObject(Interface::PlayerUnitFrame)->updateTopLeftCorner(sf::Vector2i(myUnitFrameX, 15));
	getRenderObject(Interface::TargetUnitFrame)->updateTopLeftCorner(sf::Vector2i(myUnitFrameX + 400, 15));
	getRenderObject(Interface::Party1UnitFrame)->updateTopLeftCorner(sf::Vector2i(myUnitFrameX - 50, 220));
	getRenderObject(Interface::Party2UnitFrame)->updateTopLeftCorner(sf::Vector2i(myUnitFrameX - 50, 350));
	getRenderObject(Interface::Party3UnitFrame)->updateTopLeftCorner(sf::Vector2i(myUnitFrameX - 50, 480));
}

void World::updateButtonBind(const Interface id, const string& bindname)
{
	if (auto button = dynamic_pointer_cast<Button>(getRenderObject(id)))
		button->setKeyEvent(sKeybinds->getKeybindData(bindname));
}

void World::updateActionBarBind(const Interface id, const int startI, const int endI)
{
	int count = 0;

	for (int i = startI; i <= endI; ++i)
	{
		const string bindName = "ActionBar" + to_string(i);
		const auto buttonId = Toolbar::Interface(Toolbar::GameIcon1 + (count++));

		if (auto toolbar = dynamic_pointer_cast<Toolbar>(getRenderObject(id)))
			toolbar->setButtonBind(buttonId, sKeybinds->getKeybindData(bindName));
	}
}

void World::setMap(const int mapId)
{
	m_combatMessages.clear();
	m_objects.clear();
	m_fadingObjects.clear();
	setSelectedGuid(0);
	closeDialogPanels();

	sContentMgr->stopAllLoopSound();
	m_mapId = mapId;
	const auto& db = sContentMgr->db("map");
	const string mapname = db.data(mapId, "name");
	ASSERT(m_map->loadFromDisk(mapname));
	m_introAlpha = 3.f;
	dynamic_pointer_cast<Minimap>(getRenderObject(Interface::MinimapObj))->setMap(mapname);
	m_zoneIndicator->onMapChange();
}

void World::setController(const int guid)
{
	// The server streams the controlled player's data inside NewWorld(op77) WITHOUT a guid,
	// then SetController(op80) supplies the guid. So the player object usually isn't in
	// m_objects yet — spawn it here from the bridged char-select + NewWorld pos info.
	auto controlled = getClientObject(guid);

	if (controlled == nullptr)
	{
		auto player = make_shared<ClientPlayer>(
			guid, &getMap(),
			static_cast<PlayerDefines::Classes>(s_pendingSelf.classId),
			static_cast<PlayerDefines::Gender>(s_pendingSelf.gender),
			s_pendingSelf.portrait);

		player->setName(s_pendingSelf.name);

		if (s_pendingSelf.hasPos)
			player->setWorldPosition({ s_pendingSelf.x, s_pendingSelf.y });

		for (auto& v : s_pendingSelf.vars)
			player->setVariable(static_cast<ObjDefines::Variable>(v.first), v.second);

		for (auto& e : s_pendingSelf.equipment)
			player->setEquipment(static_cast<UnitDefines::EquipSlot>(e.slot), e.item);

		addClientObject(player);
		controlled = player;
	}

	m_myself = dynamic_pointer_cast<ClientPlayer>(controlled);

	if (m_myself == nullptr)
	{
		sApplication->logout();
		return;
	}

	BuffDebuffRenderer::setUnit(m_myself);
	BuffDebuffRenderer::refreshDispelTypeLogic();

	m_myself->setIsLocal(true);

	if (!m_myself->isAlive())
		m_myself->queueDiedPopup();

	auto toolbar1 = dynamic_pointer_cast<Toolbar>(getRenderObject(Interface::Toolbar1));
	auto toolbar2 = dynamic_pointer_cast<Toolbar>(getRenderObject(Interface::Toolbar2));
	auto toolbar3 = dynamic_pointer_cast<Toolbar>(getRenderObject(Interface::Toolbar3));

	if (toolbar1->getPlayerName() != m_myself->getName())
	{
		// Auto attack, Auto shot are default actions
		toolbar1->createBaseIcon(Toolbar::Interface::GameIcon1, GameIcon::Type::Spell, SpellDefines::StaticSpells::MeleeSpell);
		toolbar2->createBaseIcon(Toolbar::Interface::GameIcon11, GameIcon::Type::Spell, SpellDefines::StaticSpells::Lockpicking);
		toolbar2->createBaseIcon(Toolbar::Interface::GameIcon12, GameIcon::Type::Spell, SpellDefines::StaticSpells::SleepRest);

		if (m_myself->canEquipItem(m_myself->getClass(), ItemDefines::EquipType::Ranged))
			toolbar1->createBaseIcon(Toolbar::Interface::GameIcon2, GameIcon::Type::Spell, SpellDefines::StaticSpells::RangedSpell);	
	
		const bool loadToolbar1 = toolbar1->loadCache(m_myself->getName());
		const bool loadToolbar2 = toolbar2->loadCache(m_myself->getName());
		const bool loadToolbar3 = toolbar3->loadCache(m_myself->getName());
		m_emptyToolbars = !(loadToolbar1 || loadToolbar2 || loadToolbar3);
	}

	// Interface
	dynamic_pointer_cast<Equipment>(getRenderObject(Interface::EquipmentPanel))->updatePlayer(m_myself);
	dynamic_pointer_cast<UnitFrame>(getRenderObject(Interface::PlayerUnitFrame))->setUnit(m_myself);
	dynamic_pointer_cast<Inventory>(getRenderObject(World::Interface::InventoryPanel))->setMoney(m_myself->getVariable(ObjDefines::Variable::Money));
	dynamic_pointer_cast<VendorNpc>(getRenderObject(World::Interface::VendorNpcPanel))->setLocalMoney(m_myself->getVariable(ObjDefines::Variable::Money));
	dynamic_pointer_cast<QuestLog>(getRenderObject(World::Interface::QuestLogPanel))->loadTrackedQuests();
	dynamic_pointer_cast<MapQuester>(getRenderObject(Interface::MapQuesterPanel))->loadCache();

	// Map camera
	const auto newCameraPosition = getControllerCameraPosition();
	m_map->getCameraRef() = { static_cast<float>(newCameraPosition.x), static_cast<float>(newCameraPosition.y) };

}
		
bool World::isPartyLeader(const int guid) const
{
	return m_partyLeaderGuid == guid;
}

bool World::popEmptyToolbars()
{
	bool result = m_emptyToolbars;
	m_emptyToolbars = false;
	return result;
}

sf::Vector2i World::getControllerCameraPosition() const
{
	ASSERT(m_myself != nullptr);
	const auto raw = m_myself->computeRawScreenPositioni() - sApplication->centerOfScreen();
	return { -raw.x, -raw.y };
}

string World::getGossipOptionsString(const int entry) const
{
	auto itr = m_gossipOptions.find(entry);

	if (itr == m_gossipOptions.end())
		return "";

	return itr->second;
}

string World::getGossipString(const int entry) const
{
	auto itr = m_gossips.find(entry);

	if (itr == m_gossips.end())
		return "";

	return itr->second;
}

string World::getMyselfName() const
{
	if (m_myself == nullptr)
		return "";

	return m_myself->getName();
}

shared_ptr<ClientObject> World::getClientObject(const int guid) const
{
	auto itr = m_objects.find(guid);

	if (itr != m_objects.end())
	{
		ASSERT(itr->second->getGuid() == guid);
		return itr->second;
	}

	return nullptr;
}

void World::closePanels(const set<Interface>& exceptions, const bool playsound)
{
	for (int i = Interface::Panels_Start + 1; i < Interface::Panels_End; ++i)
	{
		if (exceptions.find(Interface(i)) != exceptions.end())
			continue;

		closePanel(Interface(i), playsound);
	}
}

void World::closeDialogPanels()
{
	for (int i = Interface::Panels_Start + 1; i < Interface::Panels_End; ++i)
	{
		if (auto panel = dynamic_pointer_cast<DialogPanel>(getRenderObject(i)))
			closePanel(Interface(i), false);
	}
}

void World::switchDialogPanel(const Interface id)
{
	if (isPanelOpen(id))
		return;

	closeDialogPanels();
	openPanel(id);

	if (auto ptr = dynamic_pointer_cast<DialogPanel>(getRenderObject(id)))
		ptr->refreshDockPosition();
}

void World::openPanel(const Interface id, const bool playsound)
{
	if (isPanelOpen(id))
		return;

	if (playsound)
		sContentMgr->playSound(SfxId::WindowOpen);

	bool frontInsert = true;

	if (auto firstPanel = dynamic_pointer_cast<WorldPanel>(getRenderObject(id)))
	{
		firstPanel->onOpen();
		firstPanel->setHidden(false);
		frontInsert = firstPanel->isFrontInsertPanel();

		if (firstPanel->independant())
			return;
	}
	
	if (!m_leftPanel.empty())
	{
		// Limit of three at a time
		if (m_leftPanel.size() >= 3)
		{
			if (auto firstPanel = getRenderObject(m_leftPanel[0]))
				firstPanel->setHidden(true);

			m_leftPanel.erase(m_leftPanel.begin());
		}
	}

	if (frontInsert)
		m_leftPanel.insert(m_leftPanel.begin(), id);
	else
		m_leftPanel.push_back(id);

	refreshPanelXpos();
}

void World::closePanel(const Interface id, const bool playsound)
{
	if (!isPanelOpen(id))
		return;

	if (playsound)
		sContentMgr->playSound(SfxId::WindowClose);

	if (auto panel = dynamic_pointer_cast<WorldPanel>(getRenderObject(id)))
	{
		panel->setHidden(true);
		panel->resetPosition();
		panel->onClose();

		if (panel->independant())
			return;
	}

	auto itr = m_leftPanel.begin();

	// Not sure how we'd ever have it more than once but let's just iterate all of them anyway
	while (itr != m_leftPanel.end())
	{
		if (*itr == id)
			itr = m_leftPanel.erase(itr);
		else
			++itr;
	}

	refreshPanelXpos();
}

int World::getPanelMaxXpos() const
{
	int x = 0;

	// Move all current ones over to the right
	for (auto& id : m_leftPanel)
	{
		if (auto firstPanel = getRenderObject(id))
		{
			// Width is calculated based on bottomRight - topLeft, but we're about to change top left
			const int width = firstPanel->mouseableWidth();
			x += width;
		}
	}

	return x;
}

void World::refreshPanelXpos()
{
	int x = max(0, (sApplication->sW() / 2) - 1100);

	// Move all current ones over to the right
	for (auto& id : m_leftPanel)
	{
		if (auto firstPanel = dynamic_pointer_cast<WorldPanel>(getRenderObject(id)))
		{
			// Width is calculated based on bottomRight - topLeft, but we're about to change top left
			const int width = firstPanel->mouseableWidth();
			firstPanel->getTopLeftCornerRef().x = x;
			firstPanel->getTopLeftCornerRef().y = firstPanel->worldPanelYPos();
			x += width;
		}
	}
}

bool World::canAct() const
{
	if (m_myself == nullptr)
		return false;

	if (!m_myself->isAlive())
		return false;

	if (m_myself->isSlidingSpline())
		return false;

	int mechanicMsk = m_myself->getVariable(ObjDefines::Variable::MechanicMask);

	if (Util::maskHas(mechanicMsk, SpellDefines::Mechanics::Fear) ||
		Util::maskHas(mechanicMsk, SpellDefines::Mechanics::Confused) ||
		Util::maskHas(mechanicMsk, SpellDefines::Mechanics::Stun) ||
		Util::maskHas(mechanicMsk, SpellDefines::Mechanics::Sleep) ||
		Util::maskHas(mechanicMsk, SpellDefines::Mechanics::Incapacitated) ||
		Util::maskHas(mechanicMsk, SpellDefines::Mechanics::Charging) ||
		Util::maskHas(mechanicMsk, SpellDefines::Mechanics::Polymorph))
	{
		return false;
	}

	if (m_myself->getVariable(ObjDefines::Variable::IsAnimating) != 0)
		return false;

	if (m_myself->getVariable(ObjDefines::Variable::IsSliding) != 0)
		return false;

	return true;
}

bool World::canMove() const
{
	if (!canAct())
		return false;

	int mechanicMsk = m_myself->getVariable(ObjDefines::Variable::MechanicMask);

	if (Util::maskHas(mechanicMsk, SpellDefines::Mechanics::Root))
		return false;

	return true;
}

void World::tabTargeting()
{
	if (!sApplication->keyDown(sf::Keyboard::Tab))
		return;

	// After 1 idle second, we're back to just targeting the closest
	if (sApplication->timeNowMs() - m_tabTargetTimer > 1000 || getSelectedGuid() == 0)
		m_tabTargetList.clear();

	m_tabTargetTimer = sApplication->timeNowMs();

	vector<ClientUnit*> viableTargets;

	for (auto& itr : m_objects)
	{
		if (ClientUnit* ptr = dynamic_cast<ClientUnit*>(itr.second.get()))
		{
			if (m_tabTargetList.find(ptr->getGuid()) != m_tabTargetList.end())
				continue;

			if (m_myself->seesAsFriendly(*ptr))
				continue;

			if (!ptr->isAlive())
				continue;

			if (getSelectedGuid() == ptr->getGuid())
				continue;

			if (!ptr->isWithinScreenPosition())
				continue;
			 
			viableTargets.push_back(ptr);
		}
	}

	if (viableTargets.empty() && !m_tabTargetList.empty())
	{
		// We cycled through all of them, so start over
		m_tabTargetList.clear();
		tabTargeting();
		return;
	}

	if (viableTargets.empty())
		return;

	sort(viableTargets.begin(), viableTargets.end(),		
		[&](const ClientUnit* a, const ClientUnit* b)
	{
		float dist1 = Geo2d::distance2di(a->getScreenPosition().x, a->getScreenPosition().y, m_myself->getScreenPosition().x, m_myself->getScreenPosition().y);
		float dist2 = Geo2d::distance2di(b->getScreenPosition().x, b->getScreenPosition().y, m_myself->getScreenPosition().x, m_myself->getScreenPosition().y);

		// Closest unit should be first in the list
		return dist1 < dist2;
	});

	ClientUnit* target = *viableTargets.begin();
	setSelectedGuid(target->getGuid());
	m_tabTargetList.insert(target->getGuid());
}

void World::wasd()
{
	if (m_myself == nullptr || !canMove())
		return;

	auto BindMoveUp = sKeybinds->getKeybindData("MoveUp");
	auto BindMoveDown = sKeybinds->getKeybindData("MoveDown");
	auto BindMoveLeft = sKeybinds->getKeybindData("MoveLeft");
	auto BindMoveRight = sKeybinds->getKeybindData("MoveRight");

	bool moveUp = sf::Keyboard::isKeyPressed(BindMoveUp.code);
	bool moveDown = sf::Keyboard::isKeyPressed(BindMoveDown.code);
	bool moveLeft = sf::Keyboard::isKeyPressed(BindMoveLeft.code);
	bool moveRight = sf::Keyboard::isKeyPressed(BindMoveRight.code);
	
	// Trying to move left and right at same time cancels each other out
	if (moveLeft && moveRight)
		moveLeft = moveRight = false;

	if (moveUp && moveDown)
		moveUp = moveDown = false;

	// We don't want to flood the server with a packet every frame
	WASDCooldown cd;

	Geo2d::Vector2 worldPos = m_myself->getWorldPosition();

	// Up/Left
	if (moveUp && moveLeft)
	{
		cd = WASDCooldown::UpLeft;
		worldPos.x -= 1.0f;
	}

	// Up/Right
	else if (moveUp && moveRight)
	{
		cd = WASDCooldown::UpRight;
		worldPos.y -= 1.0f;
	}

	// Down/Left
	else if (moveDown && moveLeft)
	{
		cd = WASDCooldown::DownLeft;
		worldPos.y += 1.0f;
	}

	// Down/Right
	else if (moveDown && moveRight)
	{
		cd = WASDCooldown::DownRight;
		worldPos.x += 1.0f;
	}

	// Up
	else if (moveUp)
	{
		cd = WASDCooldown::Up;
		worldPos.x -= 1.0f;
		worldPos.y -= 1.0f;
	}

	// Down
	else if (moveDown)
	{
		cd = WASDCooldown::Down;
		worldPos.x += 1.0f;
		worldPos.y += 1.0f;
	}

	// Left
	else if (moveLeft)
	{
		cd = WASDCooldown::Left;
		worldPos.x -= 0.75f;
		worldPos.y += 0.75f;
	}

	// Right
	else if (moveRight)
	{
		cd = WASDCooldown::Right;
		worldPos.x += 0.75f;
		worldPos.y -= 0.75f;
	}
	else
	{
		if (sApplication->keyUp(BindMoveUp.code, BindMoveUp.alt, BindMoveUp.control, BindMoveUp.shift) ||
			sApplication->keyUp(BindMoveDown.code, BindMoveDown.alt, BindMoveDown.control, BindMoveDown.shift) ||
			sApplication->keyUp(BindMoveLeft.code, BindMoveLeft.alt, BindMoveLeft.control, BindMoveLeft.shift) ||
			sApplication->keyUp(BindMoveRight.code, BindMoveRight.alt, BindMoveRight.control, BindMoveRight.shift))
		{
			m_myself->clearSpline();
			
			GP_Client_RequestStop packet;
			packet.m_myX = m_myself->getWorldPosition().x;
			packet.m_myY = m_myself->getWorldPosition().y;
			sConnector->sendPacket(packet.build(StlBuffer{}));
		}

		return;
	}

	const auto cnow = std::clock();

	if (cnow - m_wasdCooldown[cd] > 100)
	{
		m_wasdCooldown[cd] = cnow;
		
		// Preemptively give the controller the likely spline to reduce the feel of lag
		if (MapLogic::checkLosToC(*m_map, m_myself->getWorldPosition(), worldPos, MapCellT::Unwalkable))
		{
			m_myself->setSpline({ worldPos });
			requestMove(sf::Vector2f(worldPos.x, worldPos.y), true);
		}
	}
}

void World::selectUnitByButtonId(const Interface id)
{
	if (id == ButtonTargetSelf)
	{
		setSelectedGuid(m_myself ? m_myself->getGuid() : 0);
	}

	if (id == ButtonTargetParty1)
	{
		if (auto partyFrame = dynamic_pointer_cast<UnitFrame>(getRenderObject(Interface::Party1UnitFrame)))
		{
			if (auto unit = partyFrame->getUnitPtr())
				setSelectedGuid(unit->getGuid());
		}
	}

	if (id == ButtonTargetParty2)
	{
		if (auto partyFrame = dynamic_pointer_cast<UnitFrame>(getRenderObject(Interface::Party2UnitFrame)))
		{
			if (auto unit = partyFrame->getUnitPtr())
				setSelectedGuid(unit->getGuid());
		}
	}

	if (id == ButtonTargetParty3)
	{
		if (auto partyFrame = dynamic_pointer_cast<UnitFrame>(getRenderObject(Interface::Party3UnitFrame)))
		{
			if (auto unit = partyFrame->getUnitPtr())
				setSelectedGuid(unit->getGuid());
		}
	}
}

void World::togglePanel(const Interface id)
{
	if (isPanelOpen(id))
		closePanel(id);
	else
		openPanel(id);
}

bool World::isPartyMember(const int guid) const
{
	if (m_partyLeaderGuid == 0)
		return false;

	if (guid == getMyselfGuid())
		return true;

	return find(m_partyMembers.begin(), m_partyMembers.end(), guid) != m_partyMembers.end();
}

bool World::isPanelOpen(const Interface id) const
{
	if (auto firstPanel = getRenderObject(id))
		return !firstPanel->isHidden();

	// Where the hell is it?
	return false;
}

bool World::mouseInWorld() const
{
	// Moused over some sort of gui object
	if (auto obj = getFirstMousedOver(false, Interface::MapObject))
		return false;

	for (auto& itr : m_toolbars)
	{
		if (auto obj = itr->getFirstMousedOver())
			return false;
	}

	return true;
}

void World::drawTopCenterMsgs()
{	
	auto msgItr = m_topCenterMsgs.begin();

	while (msgItr != m_topCenterMsgs.end())
	{
		auto& text = (*msgItr);
		text.txt->draw();

		sf::Color textColor(sf::Color(text.originalColor));
		sf::Color outlineColor = sf::Color::Black;

		if (text.alphaPct < 0.0f)
		{
			// Done
			textColor.a = 0;
		}
		else if (text.alphaPct < 1.0f)
		{
			// Fading away
			textColor.a = static_cast<int>(textColor.a * static_cast<float>(text.alphaPct));
			text.alphaPct -= 0.5f * sApplication->delta();
		}
		else 
		{
			if (text.alphaPct > m_topcenterStartPct - 0.03f && msgItr == m_topCenterMsgs.begin())
			{
				// Brighter intro
				const float diff = -(m_topcenterStartPct - 0.03f - text.alphaPct);
				textColor = Util::brightenColor(sf::Color(text.originalColor), diff * 15);
			}

			// Solid color
			textColor.a = 255;
			text.alphaPct -= 0.1f * sApplication->delta();
		}

		outlineColor.a = textColor.a;

		if (textColor.a <= 0)
		{
			msgItr = m_topCenterMsgs.erase(msgItr);
			computeTopCenterMsgPositions();
		}
		else
		{
			++msgItr;
			text.txt->setColor(textColor, outlineColor);
		}
	}
}

void World::drawCombatMsgs()
{
	for (auto& messagesListRef : m_combatMessages)
	{
		auto& messagesList = messagesListRef.second;

		for (auto itr = messagesList.begin(); itr != messagesList.end();)
		{
			auto& msg = (*itr);
			float alphaPct = 1.f;
			float delta = sApplication->delta() * msg.decayRate;

			if (msg.moveUpward)
				msg.startPosf.y -= delta * 30.f;

			if (msg.floatSideways)
				msg.startPosf.x -= delta * 30.f * msg.sidewaysSwitch;

			// 200ms fade in
			if (msg.fadeInTimer < 0.2f)
			{
				alphaPct = msg.fadeInTimer / 0.3f;
				msg.fadeInTimer += delta;
			}

			// 1s hold
			else if (msg.solidTimer < 1.f)
			{
				msg.solidTimer += delta;
			}

			// 500ms fade1
			else
			{
				alphaPct = 1.f - (msg.fadeOutTimer / .500f);
				msg.fadeOutTimer += delta;
			}

			if (alphaPct <= 0.f && msg.fadeOutTimer != 0)
			{
				itr = messagesList.erase(itr);
				continue;
			}

			// Text color
			sf::Color textColor(msg.textColor);
			sf::Color outlineColor = sf::Color::Black;
			textColor.a = outlineColor.a = static_cast<uint8_t>(float(textColor.a) * alphaPct);
			msg.txt->setFillColor(textColor);
			msg.txt->setOutlineColor(outlineColor);
			msg.txt->setOutlineThickness(1.f);
			
			sf::Vector2i newPos(msg.startPosf);

			if (!msg.ignoreCamera)
			{
				// Adjust for camera changes
				Geo2d::Vector2 cameraChange(msg.originalCamera);
				cameraChange.x -= m_map->getCameraRef().x;
				cameraChange.y -= m_map->getCameraRef().y;

				newPos.x -= int(cameraChange.x);
				newPos.y -= int(cameraChange.y);
			}

			// Crit animation
			if (msg.crit)
			{
				if (!msg.sizeSwitch)
				{
					msg.size += sApplication->delta() * 500.f;

					if (msg.size >= msg.sizeDest)
						msg.sizeSwitch = true;
				}
				else
				{
					msg.size -= sApplication->delta() * 500.f;
				}

				if (msg.size > msg.sizeDest)
					msg.size = msg.sizeDest;

				// Stay big, but not as big as the dest
				if (msg.size < msg.sizeRestDest)
					msg.size = msg.sizeRestDest;
		
				msg.txt->setCharacterSize(static_cast<int>(msg.size));
		
				// Center
				float widthAfter = msg.txt->getGlobalBounds().width;		
				newPos.x -= int((widthAfter - msg.widthBefore) / 2);
			}

			msg.txt->draw(newPos.x, newPos.y - msg.txt->getCharacterSize());
			++itr;
		}
	}
}

void World::pushCombatMessage(const string& str, const sf::Vector2i& pos, const sf::Color color /*= sf::Color::White*/, const bool crit /*= false*/, 
	const float textScale /*= 1.f*/, const int sourceGuid /*= 0*/, const bool floatSideways /*= false*/, const float decayRate /*= 1.f*/, const bool pushUpward /*= true*/, const bool ignoreCameraChanges /*= false*/, const bool makeRoomAtSide /*= true*/, const bool skipFadeIn /*= false*/)
{
	if (m_map == nullptr)
		return;

	CombatMsg msg;
	msg.size = 35.f * textScale;
	msg.msg = str;
	msg.textColor = color;
	msg.sourceGuid = sourceGuid;
	msg.txt = make_shared<Text>(sContentMgr->getFont(FontId::FrizRegular));
	msg.decayRate = decayRate;
	msg.txt->setShadowOffset(1);
	msg.originalCamera = m_map->getCameraRef();
	msg.startDate = std::clock();
	msg.ignoreCamera = ignoreCameraChanges;

	if (skipFadeIn)
		msg.fadeInTimer = 1.f;

	if (auto worldObj = this->getClientObject(sourceGuid))
		msg.objStartPos = worldObj->getWorldPosition();

	// Diagnol text alternates between left/right
	if (msg.floatSideways = floatSideways)
	{
		static map<int, float> sidewaysSwitch;

		if (sidewaysSwitch[sourceGuid] == 0)
			sidewaysSwitch[sourceGuid] = -1.f;

		msg.sidewaysSwitch = sidewaysSwitch[sourceGuid] *= -1.f;
	}
	
	// Will expand then contract in size
	if (msg.crit = crit)
	{
		// How big it will reach before shrinking back down
		msg.sizeDest = 150.f * textScale;
		msg.sizeRestDest = 40.f * textScale;
		msg.startSize = msg.size;
	
		// Use text as dummy to scale widthBefore to resting size
		msg.txt->setString(msg.msg);
		msg.txt->setCharacterSize(uint32_t(msg.sizeRestDest));
		msg.widthBefore = msg.txt->getGlobalBounds().width;
		msg.heightBefore = msg.txt->getGlobalBounds().height;
	
		// Set the starting size
		msg.txt->setCharacterSize(uint32_t(msg.startSize));
	}
	else
	{
		msg.txt->setString(msg.msg);
		msg.txt->setCharacterSize(uint32_t(msg.size));
		msg.widthBefore = msg.txt->getGlobalBounds().width;
		msg.heightBefore = msg.txt->getGlobalBounds().height;
	}
	
	// Center it
	msg.startPosf = sf::Vector2f(sf::Vector2i(pos.x - (int(msg.widthBefore) / 2), pos.y));
	
	// Text gets moved over to the side if there's no room
	auto& messagesList = m_combatMessages[sourceGuid];

	if (!messagesList.empty() && !msg.floatSideways)
	{
		if (atoi(msg.msg.c_str()) == 0)
		{
			for (int i = messagesList.size() - 1; i >= 0; --i)
			{
				auto& top = messagesList[i];
				
				// Origin point too different
				if (top.sourceGuid != msg.sourceGuid && top.objStartPos.getDist(msg.objStartPos) > 0.5f)
					continue;

				if (top.floatSideways)
					continue;

				if (abs(top.startPosf.y - msg.startPosf.y) > 5.f)
					break;

				if (makeRoomAtSide)
				{
					msg.movedSideways = true;

					if (top.movedSideways)
					{
						// If we're pushed to the side by an already sideways moved text, then start stacking downward
						msg.startPosf.x = top.startPosf.x;
						msg.startPosf.y += top.heightBefore;
					}
					else
					{					
						msg.startPosf.x = float(top.startPosf.x) + float(top.widthBefore) + float(top.txt->getCharacterSize() / 5);
					}
				}

				break;
			}
		}
	}

	// After any X changes 
	msg.txt->setPosition(msg.startPosf);

	if (!msg.movedSideways && !msg.floatSideways && pushUpward)
	{
		// Move all messages upward from the same source
		for (auto& itr : messagesList)
		{				
			if (itr.sourceGuid != msg.sourceGuid && itr.objStartPos.getDist(msg.objStartPos) > 0.5f)
				continue;

			if (itr.floatSideways)
				continue;

			if (std::clock() - itr.startDate > 700)
				continue;
			
			itr.startPosf.y -= msg.heightBefore;
		}
	}

	messagesList.push_back(msg);
}

void World::pushScrollingMessage(const string& str, const sf::Color color /*= sf::Color::White*/)
{
	if (m_topCenterMsgs.size() >= 3)
		m_topCenterMsgs.erase(m_topCenterMsgs.end() - 1);

	ScrollingMsg msg;
	msg.txt = make_shared<TextBox>(sContentMgr->getFont(FontId::FrizBold), 16);
	msg.txt->setData(0, 0, str, sApplication->sW(), TextBox::AlignCenterBounds, true);
	msg.txt->setColor(color);
	msg.txt->getTextRef().setShadowOffset(2);
	msg.alphaPct = m_topcenterStartPct;
	msg.originalColor = color;

	m_topCenterMsgs.insert(m_topCenterMsgs.begin(), msg);

	// We might need to adjust old ones, which would affect where this new one would go
	computeTopCenterMsgPositions();
}

void World::computeTopCenterMsgPositions()
{
	// Adjust others new positions accordingly
	for (size_t i = 0; i < m_topCenterMsgs.size(); ++i)
		m_topCenterMsgs[i].txt->moveTo(sf::Vector2i(0, 150 + ((i + 1) * 20)));
}

void World::formatQuestText(string& str, const int questId)
{	
	string startName = "Unknown";
	string endName = "Unknown";

	// $NPC$ and $END$
	auto fill = [&](map<int, pair<int, MutualObject::Type>>& cache, string& output)
	{
		auto itr = cache.find(questId);

		if (itr != cache.end())
		{
			if (itr->second.second == MutualObject::Type::Npc)
				output = sContentMgr->db("npc_template").data(itr->second.first, "name");
			else if (itr->second.second == MutualObject::Type::GameObject)
				output = sContentMgr->db("gameobject_template").data(itr->second.first, "name");
		}
	};
	
	fill(m_questStartCache, startName);
	fill(m_questEndCache, endName);
	
	Util::strReplaceAll(str, "$END$", endName);
	Util::strReplaceAll(str, "\r", "\n");

	// Can avoid duplicating code like this
	formatWorldText(str, startName);
}

void World::formatWorldText(string& str, const string& objName)
{
	string playerName = "Unknown";
	string classname = "Unknown";
	string classnameLower = "unknown";
	PlayerDefines::Gender gender = PlayerDefines::Gender::Male;

	if (m_myself != nullptr)
	{
		playerName = m_myself->getName();		
		gender = m_myself->getGender();
		classname = PlayerFunctions::className(m_myself->getClass());
		classnameLower = classname;
		transform(classnameLower.begin(), classnameLower.end(), classnameLower.begin(), ::tolower);
	}

	Util::strReplaceAll(str, "$NPC$", objName.empty() ? "Unknown" : objName);
	Util::strReplaceAll(str, "$PLAYER$", playerName);
	Util::strReplaceAll(str, "$CLASS$", classname);
	Util::strReplaceAll(str, "$class$", classnameLower);
	
	size_t gpos = 0;

	// $G=his:her$, ends in dollar sign, starts with $G=
	while ((gpos = str.find("$G=", gpos + 1)) != string::npos)
	{
		string firstWord;
		size_t i = gpos + 3;

		for (; i < str.size() && str[i] != ':'; ++i)
			firstWord.push_back(str[i]);

		++i;

		string secondWord;

		for (; i < str.size() && str[i] != '$'; ++i)
			secondWord.push_back(str[i]);

		if (i < str.size() && str[i] == '$')
			str.replace(gpos, i - gpos + 1, gender == PlayerDefines::Gender::Male ? firstWord : secondWord);
	}
}

void World::addClientObject(shared_ptr<ClientObject> obj)
{
	// Never have duplicate guid's
	removeClientObject(obj->getGuid());

	m_map->addWorldRenderable(obj);
	m_objects[obj->getGuid()] = obj;

	// Maintain fade when restoring an object that was being deleted
	float fadePct = 0.f;

	auto itr = m_fadingObjects.find(obj->getGuid());

	if (itr != m_fadingObjects.end())
	{ 
		fadePct = itr->second.second;
		m_map->removeWorldRenderable(itr->second.first);
		m_fadingObjects.erase(itr);
	}

	obj->setFadeInPct(fadePct);
}

void World::removeClientObject(const int guid)
{
	if (guid == getSelectedGuid())
		setSelectedGuid(0);

	auto itr = m_objects.find(guid);

	if (itr == m_objects.end())
		return;

	// Uhhh??
	if (itr->second == m_myself)
		m_myself = nullptr;

	m_objects.erase(itr);
	m_fadingObjects[itr->second->getGuid()] = { itr->second, 1.f };
}

void World::setGrabbedIcon(shared_ptr<GameIcon> icon)
{
	if (m_grabbedIcon == icon)
		return;

	if (m_grabbedIcon != nullptr)
		m_grabbedIcon->release(*this);

	m_grabbedIcon = icon;

	if (icon == nullptr)
	{
		m_grabbedIconCopy = nullptr;
	}
	else
	{
		if (icon->getType() == GameIcon::Type::Item)
			m_grabbedIconCopy = make_shared<ItemIcon>(*this, icon->getEntry(), "gameicon32");
		else
			m_grabbedIconCopy = make_shared<SpellIcon>(*this, icon->getEntry(), "gameicon32");
		
		m_grabbedIconCopy->changeEntry(icon->getEntry());
		m_grabbedIconCopy->setMouseable(false);
	}
}

void World::addWorldSpellAnimation(shared_ptr<WorldSpellAnimation> ptr)
{
	m_map->addWorldRenderable(ptr);
}

void World::setSelectedGuid(const int guid)
{
	if (getSelectedGuid() == guid)
		return;

	m_selectedUnit = dynamic_pointer_cast<ClientUnit>(getClientObject(guid));

	if (auto unitFrame = dynamic_pointer_cast<UnitFrame>(getRenderObject(Interface::TargetUnitFrame)))
	{
		// Null is a valid value
		unitFrame->setUnit(m_selectedUnit);

		if (m_selectedUnit != nullptr)
			sContentMgr->playSound(SfxId::WindowTargetOpen);
		else
			sContentMgr->playSound(SfxId::WindowTargetClose);
		
		GP_Client_SetSelected packet;
		packet.m_guid = guid;
		sConnector->sendPacket(packet.build(StlBuffer{}));
	}
}

int World::getSelectedGuid() const
{
	if (m_selectedUnit != nullptr)
		return m_selectedUnit->getGuid();

	return 0;
}

int World::getMyselfGuid() const
{
	if (m_myself == nullptr)
		return 0;

	return m_myself->getGuid();
}

void World::requestLevelup()
{
	if (m_myself == nullptr)
		return;
	
	auto equipment = dynamic_pointer_cast<Equipment>(getRenderObject(World::Interface::EquipmentPanel));
	auto abilities = dynamic_pointer_cast<Abilities>(getRenderObject(World::Interface::AbilitiesPanel));

	// Spell rank-ups require a SpendExp (op41) per spell BEFORE the LevelUp (op40) commit — this is the
	// original-client flow (capture conn1: op41 {spell,targetRank} then op40 {spell,points}). Without it the
	// server drops the spell side of the level-up. rank sent = the spell's TARGET rank (current + points).
	for (const auto& inv : abilities->getDesiredInvestments())
	{
		if (inv.second == 0)
			continue;

		GP_Server_Spellbook::SpellSlot slot;
		const int targetRank = (abilities->getSpellPoints(inv.first, slot) ? slot.level : 0) + inv.second;

		GP_Client_SpellRankInvest rankPk;
		rankPk.m_spellId = inv.first;
		rankPk.m_rank = targetRank;
		sConnector->sendPacket(rankPk.build(StlBuffer{}));
	}

	GP_Client_LevelUp pk;
	pk.m_spellInvestments = abilities->getDesiredInvestments();
	pk.m_statInvestments = equipment->getPendingStatInvestments();

	sConnector->sendPacket(pk.build(StlBuffer{}));
}

void World::computePendingLevelupCost() 
{
	m_pendingLevelupCost = 0;

	if (m_myself == nullptr)
		return;
	
	auto equipment = dynamic_pointer_cast<Equipment>(getRenderObject(World::Interface::EquipmentPanel));
	auto abilities = dynamic_pointer_cast<Abilities>(getRenderObject(World::Interface::AbilitiesPanel));
	m_pendingLevelupCost = m_myself->computeLevelupCost(equipment->getPendingStatInvestments(), abilities->getDesiredInvestments());
}

bool World::isIconGrabbed() const
{
	return m_grabbedIcon != nullptr && m_grabbedIcon->getEntry() != 0;
}

void World::updateCameraDrag()
{
	m_cameraDrag->updateDraggableObject(*m_map, true);
	sApplication->getWindow().setMouseCursorVisible(!m_cameraDrag->isGrabbed());

	if (m_cameraOffset.x > 300.f)
	{
		float overShoot = m_cameraOffset.x - 300.f;

		m_cameraOffset.x -= overShoot;
		m_map->getCameraRef().x -= overShoot;
	}

	if (m_cameraOffset.y > 300.f)
	{
		float overShoot = m_cameraOffset.y - 300.f;

		m_cameraOffset.y -= overShoot;
		m_map->getCameraRef().y -= overShoot;
	}

	if (m_cameraOffset.x < -300.f)
	{
		float overShoot = -300.f - m_cameraOffset.x;

		m_cameraOffset.x += overShoot;
		m_map->getCameraRef().x += overShoot;
	}

	if (m_cameraOffset.y < -300.f)
	{
		float overShoot = -300.f - m_cameraOffset.y;

		m_cameraOffset.y += overShoot;
		m_map->getCameraRef().y += overShoot;
	}

	// Snap to the center within 100 readius
	if (!m_cameraDrag->isGrabbed() && abs(m_cameraOffset.x) < 100.f && abs(m_cameraOffset.y) < 100.f)
		m_cameraOffset = { 0.f, 0.f };
}

void World::cacheGossipOptions()
{
	shared_ptr<SqlConnector> db = SqlConnector::create(SqlType::SQLite, sConfig->getString("GameDb", "Name"), sConfig->getString("GameDb", "Path"));

	if (auto result = db->query("SELECT entry, text FROM gossip_option"))
	{
		for (auto& itr : result->fetchData())
		{
			auto entry = itr[0].getInt32();
			m_gossipOptions[entry] = itr[1].getString();
		}
	}

	if (auto result = db->query("SELECT entry, text, text FROM gossip"))
	{
		for (auto& itr : result->fetchData())
		{
			auto entry = itr[0].getInt32();
			m_gossips[entry] = itr[1].getString();
		}
	}
}

void World::cacheQuests()
{		
	shared_ptr<SqlConnector> db = SqlConnector::create(SqlType::SQLite, sConfig->getString("GameDb", "Name"), sConfig->getString("GameDb", "Path"));

	if (auto result = db->query("SELECT entry, start_npc_entry FROM quest_template"))
	{
		for (auto& itr : result->fetchData())
		{
			auto quest = itr[0].getInt32();
			auto entry = itr[1].getInt32();
			m_questStartCache[quest] = { entry, MutualObject::Type::Npc };
		}
	}

	if (auto result = db->query("SELECT entry, finish_npc_entry FROM quest_template"))
	{
		for (auto& itr : result->fetchData())
		{
			auto quest = itr[0].getInt32();
			auto entry = itr[1].getInt32();
			m_questEndCache[quest] = { entry, MutualObject::Type::Npc };
		}
	}

	if (auto result = db->query("SELECT entry, map, position_x, position_y FROM npc WHERE entry IN (SELECT start_npc_entry FROM quest_template) OR entry IN (SELECT finish_npc_entry FROM quest_template)"))
	{
		for (auto& itr : result->fetchData())
		{
			auto entry = itr[0].getInt32();
			auto mapId = itr[1].getInt32();
			auto position_x = itr[2].getFloat();
			auto position_y = itr[3].getFloat();
			m_questRelatedObjects[mapId][{ entry, MutualObject::Type::Npc }].push_back(sf::Vector2f(position_x, position_y));
		}
	}
}
		
void World::getObjectPositions(const int mapId, const int entry, const MutualObject::Type type, vector<sf::Vector2f>& output)
{
	auto itr = m_questRelatedObjects.find(mapId);

	if (itr != m_questRelatedObjects.end())
	{
		auto subItr = itr->second.find({entry, type});

		if (subItr != itr->second.end())
			output = subItr->second;
	}
}

shared_ptr<Button> World::makeBtnBindOnly(shared_ptr<Button> input) const
{
	input->setAllowLeftClick(false);
	input->setAllowRightClick(false);

	return input;
}

pair<int, MutualObject::Type> World::getQuestStartInfo(const int questId) const
{
	auto itr = m_questStartCache.find(questId);

	if (itr != m_questStartCache.end())
		return itr->second;

	return {};
}
		
pair<int, MutualObject::Type> World::getQuestEndInfo(const int questId) const
{
	auto itr = m_questEndCache.find(questId);

	if (itr != m_questEndCache.end())
		return itr->second;
	
	return {};
}

void World::refreshToolbarTooltips()
{
	dynamic_pointer_cast<Toolbar>(getRenderObject(World::Toolbar1))->refreshTooltips();
	dynamic_pointer_cast<Toolbar>(getRenderObject(World::Toolbar2))->refreshTooltips();
	dynamic_pointer_cast<Toolbar>(getRenderObject(World::Toolbar3))->refreshTooltips();
}
	
void World::onLevelTo(const int lvl)
{
	if (myself() != nullptr)
		myself()->setVariable(ObjDefines::Variable::Level, lvl);

	dynamic_pointer_cast<LevelupNotify>(getRenderObject(World::LevelupNotifyObj))->onLevelTo(lvl);
	dynamic_pointer_cast<Abilities>(getRenderObject(World::AbilitiesPanel))->refreshTooltips();
	dynamic_pointer_cast<Inventory>(getRenderObject(World::InventoryPanel))->refreshTooltips();
	refreshToolbarTooltips();
}
		
void World::exclaimHint(const Hint hint)
{
	if (hint == Hint::SpendExp)
	{
		if (!isPanelOpen(World::Interface::EquipmentPanel))
			dynamic_pointer_cast<Button>(getRenderObject(Interface::EquipmentButton))->setExclaimNotice(true);

		auto levelupbtn = dynamic_pointer_cast<Button>(m_equipment->getRenderObject(Equipment::Interface::LevelUp));

		if (!levelupbtn->isHidden())
			levelupbtn->setExclaimNotice(true);
	}
}
	
void World::flushCombatMsgs(const int sourceGuid)
{
	m_combatMessages[sourceGuid].clear();
}
		
void World::setParty(const vector<int>& mbrs, const int leaderGuid)
{
	// mbrs includes ourselves, m_partyMembers does not
	int numAfter = mbrs.size() - 1;
	int numBefore = 0;

	for (auto& itr : m_partyMembers)
	{
		if (itr != 0)
			++numBefore;
	}

	// New party member
	if (numAfter > numBefore)
		sContentMgr->playSound(SfxId::AlertParty);

	// Removed party member
	else if (numAfter < numBefore)
		sContentMgr->playSound(SfxId::ItemGenMove);

	// New leader
	else if (m_partyLeaderGuid != leaderGuid)
		sContentMgr->playSound(SfxId::AlertHelpIndicator);

	m_partyMembers = mbrs;
	m_partyLeaderGuid = leaderGuid;

	if (m_myself == nullptr)
		return;

	// No need to include myself
	for (auto itr = m_partyMembers.begin(); itr != m_partyMembers.end(); ++itr)
	{
		if (*itr == m_myself->getGuid())
		{
			m_partyMembers.erase(itr);
			break;
		}
	}

	// Start fresh
	for (auto& itr : m_partyFrames)
	{
		if (itr)
		{
			itr->setOffline(false);
			itr->setUnit(nullptr);
		}
	}
	
	// Fill in empty slots for an easy loop below
	while (m_partyMembers.size() < m_partyFrames.size())
		m_partyMembers.push_back(0);
	
	for (size_t i = 0; i < m_partyMembers.size(); ++i)
	{
		auto& frameObj = m_partyFrames[i];
		int guid = m_partyMembers[i];
		auto unit = dynamic_pointer_cast<ClientUnit>(getClientObject(guid));

		if (guid != 0 && unit == nullptr)
			frameObj->setOffline(true);

		frameObj->setUnit(unit);
	}
}

void World::chatError(const int chatError)
{
	auto gameChat = dynamic_pointer_cast<GameChat>(getRenderObject(World::GameChatBox));
	ASSERT(gameChat);

	switch (chatError)
	{
		case PlayerDefines::ChatError::ChatIgnored: gameChat->addLine("That player has ignored you", ChatDefines::Channels::System); break;
		case PlayerDefines::ChatError::CantAcceptQuest: gameChat->addLine("Can't accept that quest", ChatDefines::Channels::System); break;
		case PlayerDefines::ChatError::TargetBusy: gameChat->addLine("Your target is busy right now", ChatDefines::Channels::System); break;
		case PlayerDefines::ChatError::DuelDeclined: gameChat->addLine("Your duel was declined", ChatDefines::Channels::System); break;
		case PlayerDefines::ChatError::DuelCount3: gameChat->addLine("The duel will begin in 3 seconds", ChatDefines::Channels::System); break;
		case PlayerDefines::ChatError::DuelCount2: gameChat->addLine("The duel will begin in 2 seconds", ChatDefines::Channels::System); break;
		case PlayerDefines::ChatError::DuelCount1: gameChat->addLine("The duel will begin in 1 second", ChatDefines::Channels::System); break;
		case PlayerDefines::ChatError::DuelBegin: gameChat->addLine("The duel has begun!", ChatDefines::Channels::System); break;			
		case PlayerDefines::ChatError::PlayerNotFound: gameChat->addLine("Player not found.", ChatDefines::Channels::System); break;
		case PlayerDefines::ChatError::PlayerGroupedAlready: gameChat->addLine("Player is already grouped.", ChatDefines::Channels::System); break;
		case PlayerDefines::ChatError::PartyDeclined: gameChat->addLine("Invitation was declined.", ChatDefines::Channels::System); break;
		case PlayerDefines::ChatError::PartyFull: gameChat->addLine("Party is full.", ChatDefines::Channels::System); break;
		case PlayerDefines::ChatError::InstanceFull: gameChat->addLine("Instance is full.", ChatDefines::Channels::System); break;
		case PlayerDefines::ChatError::ErrorTransferingMap: gameChat->addLine("Error transferring to map.", ChatDefines::Channels::System); break;
		case PlayerDefines::ChatError::CantInviteSelf: gameChat->addLine("You cannot invite yourself.", ChatDefines::Channels::System); break;
		case PlayerDefines::ChatError::AlreadyInGuild: gameChat->addLine("You are already in a guild.", ChatDefines::Channels::System); break;
		case PlayerDefines::ChatError::TargetAlreadyInGuild: gameChat->addLine("That player is already in a guild.", ChatDefines::Channels::System); break;
		case PlayerDefines::ChatError::NotInGuild: gameChat->addLine("You are not in a guild.", ChatDefines::Channels::System); break;
		case PlayerDefines::ChatError::InsufficientGuildRank: gameChat->addLine("You do not have permission to do that.", ChatDefines::Channels::System); break;
		case PlayerDefines::ChatError::GuildDisbanded: gameChat->addLine("Your guild has been disbanded.", ChatDefines::Channels::System); break;
		case PlayerDefines::ChatError::CantLeaveAsLeader: gameChat->addLine("You must transfer leadership before leaving the guild.", ChatDefines::Channels::System); break;
		case PlayerDefines::ChatError::TradeInvalid: gameChat->addLine("Trade aborted. Inventories failed validation.", ChatDefines::Channels::System); break;
		case PlayerDefines::ChatError::GuildNameTaken: gameChat->addLine("A guild with that name already exists.", ChatDefines::Channels::System); break;
		case PlayerDefines::ChatError::GuildFull: gameChat->addLine("Your guild is full.", ChatDefines::Channels::System); break;
		case PlayerDefines::ChatError::InstanceInCombat: gameChat->addLine("Unable to join instance while combat is in progress.", ChatDefines::Channels::System); break;
			
		default: gameChat->addLine("Unknown Error.", ChatDefines::Channels::System); break;
	}
}

void World::error(const int worldError)
{
	switch (worldError)
	{
		case PlayerDefines::WorldError::SpellRequirementNotMet: pushScrollingMessage("Requirement for that spell is not met", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::NotHighEnoughLevel: pushScrollingMessage("You are not a high enough level", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::MissingItem: pushScrollingMessage("Missing an item", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::BuybackEmpty: pushScrollingMessage("Nothing to buy back", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::EnchantLimit: pushScrollingMessage("Maximum potential reached for an item of that quality", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::NoEmptySockets: pushScrollingMessage("No empty sockets in that item", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::VendorNotWorth: pushScrollingMessage("The vendor doesn't want that item", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::RewardNotChosen: pushScrollingMessage("Choose a reward", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::MaximumQuestsTracked: pushScrollingMessage("Reached Maximum tracked quests", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::InventoryFull: pushScrollingMessage("Inventory is full", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::QuestNotDone: pushScrollingMessage("Quest not complete", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::Stunned: pushScrollingMessage("Can't do that while Stunned", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::Confused: pushScrollingMessage("Can't do that while Confused", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::Feared: pushScrollingMessage("Can't do that while Feared", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::Sleeping: pushScrollingMessage("Can't do that while Sleeping", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::Incapacitated: pushScrollingMessage("Can't do that while Incapacitated", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::Polymorphed: pushScrollingMessage("Can't do that while Polymorphed", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::Pacified: pushScrollingMessage("Can't do that while Pacified", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::Charging: pushScrollingMessage("Can't do that while Charging", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::Rooted: pushScrollingMessage("Can't do that while Rooted", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::Silenced: pushScrollingMessage("Can't do that while Silenced", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::CantEquipItem: pushScrollingMessage("Unable to Equip Item", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::NotWhileInCombat: pushScrollingMessage("You're in Combat", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::SpellNotReady: pushScrollingMessage("Spell isn't ready", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::CantWhileMoving: pushScrollingMessage("Can't do that while moving", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::LineOfSight: pushScrollingMessage("Not in Line of Sight", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::CasterDead: pushScrollingMessage("You are Dead", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::EquipItemRequired: pushScrollingMessage("Missing required Equiped Item", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::TargetDead: pushScrollingMessage("Target is Dead", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::InvalidTarget: pushScrollingMessage("Invalid Target", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::NotWhileTargetInCombat: pushScrollingMessage("Target's in Combat", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::TargetPlayersOnly: pushScrollingMessage("Can only Target Players", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::OutOfRange: pushScrollingMessage("Out of Range", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::TooClose: pushScrollingMessage("You are too close", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::EquipItemBroken: pushScrollingMessage("Required Item is Broken", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::NotEnoughMana: pushScrollingMessage("Not enough Mana", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::CastInProgress: pushScrollingMessage("Cast already in progress", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::OutOfStock: pushScrollingMessage("Out of stock", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::NotEnoughGold: pushScrollingMessage("Not enough gold", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::PerceptionFail: pushScrollingMessage("Perception skill too low", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::AlreadyLooted: pushScrollingMessage("Already looted", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::NotEnoughExp: pushScrollingMessage("Not enough Experience", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::MagicSchoolLock: pushScrollingMessage("Unable to use that School of Magic", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::LockpickTooLow: pushScrollingMessage("Lockpicking not high enough", sf::Color(0xff8000ff)); break;
		case PlayerDefines::WorldError::WrongClass: pushScrollingMessage("You are not the right Class", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::AlreadyKnowSpell: pushScrollingMessage("You already know that ability", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::ItemIsSoulBound: pushScrollingMessage("That item is soulbound", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::RequiresLockPicking: pushScrollingMessage("It's locked, try using Pick Lock", sf::Color(0xff8000ff)); break;
		case PlayerDefines::WorldError::CantInArena: pushScrollingMessage("You can't do that in an Arena.", sf::Color(0x9f0c12ff)); break;
		case PlayerDefines::WorldError::CantInDungeon: pushScrollingMessage("You can't do that in a Dungeon.", sf::Color(0x9f0c12ff)); break;
		default: pushScrollingMessage("Unknown Error", sf::Color(0x9f0c12ff)); break;
	}

	sContentMgr->playSound(SfxId::AlertExchangeDelete);
}