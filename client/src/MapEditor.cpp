#include "stdafx.h"
#include "MapEditor.h"
#include "ContentMgr.h"
#include "Application.h"
#include "TickBox.h"
#include "TextLines.h"
#include "Button.h"
#include "ScrollBar.h"
#include "ConfirmMessageBox.h"
#include "ClientMap.h"
#include "SpriteRo.h"
#include "MapCellClient.h"
#include "Sprite.h"
#include "ClientNpc.h"
#include "SpriteScript.h"
#include "PopupPrompt.h"
#include "ClientGameObj.h"
#include "Minimap.h"
#include "DbTemplateEditor.h"

#include "..\Math.h"
#include "..\Files.h"
#include "..\Rand.h"
#include "..\StringHelpers.h"
#include "..\Shared\MapCellT.h"
#include "..\Shared\Config.h"
#include "..\Shared\NpcDefines.h"
#include "..\SqlConnector\QueryResult.h"
#include "..\SqlConnector\SqlConnector.h"
#include "..\Shared\MapLogic.h"

MapEditor::MapEditor(RenderObject& owner, const int id) :
	RenderObjectHolder(&owner, id),
	m_spriteSelectedTile(sContentMgr->spawnSprite("mapeditor_selected_tile.png")),
	m_spriteSelectedTerrain(sContentMgr->spawnSprite("map_blank_terrain.png")),
	m_font(sContentMgr->getFont(FontId::Arial)),
	m_previewSpriteSpot(440.0f, 330.0f),
	m_lastScale(1.f, 1.f)
{
	setMultiInput(true);

	ASSERT(m_spriteSelectedTile != nullptr && m_spriteSelectedTerrain != nullptr);
	m_spriteSelectedTile->setHotspotEasy(true, false);
	m_spriteSelectedTerrain->setHotspotEasy(true, false);

	m_tileinfoText = make_unique<Text>(m_font);
	m_tileinfoText->setCharacterSize(15);

	m_layerIdentifierText = make_unique<Text>(m_font);
	m_layerIdentifierText->setCharacterSize(15);

	m_zoneIdentifierText = make_unique<Text>(m_font);
	m_zoneIdentifierText->setCharacterSize(15);

	m_areaIdentifierText = make_unique<Text>(m_font);
	m_areaIdentifierText->setCharacterSize(15);

	m_spriteScaleText = make_unique<Text>(m_font);
	m_spriteScaleText->setCharacterSize(15);

	m_spriteMinimapMyLocX = sContentMgr->spawnSprite("quest_kill_map.png");
	m_spriteMinimapMyLocX->setHotspotEasy(true, true);

	// Terrain definition
	vector<string> lines;
	Util::readLinesFromFile("scripts\\mapeditor_defs\\terrain.txt", lines);

	for (auto& itr : lines)
		m_terrainDef.insert(itr);

	// Texture name library
	lines.clear();
	Util::readLinesFromFile("scripts\\mapeditor_defs\\texture_names.txt", lines);

	for (auto& line : lines)
	{
		vector<string> tokens;
		stringstream check1(line);

		string intermediate;

		// Tokenizing
		while (getline(check1, intermediate, '='))
			tokens.push_back(intermediate);

		if (tokens.size() == 2)
		{
			m_textureNameLibrary[tokens[0]] = tokens[1];
			m_textureNameCache[tokens[1]] = tokens[0];
		}
	}

	// The map that we will be editing
	m_map = make_shared<ClientMap>(*this, Interface::GameMap);
	m_map->setRenderEmptyCells(true);
	m_map->setAllowCameraDrag(true);
	m_map->setMouseable(true);
	addRenderObject(m_map);

	// Background
	m_spriteBg = make_shared<SpriteRo>(*this, sContentMgr->spawnSprite("map_editor.png"), 0);
	m_spriteBg->setPos({ 5, 5 });
	addRenderObject(m_spriteBg);

	// Texture browser
	m_textureList = make_shared<TextLines>(*this, Interface::TextureBrowser, FontId::Arial, Util::GeoBox2d(22, 125, 245, 384));
	m_textureList->setClickableLines(true);
	m_textureList->setDialogCharacterSize(12);
	m_textureList->setMaxLines(4096);
	addRenderObject(m_textureList);

	// Given to the texture browser
	auto scrollBar = make_shared<ScrollBar>(*m_textureList, "mapeditor_scroll_up", "mapeditor_scroll_down", ScrollBar::ScrollTopDown, "mapeditor_scroll_box");
	scrollBar->getScrollUpButton()->setPos({ 293, 107 });
	scrollBar->getScrollDownButton()->setPos({ 293, 517 });
	scrollBar->getScrollSquare()->setPos({ 293, 517 });
	m_textureList->setScrollObject(scrollBar);

	// Day night slider
	scrollBar = make_shared<ScrollBar>(*this, "side_scroll_up", "side_scroll_down", ScrollBar::ScrollLeftRight, "side_scroll_box", Interface::DayNightSlider);
	scrollBar->getScrollDownButton()->setPos(sf::Vector2i(325, 414));
	scrollBar->getScrollUpButton()->setPos(sf::Vector2i(515, 414));
	scrollBar->setMaxOffset(100);
	scrollBar->setScrollOffset(100);
	scrollBar->setAllowMousewheel(false);
	addRenderObject(scrollBar);

	// Layer ticks
	addRenderObject(make_shared<TickBox>(*this, Interface::Layer1_Tick, "tick_box_empty", "tick_box_full", sf::Vector2i(12,57), true));
	addRenderObject(make_shared<TickBox>(*this, Interface::Layer2_Tick, "tick_box_empty", "tick_box_full", sf::Vector2i(12 + 30, 57), true));
	addRenderObject(make_shared<TickBox>(*this, Interface::Layer3_Tick, "tick_box_empty", "tick_box_full", sf::Vector2i(12 + 60, 57), true));

	// Tile property display ticks
	addRenderObject(make_shared<TickBox>(*this, Interface::ShowUnwalkable_Tick, "tick_box_empty", "tick_box_full",	sf::Vector2i(130, 57), true, SfKeyEvent(sf::Keyboard::F4)));
	addRenderObject(make_shared<TickBox>(*this, Interface::ShowBlocking_Tick, "tick_box_empty", "tick_box_full", sf::Vector2i(130 + 30, 57), true, SfKeyEvent(sf::Keyboard::F5)));
	addRenderObject(make_shared<TickBox>(*this, Interface::Grid_Tick, "tick_box_empty", "tick_box_full", sf::Vector2i(130 + 60, 57), true, SfKeyEvent(sf::Keyboard::F6)));

	// Whether or not we're editing terrain is a tick, but have it turned off by default
	m_editingTerrainTick = make_shared<TickBox>(*this, Interface::EditTerrain_Tick, "tick_box_empty", "tick_box_full", sf::Vector2i(430, 92), true);
	m_editingTerrainTick->setTicked(false);
	addRenderObject(m_editingTerrainTick);

	// Whether or not to highlight cells that are occupied with data
	auto tickbox = make_shared<TickBox>(*this, Interface::RevealOccupied, "tick_box_empty", "tick_box_full", sf::Vector2i(250, 57), true, SfKeyEvent(sf::Keyboard::R));
	tickbox->setTicked(false);
	tickbox->setPlayButtonOnKeyboardPress(true);
	addRenderObject(tickbox);

	// Buttons 
	addRenderObject(make_shared<Button>(*this, "mapeditor_hand", Interface::HandButton, sf::Vector2i(548, 11), SfKeyEvent(sf::Keyboard::H)));
	addRenderObject(make_shared<Button>(*this, "mapeditor_fill", Interface::TerrainButton, sf::Vector2i(505, 11), SfKeyEvent(sf::Keyboard::F, false, true)));
	addRenderObject(make_shared<Button>(*this, "mapeditor_save", Interface::SaveButton, sf::Vector2i(462, 11), SfKeyEvent(sf::Keyboard::S, false, true)));
	addRenderObject(make_shared<Button>(*this, "generic_highlight_small", Interface::ZoneButton, sf::Vector2i(520, 50)));
	addRenderObject(make_shared<Button>(*this, "generic_highlight_small", Interface::AreaButton, sf::Vector2i(520, 70)));

	auto returnbutton = make_shared<Button>(*this, "mapeditor_return", Interface::BrowserReturnButton, sf::Vector2i(275, 100), SfKeyEvent(sf::Keyboard::BackSpace));
	returnbutton->setPlayButtonOnKeyboardPress(true);
	addRenderObject(returnbutton);

	// Start off with a "folder" listing
	setBrowserStage(BrowserStages::Directory);
}

MapEditor::~MapEditor()
{

}

void MapEditor::input()
{
	if (m_dbEditor != nullptr)
	{
		m_dbEditor->attemptInput();

		if (m_dbEditor->wantsClose())
		{
			m_dbEditor = nullptr;
			verifyDbObjects();
			setBrowserStage(MapEditor::BrowserStages::Directory);
		}

		return;
	}

	sApplication->getWindow().setKeyRepeatEnabled(true);
	RenderObjectHolder::input();
	
	if (m_map != nullptr)
	{
		float x, y;
		m_map->getCellWorldPosRelative(x, y, { sApplication->mousePosf(true).x, sApplication->mousePosf(true).y }, true);
		saveCameraPosition();
	}

	/**
	* Key binds
	*/

	if (sApplication->popKeyUp(sf::Keyboard::Escape))
	{
		m_spriteGrabbed = nullptr;
		m_objGrabbed = nullptr;
		m_objMoving = nullptr;
	}

	if (sApplication->popKeyUp(sf::Keyboard::F1))
	{
		if (m_editingLayer != 0)
			sContentMgr->playSound(SfxId::ButtonChange);

		m_editingLayer = 0;
	}
	else if (sApplication->popKeyUp(sf::Keyboard::F2))
	{
		if (m_editingLayer != 1)
			sContentMgr->playSound(SfxId::ButtonChange);

		m_editingLayer = 1;
	}
	else if (sApplication->popKeyUp(sf::Keyboard::F3))
	{
		if (m_editingLayer != 2)
			sContentMgr->playSound(SfxId::ButtonChange);

		m_editingLayer = 2;
	}
	else if (sApplication->popKeyUp(sf::Keyboard::M))
	{
		if (m_minimapSprite.getTexture() == nullptr)
		{
			sApplication->spawnPopup("Minimap texture not ready.", ConfirmMessageBox::ConfirmBox_Ok, ConfirmMessageBox::ConfirmCode_Null);
		}
		else
		{
			sContentMgr->playSound(SfxId::ButtonChange);
			m_showMinimap = !m_showMinimap;
		}
	}

	m_editingLayer = min(m_editingLayer, ClientMap::Defines::NumLayers - 1);

	/**
	* Texture Browser
	*/

	string clickedLine = m_textureList->popPendingClickedLine();
	m_textureList->getScrollObject()->setAllowMousewheel(!m_map->isMousedOver());

	if (!clickedLine.empty())
	{
		if (m_browserStage == BrowserStages::Directory && clickedLine.find("npc_template") != string::npos)
			setBrowserStage(BrowserStages::NpcList, clickedLine);
		else if (m_browserStage == BrowserStages::Directory && clickedLine.find("gameobject_template") != string::npos)
			setBrowserStage(BrowserStages::GoList, clickedLine);
		else if (m_browserStage == BrowserStages::Directory)
			setBrowserStage(BrowserStages::TextureList, clickedLine);
		else if (m_browserStage == BrowserStages::TextureList)
			m_spritePreview = sContentMgr->spawnSprite(textureName(clickedLine));
		else if (m_browserStage == BrowserStages::NpcList)
			m_objPreview = createNpc(atoi(clickedLine.c_str()));
		else if (m_browserStage == BrowserStages::GoList)
			m_objPreview = createGameObj(atoi(clickedLine.c_str()));

		m_textureList->setSelectedLine(clickedLine);
		sContentMgr->playSound(SfxId::ButtonChange);
	}

	auto selectedScroll = [&](const int dir)
	{
		int line = max(m_textureList->getSelectedLineId() + dir, 0);

		if (auto linePtr = m_textureList->getLine(line))
		{
			string newLine = linePtr->getTextStr();
			m_textureList->setSelectedLine(newLine.c_str());

			if (m_browserStage == BrowserStages::NpcList)
				m_objPreview = createNpc(atoi(newLine.c_str()));
			else if (m_browserStage == BrowserStages::GoList)
				m_objPreview = createGameObj(atoi(newLine.c_str()));
			else
				m_spritePreview = sContentMgr->spawnSprite(textureName(newLine));

			m_textureList->getScrollObject()->setScrollOffset(m_textureList->getScrollOffset() + dir);
			sContentMgr->playSound(SfxId::ButtonChange);
		}
	};

	// Up
	if (!m_textureList->getSelectedLine().empty() && sApplication->popKeyDown(sf::Keyboard::Up))
	{
		selectedScroll(-1);
	}

	// Down
	if (!m_textureList->getSelectedLine().empty() && sApplication->popKeyDown(sf::Keyboard::Down))
	{
		selectedScroll(1);
	}

	// Random selection
	if (m_browserStage == BrowserStages::TextureList && sApplication->popKeyDown(sf::Keyboard::N))
	{
		string line = m_textureList->getLine(Util::irand(0, m_textureList->getNumLines() - 1))->getTextStr();
		m_spriteGrabbed = sContentMgr->spawnSprite(textureName(line));
		m_spritePreview = sContentMgr->spawnSprite(textureName(line));
		m_textureList->setSelectedLine(line);
		sContentMgr->playSound(SfxId::ButtonChange);

		if (m_lastScale.x >= 0.4)
			m_spriteGrabbed->setScale(m_lastScale);
	}

	/** 
	* Map Tick Boxes
	*/

	if (m_map != nullptr)
	{
		if (auto gridTick = dynamic_pointer_cast<TickBox>(getRenderObject(Interface::Grid_Tick)))
			m_map->setRenderEmptyCells(gridTick->ticked());

		if (auto blockingTick = dynamic_pointer_cast<TickBox>(getRenderObject(Interface::ShowBlocking_Tick)))
			m_map->setRenderBlockingCells(blockingTick->ticked());

		if (auto unwalkableTick = dynamic_pointer_cast<TickBox>(getRenderObject(Interface::ShowUnwalkable_Tick)))
			m_map->setRenderUnwalkableCells(unwalkableTick->ticked());

		if (auto unwalkableTick = dynamic_pointer_cast<TickBox>(getRenderObject(Interface::Layer1_Tick)))
			m_map->setSkipLayerRender(GameMap::Defines::Layer1, !unwalkableTick->ticked());

		if (auto unwalkableTick = dynamic_pointer_cast<TickBox>(getRenderObject(Interface::Layer2_Tick)))
			m_map->setSkipLayerRender(GameMap::Defines::Layer2, !unwalkableTick->ticked());

		if (auto unwalkableTick = dynamic_pointer_cast<TickBox>(getRenderObject(Interface::Layer3_Tick)))
			m_map->setSkipLayerRender(GameMap::Defines::Layer3, !unwalkableTick->ticked());

		if (auto revealTick = dynamic_pointer_cast<TickBox>(getRenderObject(Interface::RevealOccupied)))
		{
			m_map->setRevealOccupied(static_cast<GameMap::Defines>(GameMap::Defines::Layer1), false);
			m_map->setRevealOccupied(static_cast<GameMap::Defines>(GameMap::Defines::Layer2), false);
			m_map->setRevealOccupied(static_cast<GameMap::Defines>(GameMap::Defines::Layer3), false);
			m_map->setRevealOccupied(static_cast<GameMap::Defines>(GameMap::Defines::Layer1 + m_editingLayer), revealTick->ticked());
		}
	}

	/**
	* Buttons
	*/

	switch (popFirstButtonId())
	{
		case Interface::HandButton:
		{
			m_objMoving = nullptr;

			if (m_objPreview != nullptr && (m_browserStage == BrowserStages::NpcList || m_browserStage == BrowserStages::GoList))
			{
				if (auto npc = dynamic_pointer_cast<ClientNpc>(m_objPreview))
					m_objGrabbed = createNpc(npc->getEntry());
				else if (auto goObj = dynamic_pointer_cast<ClientGameObj>(m_objPreview))
					m_objGrabbed = createGameObj(goObj->getEntry());
			}
			else if (m_spritePreview != nullptr)
			{
				m_spriteGrabbed = sContentMgr->spawnSprite(m_spritePreview->getTextureName());
			}

			if (m_spriteGrabbed != nullptr)
				m_spriteGrabbed->setScale(m_lastScale);
			
			break;
		}
		case Interface::ZoneButton:
		{
			sApplication->spawnPopupPrompt("Use the following zone value:", PopupPrompt::Codes::MapeditorSetZone);
			break;
		}
		case Interface::AreaButton:
		{
			sApplication->spawnPopupPrompt("Use the following area value:", PopupPrompt::Codes::MapeditorSetArea);
			break;
		}
		case Interface::BrowserReturnButton:
		{
			if (m_browserStage != BrowserStages::Directory)
				setBrowserStage(BrowserStages::Directory);

			break;
		}
		case Interface::TerrainButton:
		{
			sApplication->spawnPopup("Run terrain script?", ConfirmMessageBox::ConfirmBox_YesNo, ConfirmMessageBox::ConfirmCode_TerrainScript);
			break;
		}
		case Interface::SaveButton:
		{
			if (m_map->getName().size() > 0)
				sApplication->spawnPopup("Save " + m_map->getName() + "?", ConfirmMessageBox::ConfirmBox_YesNo, ConfirmMessageBox::ConfirmCode_SaveMapEditor);
			else
				sApplication->spawnPopup("Map does not have a name.", ConfirmMessageBox::ConfirmBox_Ok, ConfirmMessageBox::ConfirmCode_Null);
			break;
		}
	}

	// Object context menu
	if (sApplication->mouseUp(sf::Mouse::Right))
	{
		if (m_map->isMousedOver())
		{
			if (m_map->getMousedRenderable() != nullptr)
			{
				m_ctxRenderable = m_map->getMousedRenderable();
				sApplication->registerContextMenu(Interface::ObjectContextMenu, getId(), sApplication->mousePos(), { "Rotate", "Delete", "Edit Template", "Respawn Time", "Wander Radius", "Move" });
			}
			else if (auto cell = dynamic_cast<MapCellClient*>(m_map->getCell(getHoveringCell())))
			{
				m_ctxCell = getHoveringCell();
				vector<string> options;

				for (int i = 0; i <= GameMap::Defines::Layer3; ++i)
				{
					const string texname = cell->getTextureName(i);

					if (!texname.empty())
					{
						options.push_back("Select " + friendlyName(texname));
						options.push_back("Resize " + friendlyName(texname));
					}
				}

				if (!options.empty())
					sApplication->registerContextMenu(Interface::SelectTextureContextMenu, getId(), sApplication->mousePos(), options);
			}
		}
		else if (m_spritePreview != nullptr)
		{
			auto topLeftCorner = sf::Vector2i(m_spritePreview->getPosition());

			if (Util::cordsInBox(sApplication->mousePos(true), topLeftCorner, 
				static_cast<int>(m_spritePreview->getGlobalBounds().width), static_cast<int>(m_spritePreview->getGlobalBounds().height)))
			{
				m_hotspotCache = sApplication->mousePos(true) - topLeftCorner;
				m_hotspotCache.x = static_cast<int>(m_hotspotCache.x / m_spritePreview->getScale().x);
				m_hotspotCache.y = static_cast<int>(m_hotspotCache.y / m_spritePreview->getScale().y);
				sApplication->registerContextMenu(Interface::SetHotspotContextMenu, getId(), sApplication->mousePos(true), { "Set hotspot" });
			}
		}
	}

	// Scale ctx popup followup
	if (auto promptPopup = sApplication->popPopupPrompt(
		{
			PopupPrompt::MapeditorScaleObj,
			PopupPrompt::MapeditorSetZone,
			PopupPrompt::MapeditorSetArea,
			PopupPrompt::MapeditorWanderNpc,
			PopupPrompt::MapeditorRespawnNpc,
			PopupPrompt::MapeditorRespawnGo
		}))
	{
		if (promptPopup->isAccepted())
		{
			switch (promptPopup->getCode())
			{
				case PopupPrompt::MapeditorWanderNpc:
				{
					if (auto npc = dynamic_pointer_cast<ClientNpc>(m_ctxRenderable))
					{
						float radius = (float)atof(promptPopup->getContent().c_str());
						m_cachedWanderRadius[npc->getGuid()] = radius;

						queueQuery(Util::fmtStr("UPDATE npc SET wander_distance = '%s' WHERE guid = '%d';", to_string(radius).c_str(), npc->getGuid()));
						
						if (radius == 0)
						{
							// Only set movement_type back to idle if it was at random (might have been at patrol with an ignored radius)
							queueQuery(Util::fmtStr("UPDATE npc SET wander_distance = 0 WHERE guid = %d;", npc->getGuid()));
							queueQuery(Util::fmtStr("UPDATE npc SET movement_type = '%d' WHERE guid = %d AND movement_type = '%d';", NpcDefines::DefaultMovement::Idle, npc->getGuid(), NpcDefines::DefaultMovement::Random));
						}
						else
						{ 
							queueQuery(Util::fmtStr("UPDATE npc SET movement_type = '%d' WHERE guid = %d AND movement_type = '%d';", NpcDefines::DefaultMovement::Random, npc->getGuid(), NpcDefines::DefaultMovement::Idle));
						}
					}

					break;
				}
				case PopupPrompt::MapeditorRespawnNpc:
				{
					if (auto npc = dynamic_pointer_cast<ClientNpc>(m_ctxRenderable))
					{
						int miliseconds = atoi(promptPopup->getContent().c_str());
						m_cachedRespawnTimes_Npc[npc->getGuid()] = miliseconds;
						queueQuery(Util::fmtStr("UPDATE npc SET respawn_time = '%d' WHERE guid = %d;", miliseconds, npc->getGuid()));
					}

					break;
				}
				case PopupPrompt::MapeditorRespawnGo:
				{					
					if (auto npc = dynamic_pointer_cast<ClientGameObj>(m_ctxRenderable))
					{
						int miliseconds = atoi(promptPopup->getContent().c_str());
						m_cachedRespawnTimes_Go[npc->getGuid()] = miliseconds;
						queueQuery(Util::fmtStr("UPDATE gameobject SET respawn = '%d' WHERE guid = %d;", miliseconds, npc->getGuid()));
					}

					break;
				}
				case PopupPrompt::MapeditorScaleObj:
				{
					float scale = static_cast<float>(atof(promptPopup->getContent().c_str()));

					if (scale < 1.f || scale > 100.f)
					{
						sApplication->spawnPopup("ERROR: Value must be between 1 and 100.", ConfirmMessageBox::ConfirmBox_Ok, ConfirmMessageBox::ConfirmCode_Null);				
					}
					else if (auto cell = dynamic_cast<MapCellClient*>(m_map->getCell(m_ctxCell)))
					{
						cell->setLayerScale(m_ctxLayer, scale / 100.f);
						cell->refreshSpriteScale(m_ctxLayer);
					}

					m_ctxLayer = 0;
					m_ctxCell = 0;
					break;
				}
				case PopupPrompt::MapeditorSetZone:
				{
					m_plantingZone = atoi(promptPopup->getContent().c_str());
					break;
				}
				case PopupPrompt::MapeditorSetArea:
				{
					m_plantingArea = atoi(promptPopup->getContent().c_str());
					break;
				}
			}
		}

		m_ctxRenderable = nullptr;
	}

	// Scale adjustment by mousewheel on map over
	if (m_map->isMousedOver() && m_spriteGrabbed != nullptr)
	{
		if (float mdelta = sApplication->getMouseWheelEvent().delta)
		{
			float scale = abs(m_spriteGrabbed->getScale().y);
			scale += mdelta / 20;

			if (!(abs(scale) < 0.05f || abs(scale) > 1.f))
			{
				sf::Vector2f oldScale = m_spriteGrabbed->getScale();
				m_spriteGrabbed->setScale(sf::Vector2f(oldScale.x < 0 ? -scale : scale, scale));
				m_lastScale = m_spriteGrabbed->getScale();
			}
		}
	}

	// Keybind Rotate object
	if (m_map->getMousedRenderable() != nullptr && sApplication->popKeyUp(sf::Keyboard::F))
	{
		m_ctxRenderable = m_map->getMousedRenderable();
		rotateMousedWorldRenderable();
	}

	// Flip your grabbed sprite
	if (m_spriteGrabbed != nullptr && sApplication->popKeyUp(sf::Keyboard::F))
	{
		auto scale = m_spriteGrabbed->getScale();
		scale.x *= -1.f;
		m_spriteGrabbed->setScale(scale);
	}

	/**
	* Confirmation boxes
	*/
	
	if (auto confirmBox = sApplication->popConfirmBox(
		{	ConfirmMessageBox::ConfirmCode_TerrainScript, 
			ConfirmMessageBox::ConfirmCode_SaveMapEditor,
			ConfirmMessageBox::ConfirmCode_MapeditorDeleteNpc }))
	{
		if (confirmBox->getResult() == ConfirmMessageBox::ConfirmBox_Yes)
		{
			switch (confirmBox->getCode())
			{
				case ConfirmMessageBox::ConfirmCode_TerrainScript:
				{
					// Terrain script
					blog(Logger::LOG_INFO, "Running terrain script in map editor...");
					sApplication->loadingSplash();
					parseMapScript();
					break;
				}
				case ConfirmMessageBox::ConfirmCode_SaveMapEditor:
				{
					// Save
					if (m_map != nullptr)
						m_map->saveToDisk();
					else
						sApplication->spawnPopup("ERROR: Not editing a map.", ConfirmMessageBox::ConfirmBox_Ok, ConfirmMessageBox::ConfirmCode_Null);

					shared_ptr<SqlConnector> db = SqlConnector::create(SqlType::SQLite, sConfig->getString("GameDb", "Name"), sConfig->getString("GameDb", "Path"));
					for_each(m_pendingQueries.begin(), m_pendingQueries.end(), [&](const string& itr) { db->query(itr.c_str()); });
					m_pendingQueries.clear();
					initDatabaseObjects();
					break;
				}
				case ConfirmMessageBox::ConfirmCode_MapeditorDeleteNpc:
				{					
					// Delete object
					if (auto npc = dynamic_pointer_cast<ClientNpc>(m_ctxRenderable))
					{
						m_map->removeWorldRenderable(npc);
						queueQuery(Util::fmtStr("DELETE FROM npc WHERE guid = %d", npc->getGuid()));
					}
					
					if (auto gameObj = dynamic_pointer_cast<ClientGameObj>(m_ctxRenderable))
					{
						m_map->removeWorldRenderable(gameObj);
						queueQuery(Util::fmtStr("DELETE FROM gameobject WHERE guid = %d", gameObj->getGuid()));
					}

					break;
				}
			}
		}
			
		m_ctxRenderable = nullptr;
	}

	/**
	* Map
	*/

	if (m_map != nullptr)
	{
		if (auto daynslider = dynamic_pointer_cast<ScrollBar>(getRenderObject(Interface::DayNightSlider)))
			m_map->setGlobalLightPower(daynslider->getScrollPercent());

		// If we're holding a texture that we want to plant in the world, then don't try to drag the map's camera
		m_map->setAllowCameraDrag(m_spriteGrabbed == nullptr && m_objGrabbed == nullptr && m_objMoving == nullptr);
	}

	if (m_map != nullptr && m_map->isMousedOver())
	{
		// Inserting texture into a tile
		if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && m_spriteGrabbed != nullptr)
		{
			const bool isTerrain = m_terrainDef.find(m_spriteGrabbed->getTextureName()) != m_terrainDef.end();

			if (m_editingTerrainTick->ticked())
			{
				if (isTerrain)
					m_map->setTerrain(getHoveringTerrain(), m_spriteGrabbed->getTextureName());
				else
					sApplication->spawnPopup("That is not terrain", ConfirmMessageBox::ConfirmBox_Ok, ConfirmMessageBox::ConfirmCode_Null);
			}
			else
			{
				if (isTerrain)
				{
					sApplication->spawnPopup("That is terrain", ConfirmMessageBox::ConfirmBox_Ok, ConfirmMessageBox::ConfirmCode_Null);
				}
				else
				{
					m_map->ensureCellExists(getHoveringCell());
					auto cell = static_cast<MapCellClient*>(m_map->getCell(getHoveringCell()));

					if (!cell->hasTextureForLayer(m_editingLayer))
					{
						cell->setLayerScale(m_editingLayer, 0.f);

						if (m_spriteGrabbed->getScale().x != 1.f)
							cell->setLayerScale(m_editingLayer, m_spriteGrabbed->getScale().x);

						cell->setTexture(make_shared<string>(m_spriteGrabbed->getTextureName()), m_editingLayer);
						cell->loadSprites();
					}
				}
			}
		}
		
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Z) && m_editingTerrainTick->ticked())		
			m_map->setTerrainZone(getHoveringTerrain(), m_plantingZone);
		
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) && m_editingTerrainTick->ticked())		
			m_map->setTerrainArea(getHoveringTerrain(), m_plantingArea);

		if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
		{
			// Placing a new object
			if (m_objGrabbed != nullptr)
			{
				int cellX, cellY;
				Geo2d::computeCellPos(getHoveringCell(), cellX, cellY, m_map->getMapWidth());

				if (auto npc = dynamic_pointer_cast<ClientNpc>(m_objPreview))
					addNpc(npc->getEntry(), cellX, cellY);
				else if (auto goObj = dynamic_pointer_cast<ClientGameObj>(m_objPreview))
					addGameObj(goObj->getEntry(), cellX, cellY);
			
				m_objGrabbed = nullptr;
			}

			// Moving an existing object
			if (m_objMoving != nullptr)
			{
				int cellX, cellY;
				Geo2d::computeCellPos(getHoveringCell(), cellX, cellY, m_map->getMapWidth());
				
				if (auto npc = dynamic_pointer_cast<ClientNpc>(m_objMoving))
					moveNpc(*npc, cellX, cellY);
				else if (auto goObj = dynamic_pointer_cast<ClientGameObj>(m_objMoving))
					moveGameObj(*goObj, cellX, cellY);

				m_objMoving = nullptr;
			}
		}

		// Removing data from a tile's layer
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Delete))
		{
			if (m_editingTerrainTick->ticked())
			{
				m_map->setTerrain(getHoveringTerrain(), "");
			}
			else
			{
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::RControl))
				{
					// Area of effect
					vector<int> indexes;
					MapLogic::getIndexesAround(indexes, getHoveringCell(), m_map->getMapWidth(), 5);

					for (auto& idx : indexes)
					{					
						if (sf::Keyboard::isKeyPressed(sf::Keyboard::RShift))
						{
							if (auto cell = m_map->getCell(idx))
								cell->setFlags(0);

							m_map->clearCellLayer(idx, 0);
							m_map->clearCellLayer(idx, 1);
							m_map->clearCellLayer(idx, 2);
						}
						else
						{
							m_map->clearCellLayer(idx, m_editingLayer);
						}
					}
				}
				else
				{
					m_map->clearCellLayer(getHoveringCell(), m_editingLayer);
				}
			}
		}

		// Adding/removing tiles that can't be walked on
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::C))
		{
			m_map->ensureCellExists(getHoveringCell());
			m_map->getCell(getHoveringCell())->addFlag(MapCellT::Flags::Unwalkable);
		}
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
		{
			m_map->ensureCellExists(getHoveringCell());
			m_map->getCell(getHoveringCell())->removeFlag(MapCellT::Flags::Unwalkable);
		}

		// Adding/removing tiles that cause ability line of sight collision
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::B))
		{
			m_map->ensureCellExists(getHoveringCell());
			m_map->getCell(getHoveringCell())->addFlag(MapCellT::Flags::CollideBlock);
		}
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::G))
		{
			m_map->ensureCellExists(getHoveringCell());
			m_map->getCell(getHoveringCell())->removeFlag(MapCellT::Flags::CollideBlock);
		}
	}

	/***
	* Minimap
	*/

	if (isMinimapMousedOver() && sf::Mouse::isButtonPressed(sf::Mouse::Left))
		dragMinimapCamera();
}

const string& MapEditor::friendlyName(const string& textureName) const
{
	auto itr = m_textureNameLibrary.find(textureName);

	if (itr != m_textureNameLibrary.end())
		return itr->second;

	return textureName;
}

const string& MapEditor::textureName(const string& friendlyName) const
{
	auto itr = m_textureNameCache.find(friendlyName);

	if (itr != m_textureNameCache.end())
		return itr->second;

	return friendlyName;
}

void MapEditor::render()
{	
	if (m_dbEditor != nullptr)
	{
		m_dbEditor->attemptRender();
		return;
	}

	RenderObjectHolder::render();

	if (m_spritePreview != nullptr)
	{
		m_spritePreview->capSize(225.f, 200.f);
		m_spritePreview->setHotspotEasy(true, false, true);
		m_spritePreview->render(m_previewSpriteSpot);
	}

	if (m_objPreview != nullptr)
	{
		m_objPreview->setScreenPosition(sf::Vector2i(m_previewSpriteSpot));
		m_objPreview->render();
	}

	if (m_spriteGrabbed != nullptr)
	{
		m_spriteGrabbed->renderScript(sApplication->mousePosf(true));

		if (sApplication->popKeyDown(sf::Keyboard::Escape))
			m_spriteGrabbed = nullptr;
	}

	if (m_objGrabbed != nullptr)
	{
		m_objGrabbed->setScreenPosition({ sApplication->mousePos(true) });
		m_objGrabbed->render();
	}

	if (m_objMoving != nullptr)
	{
		// If the object has a camera pointer then it calculate out its own screen position
		m_objMoving->setCameraPtr(nullptr);
		m_objMoving->setScreenPosition({ sApplication->mousePos(true) });
		m_objMoving->render();
		m_objMoving->setCameraPtr(&m_map->getCameraRef());
	}

	m_zoneIdentifierText->setOriginalStringf("Zone Tool: %u", m_plantingZone).draw(462, 52);
	m_areaIdentifierText->setOriginalStringf("Area Tool: %u", m_plantingArea).draw(462, 70);
	m_layerIdentifierText->setOriginalStringf("Editing Layer: %u", m_editingLayer + 1).draw(327, 100);
	m_spriteScaleText->setOriginalStringf("Cached Scale: %2.f%%", m_lastScale.y * 100.f).draw(335, 355);	
	
	if (m_currentFolderText != nullptr)
		m_currentFolderText->draw(20, 103);

	if (m_map != nullptr && m_map->isMousedOver() && wasInputLastFrame() && !isMinimapMousedOver())
	{
		// Terrain tile and info
		if (m_editingTerrainTick->ticked())
		{
			const int hoverTerrainId = getHoveringTerrain();
			const auto renderPos = m_map->getTerrainRenderPosRelative(hoverTerrainId);
			m_spriteSelectedTerrain->setHotspotEasy(true, true);
			m_spriteSelectedTerrain->render({ renderPos.x, renderPos.y });

			// TerainId
			string result = "Terrain " + to_string(hoverTerrainId);

			if (auto spr = m_map->getTerrain(hoverTerrainId))
				result += "\n" + spr->getTextureName();

			// Zone
			if (int zoneId = m_map->getZoneId(hoverTerrainId))
			{
				string zoneName = sContentMgr->db("zone_template").data(zoneId, "name");
				result += "\nZone: " + (zoneName.empty() ? to_string(zoneId) : zoneName);
			}

			// Area
			if (int areaId = m_map->getAreaId(hoverTerrainId))
			{
				string areaName = sContentMgr->db("area_template").data(areaId, "name");
				result += "\nArea: " + (areaName.empty() ? to_string(areaId) : areaName);
			}

			if (m_tileinfoText->getOriginalString() != result)
				m_tileinfoText->setOriginalString(result);

			m_tileinfoText->draw(static_cast<int>(renderPos.x) + 20, static_cast<int>(renderPos.y) - 20);
		}
		else
		{
			const int hoverCellId = getHoveringCell();
			const auto renderPos = m_map->getCellRenderPosRelative(hoverCellId);

			if (m_map->getMousedRenderable() == nullptr)
			{
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::RControl))
				{
					// Area of effect
					vector<int> indexes;
					MapLogic::getIndexesAround(indexes, hoverCellId, m_map->getMapWidth(), 5);

					for (auto& idx : indexes)
					{
						auto cellPos = m_map->getCellRenderPosRelative(idx);
						m_spriteSelectedTile->render({ cellPos.x, cellPos.y });
					}
				}
				else
				{
					// One at a time
					m_spriteSelectedTile->render({ renderPos.x, renderPos.y });
				}
			}

			int x, y;
			m_map->getCellPositionRelative(x, y, renderPos);

			string result;
			char baseTxt[MAX_PATH];

			if (auto obj = dynamic_pointer_cast<ClientObject>(m_map->getMousedRenderable()))
			{
				result = obj->getName();
				
				if (auto npc = dynamic_pointer_cast<ClientNpc>(m_map->getMousedRenderable()))
				{
					float wander_distance = m_cachedWanderRadius[npc->getGuid()];
					int respawn_time = m_cachedRespawnTimes_Npc[npc->getGuid()];

					result += "\nWander Radius: " + to_string(wander_distance);
					result += "\nRespawn Time: " + to_string(respawn_time);
				}				
				else if (auto npc = dynamic_pointer_cast<ClientGameObj>(m_map->getMousedRenderable()))
				{
					int respawn_time = m_cachedRespawnTimes_Go[npc->getGuid()];
					result += "\nRespawn Time: " + to_string(respawn_time);
				}
			}
			else
			{
				float mouseWorldX, mouseWorldY;
				const auto mousePos = sApplication->mousePos(true);
				m_map->getCellWorldPosRelative(mouseWorldX, mouseWorldY, { static_cast<float>(mousePos.x), static_cast<float>(mousePos.y) });

				sprintf_s(baseTxt, "Cell %d (%d, %d) (%.2f, %.2f)\n", hoverCellId, x, y, mouseWorldX, mouseWorldY);
				result = baseTxt;

				if (auto cell = dynamic_cast<MapCellClient*>(m_map->getCell(hoverCellId)))
				{
					for (int i = 0; i <= GameMap::Defines::Layer3; ++i)
					{
						const string texname = cell->getTextureName(i);

						if (!texname.empty())
							result += "Layer " + to_string(i + 1) + " " + friendlyName(texname) + "\n";
					}
				}
			}
			
			if (m_tileinfoText->getOriginalString() != result)
				m_tileinfoText->setOriginalString(result);

			m_tileinfoText->draw(static_cast<int>(renderPos.x) + 20, static_cast<int>(renderPos.y) - 20);
		}
	}

	if (m_showMinimap && m_minimapSprite.getTexture() != nullptr)
		drawMinimap();
}

bool MapEditor::isMinimapMousedOver() const
{
	if (m_showMinimap == false)
		return false;

	auto topLeftCorner = m_minimapSprite.getPosition();
	float w = m_minimapSprite.getScale().x * float(m_minimapSprite.getTexture()->getSize().x);
	float h = m_minimapSprite.getScale().y * float(m_minimapSprite.getTexture()->getSize().y);

	return Util::cordsInBox(sApplication->mousePos(), sf::Vector2i(sf::Vector2f(topLeftCorner.x, topLeftCorner.y)), int(w), int(h));
}
		
void MapEditor::dragMinimapCamera()
{
	float w = m_minimapSprite.getScale().x * float(m_minimapSprite.getTexture()->getSize().x);
	float h = m_minimapSprite.getScale().y * float(m_minimapSprite.getTexture()->getSize().y);

	auto topLeftCorner = m_minimapSprite.getPosition();
	auto minimapOffsetClicked = sApplication->mousePosf() - topLeftCorner;
	
	minimapOffsetClicked.x /= m_minimapSprite.getScale().x;
	minimapOffsetClicked.y /= m_minimapSprite.getScale().y;
	
	auto baseline = m_map->getCellRenderPos(0);
	auto baseline2 = m_map->getCellRenderPos(111071);
	
	Minimap::minimapPixelCordToMapRenderPos(minimapOffsetClicked.x, minimapOffsetClicked.y, m_map->getMapWidth());
	
	Geo2d::Vector2 worldPos = m_map->renderPosToWorldPos(Geo2d::Vector2(minimapOffsetClicked.x, minimapOffsetClicked.y));
	auto rawScreenPos = GameMap::computeRawScreenPosition(worldPos);
	m_map->getCameraRef() = { -rawScreenPos.x, -rawScreenPos.y };
}

void MapEditor::drawMinimap()
{
	auto topLeftCorner = m_spriteBg->getTopLeftCornerRef();
	topLeftCorner.y += static_cast<int>(m_spriteBg->getRaw()->getTexture()->getSize().y);
	float scale = float(m_spriteBg->getRaw()->getTexture()->getSize().x) / float(m_minimapSprite.getTexture()->getSize().x);
	m_minimapSprite.setScale({ scale, scale });
	m_minimapSprite.setPosition(sf::Vector2f(topLeftCorner));
	sApplication->canvas().draw(m_minimapSprite);

	// Border lines
	sf::RectangleShape acrossLine(sf::Vector2f(float(m_spriteBg->getRaw()->getTexture()->getSize().x), 1.f));
	sf::RectangleShape downLine(sf::Vector2f(1.f, float(m_minimapSprite.getTexture()->getSize().y) * scale));
	acrossLine.setFillColor(sf::Color::Yellow);
	downLine.setFillColor(sf::Color::Yellow);
	
	// Top
	acrossLine.setPosition(sf::Vector2f(float(topLeftCorner.x), float(topLeftCorner.y - 1)));
	sApplication->canvas().draw(acrossLine);
	
	// Bottom
	acrossLine.setPosition(sf::Vector2f(float(topLeftCorner.x), floor(float(topLeftCorner.y) + (float(m_minimapSprite.getTexture()->getSize().y) * scale)) - 1.f));
	sApplication->canvas().draw(acrossLine);
	
	// Left
	downLine.setPosition(sf::Vector2f(float(topLeftCorner.x), float(topLeftCorner.y - 1)));
	sApplication->canvas().draw(downLine);
	
	// Right
	downLine.setPosition(sf::Vector2f(float(topLeftCorner.x - 1) + float(m_spriteBg->getRaw()->getTexture()->getSize().x), float(topLeftCorner.y - 1)));
	sApplication->canvas().draw(downLine);

	// Labels
	//

	for (auto& itr : m_minimapLabels)
	{
		Text& txt = *itr.second.get();
		sf::Vector2i worldPos = itr.first;
		Geo2d::Vector2 screenPos = GameMap::computeRawScreenPosition({ float(worldPos.x), float(worldPos.y) });
		
		Minimap::mapRenderPosToMinimapPixelCord(screenPos.x, screenPos.y, m_map->getMapWidth());

		// Adjust for our new scale
		screenPos.x *= scale;
		screenPos.y *= scale;
		
		sf::Vector2i drawPos(sf::Vector2f(topLeftCorner) + sf::Vector2f(screenPos.x, screenPos.y));
		txt.draw(drawPos.x - int(txt.getGlobalBounds().width / 2.f), drawPos.y - int(txt.getGlobalBounds().height / 2.f));
	}

	// Camera marker
	//

	// Center of screen to world position
	int centerCellId = m_map->renderPosToCellIdRelative({ static_cast<float>(sApplication->centerOfScreen().x), static_cast<float>(sApplication->centerOfScreen().y) });	
	Geo2d::Vector2 minimapCameraPos = m_map->getCellRenderPos(centerCellId);
	Minimap::mapRenderPosToMinimapPixelCord(minimapCameraPos.x, minimapCameraPos.y, m_map->getMapWidth());

	// Adjust for our new scale
	minimapCameraPos.x *= scale;
	minimapCameraPos.y *= scale;

	m_spriteMinimapMyLocX->render(sf::Vector2f(topLeftCorner) + sf::Vector2f(minimapCameraPos.x, minimapCameraPos.y));
}

void MapEditor::setBrowserStage(const BrowserStages stg, const string& par /*= ""*/)
{
	m_browserStage = stg;
	m_textureList->clear();
	m_textureList->getScrollObject()->setScrollOffset(0);
	vector<string> filenames;

	auto backButton = getRenderObject(Interface::BrowserReturnButton);
	backButton->setHidden(true);

	if (stg == BrowserStages::NpcList || stg == BrowserStages::GoList)
	{
		backButton->setHidden(false);
		m_spritePreview = nullptr;

		// Database table (npc's, gameobjects)
		string dbtable = string(par.begin() + 2, par.end());
		auto data = sContentMgr->db(dbtable).fetchIndexData();

		vector<string> sortedList;

		for (auto& itr : data)
		{
			string name;
			string entry;

			for (auto& subItr : itr.second)
			{
				if (subItr.first == "name")
					name = subItr.second.getString();
				else if (subItr.first == "entry")
					entry = subItr.second.getString();
			}

			sortedList.push_back(Util::fmtStr("%s - '%s'", entry.c_str(), name.c_str()));
		}

		sort(sortedList.begin(), sortedList.end(), Util::compareNaturalSort);

		for (auto& itr : sortedList)
			m_textureList->addLine(itr);
	}
	else if (stg == BrowserStages::TextureList)
	{
		m_objPreview = nullptr;
		backButton->setHidden(false);

		// Textures for the .map
		vector<string> lines = m_textureLists["scripts//mapeditor_textures//" + par + ".txt"];
		m_textureList->addLines(lines);
		
		// Label for the current opened list
		string cpy = "-- " + par;
		bool tooBig = cpy.size() > 20;

		while (cpy.size() > 20)
			cpy.erase(cpy.end() - 1);

		if (tooBig)
			cpy += "...";

		m_currentFolderText = make_unique<Text>(m_font);
		m_currentFolderText->setCharacterSize(12);
		m_currentFolderText->setOriginalString(cpy);
	}
	else if (stg == BrowserStages::Directory)
	{
		const string dir = "scripts//mapeditor_textures";
		Util::getFileList(dir.c_str(), filenames);

		bool populateTextureList = m_textureLists.empty();

		for (auto& str : filenames)
		{
			if (populateTextureList)
			{
				vector<string> lines;
				Util::readLinesFromFile(str, lines);

				for (auto& itr : lines)
					itr = friendlyName(itr);

				m_textureLists[str] = lines;
			}

			Util::strReplaceAll(str, dir + "//", "");
			Util::strReplaceAll(str, ".txt", "");
		}

		// Textures for the .map
		m_textureList->addLines(filenames);
		m_currentFolderText = nullptr;

		// Database table (npc's, gameobjects)
		m_textureList->addLine("> npc_template");
		m_textureList->addLine("> gameobject_template");
	}
}

int MapEditor::getHoveringCell() const
{
	if (m_map == nullptr)
		return 0;

	const auto mousePos = sApplication->mousePos(true);
	return m_map->renderPosToCellIdRelative({ static_cast<float>(mousePos.x), static_cast<float>(mousePos.y) });
}

int MapEditor::getHoveringTerrain() const
{
	if (m_map == nullptr)
		return 0;

	const auto mousePos = sApplication->mousePos(true);
	return m_map->renderPosToTerrainIdRelative({ static_cast<float>(mousePos.x), static_cast<float>(mousePos.y) });
}

void MapEditor::parseMapScript()
{	
	sf::RenderTexture canvas;
	sApplication->registerCanvas(&canvas);

	canvas.create(Minimap::MapPixelWidth, Minimap::MapPixelHeight);
	canvas.clear();

	float fullW = float(m_map->getMapWidth()) * GameMap::BaseCellWidth;
	float scale = Minimap::MapPixelWidth / fullW;

	for (int terrainId = 0; terrainId < m_map->getTerrainWidth() * m_map->getTerrainWidth(); ++terrainId)
	{
		if (auto terrain = m_map->getTerrain(terrainId))
		{
			Geo2d::Vector2 renderPosition = m_map->getTerrainRenderPos(terrainId);
			Minimap::mapRenderPosToMinimapPixelCord(renderPosition.x, renderPosition.y, m_map->getMapWidth());

			terrain->setScale({ scale, scale });
			terrain->render({ renderPosition.x, renderPosition.y });
			terrain->setScale({1.f, 1.f});
		}
	}

	for (size_t cellId = 0; cellId < m_map->getNumCells(); ++cellId)
	{
		m_map->ensureCellExists(cellId);

		if (auto cell = static_cast<MapCellClient*>(m_map->getCell(cellId)))
		{			
			Geo2d::Vector2 cellPos = m_map->getCellRenderPos(cellId);
			Minimap::mapRenderPosToMinimapPixelCord(cellPos.x, cellPos.y, m_map->getMapWidth());
			
			cell->loadSprites();	

			float scaleBefore_0 = cell->getLayerScale(0);
			float scaleBefore_1 = cell->getLayerScale(1);
			float scaleBefore_2 = cell->getLayerScale(2);	

			cell->setLayerScale(0, scale);
			cell->setLayerScale(1, scale);
			cell->setLayerScale(2, scale);	

			cell->refreshSpriteScale(0);
			cell->refreshSpriteScale(1);
			cell->refreshSpriteScale(2);

			cell->renderLayer({ cellPos.x, cellPos.y }, 0, *m_map, false, false);
			cell->renderLayer({ cellPos.x, cellPos.y }, 1, *m_map, false, false);
			cell->renderLayer({ cellPos.x, cellPos.y }, 2, *m_map, false, false);

			cell->setLayerScale(0, scaleBefore_0 != 0.f ? scaleBefore_0 : 1.f);
			cell->setLayerScale(1, scaleBefore_1 != 0.f ? scaleBefore_1 : 1.f);
			cell->setLayerScale(2, scaleBefore_2 != 0.f ? scaleBefore_2 : 1.f);

			cell->refreshSpriteScale(0);
			cell->refreshSpriteScale(1);
			cell->refreshSpriteScale(2);
		}
	}
	
	canvas.display();
	canvas.getTexture().copyToImage().saveToFile(sApplication->getCacheFolderPath() + m_map->getName() + ".png");
	sApplication->resetCanvas();
	initMinimap(m_map->getName());
	m_showMinimap = true;
}

void MapEditor::initMinimap(const string& mapName)
{
	if (m_map != nullptr)
	{
		if (m_minimapTexture.loadFromFile(sApplication->getCacheFolderPath() + m_map->getName() + ".png"))
		{
			m_minimapSprite.setTexture(m_minimapTexture);
			m_minimapLabels.clear();

			shared_ptr<SqlConnector> db = SqlConnector::create(SqlType::SQLite, sConfig->getString("GameDb", "Name"), sConfig->getString("GameDb", "Path"));

			if (auto result = db->query(Util::fmtStr("SELECT name, target_x, target_y FROM teleport_names WHERE target_mapId IN (SELECT id FROM map WHERE name = '%s');", m_map->getName().c_str())))
			{
				for (auto& itr : result->fetchData())
				{
					auto name = itr[0].getString();
					auto target_x = itr[1].getInt32();
					auto target_y = itr[2].getInt32();

					auto txt = make_shared<Text>(m_font);
					txt->setCharacterSize(10);
					txt->setShadowOffset(1);
					txt->setString(name);

					pair<sf::Vector2i, shared_ptr<Text>> dat;
					dat = { sf::Vector2i(target_x, target_y), txt };
					m_minimapLabels.push_back(dat);
				}
			}
		}
	}
}

// This function is public for the console
void MapEditor::initMap()
{
	initDatabaseObjects();
	initCameraPosition();
}

void MapEditor::saveCameraPosition()
{
	string configKey = "MapEditor_" + m_map->getName();
	sConfig->setInt(configKey.c_str(), "CameraX", static_cast<int>(m_map->getCameraRef().x));
	sConfig->setInt(configKey.c_str(), "CameraY", static_cast<int>(m_map->getCameraRef().y));
}

void MapEditor::initCameraPosition()
{
	string configKey = "MapEditor_" + m_map->getName();
	m_map->getCameraRef().x = static_cast<float>(sConfig->getInt(configKey.c_str(), "CameraX"));
	m_map->getCameraRef().y = static_cast<float>(sConfig->getInt(configKey.c_str(), "CameraY"));
}

void MapEditor::moveNpc(ClientObject& obj, const int x, const int y)
{
	shared_ptr<SqlConnector> db = SqlConnector::create(SqlType::SQLite, sConfig->getString("GameDb", "Name"), sConfig->getString("GameDb", "Path"));
	
	sf::Vector2f worldPos(static_cast<float>(x), static_cast<float>(y));
	
	// Center in the selected square
	worldPos.x += 0.5f;
	worldPos.y += 0.5f;	
	
	queueQuery(Util::fmtStr("UPDATE npc SET position_x = '%f' WHERE guid = %d;", worldPos.x, obj.getGuid()));
	queueQuery(Util::fmtStr("UPDATE npc SET position_y = '%f' WHERE guid = %d;", worldPos.y, obj.getGuid()));

	obj.setWorldPosition(worldPos);
}

void MapEditor::moveGameObj(ClientObject& obj, const int x, const int y)
{
	shared_ptr<SqlConnector> db = SqlConnector::create(SqlType::SQLite, sConfig->getString("GameDb", "Name"), sConfig->getString("GameDb", "Path"));
	
	sf::Vector2f worldPos(static_cast<float>(x), static_cast<float>(y));
	
	// Center in the selected square
	worldPos.x += 0.5f;
	worldPos.y += 0.5f;
	
	queueQuery(Util::fmtStr("UPDATE gameobject SET position_x = '%f' WHERE guid = %d;", worldPos.x, obj.getGuid()));
	queueQuery(Util::fmtStr("UPDATE gameobject SET position_y = '%f' WHERE guid = %d;", worldPos.y, obj.getGuid()));

	obj.setWorldPosition(worldPos);
}

void MapEditor::addGameObj(const int entry, const int x, const int y)
{
	shared_ptr<SqlConnector> db = SqlConnector::create(SqlType::SQLite, sConfig->getString("GameDb", "Name"), sConfig->getString("GameDb", "Path"));

	if (auto result = db->query(Util::fmtStr("SELECT id FROM map WHERE name = '%s';", m_map->getName().c_str())))
	{
		const int newGuid = ++m_maxGuidGo;
		const int mapId = result->fetchData()[0][0].getInt32();

		sf::Vector2f worldPos(static_cast<float>(x), static_cast<float>(y));
		
		// Center in the selected square
		worldPos.x += 0.5f;
		worldPos.y += 0.5f;

		shared_ptr<ClientGameObj> obj = make_shared<ClientGameObj>(newGuid, entry, m_map.get());
		obj->setWorldPosition(worldPos);
		obj->setRespectAppMouseover(false);
		obj->addFlag(GoDefines::Flags::ShowName);
		m_map->addWorldRenderable(obj);
		m_dbObjs.push_back(obj);

		queueQuery(Util::fmtStr("INSERT INTO 'main'.'gameobject' ('guid', 'entry', 'map', 'position_x', 'position_y', 'respawn', 'state') VALUES (%d, %d, %d, %f, %f, %d, %d);",
			newGuid, entry, mapId, worldPos.x, worldPos.y, 300, GoDefines::ToggleState::Closed));
	}
	else
	{
		string err = db->error();

		if (err.empty())
		{
			if (m_map->getName().empty())
				err = "Map has an empty name, cannot save information about it to the database.";
			else
				err = "Map '" + m_map->getName() + "' not found in database table 'map'.";
		}

		sApplication->spawnPopup("ERROR: " + err, ConfirmMessageBox::ConfirmBox_Ok, ConfirmMessageBox::ConfirmCode_Null);
	}
}

void MapEditor::addNpc(const int entry, const int x, const int y)
{
	shared_ptr<SqlConnector> db = SqlConnector::create(SqlType::SQLite, sConfig->getString("GameDb", "Name"), sConfig->getString("GameDb", "Path"));

	if (auto result = db->query(Util::fmtStr("SELECT id FROM map WHERE name = '%s';", m_map->getName().c_str())))
	{
		const int newGuid = ++m_maxGuidNpc;
		const int mapId = result->fetchData()[0][0].getInt32();

		sf::Vector2f worldPos(static_cast<float>(x), static_cast<float>(y));
		
		// Center in the selected square
		worldPos.x += 0.5f;
		worldPos.y += 0.5f;

		shared_ptr<ClientNpc> npc = make_shared<ClientNpc>(newGuid, m_map.get());
		npc->setEntry(entry);
		npc->setVariable(ObjDefines::Variable::ModelId, atoi(sContentMgr->db("npc_template").data(entry, "model_id").c_str()));
		npc->setVariable(ObjDefines::Variable::ModelScale, atoi(sContentMgr->db("npc_template").data(entry, "model_scale").c_str()));
		npc->setWorldPosition(worldPos);
		npc->setRespectAppMouseover(false);
		m_map->addWorldRenderable(npc);
		m_dbObjs.push_back(npc);

		queueQuery(Util::fmtStr("INSERT INTO 'main'.'npc' ('guid', 'entry', 'map', 'position_x', 'position_y', 'respawn_time', 'movement_type', 'wander_distance') VALUES (%d, %d, %d, %f, %f, %d, %d, %d);",
			newGuid, entry, mapId, worldPos.x, worldPos.y, 300, 0, 0));
	}
	else
	{
		string err = db->error();
		
		if (err.empty())
		{
			if (m_map->getName().empty())
				err = "Map has an empty name, cannot save information about it to the database.";
			else
				err = "Map '" + m_map->getName() + "' not found in database table 'map'.";
		}

		sApplication->spawnPopup("ERROR: " + err, ConfirmMessageBox::ConfirmBox_Ok, ConfirmMessageBox::ConfirmCode_Null);
	}
}

void MapEditor::queueQuery(const string& query)
{
	m_pendingQueries.push_back(query);
}

void MapEditor::verifyDbObjects()
{
	for (auto itr = m_dbObjs.begin(); itr != m_dbObjs.end();)
	{
		bool valid = true;

		if (auto npc = dynamic_pointer_cast<ClientNpc>(*itr))
			valid = !sContentMgr->db("npc_template").data(npc->getEntry(), "entry").empty();
		else if (auto go = dynamic_pointer_cast<ClientGameObj>(*itr))
			valid = !sContentMgr->db("gameobject_template").data(go->getEntry(), "entry").empty();

		if (valid)
		{
			++itr;
		}
		else
		{
			m_map->removeWorldRenderable(*itr);
			itr = m_dbObjs.erase(itr);
		}
	}
}

void MapEditor::initDatabaseObjects()
{
	if (m_map == nullptr)
		return;

	// Flush old list first
	for_each(m_dbObjs.begin(), m_dbObjs.end(), [&](auto& itr) { m_map->removeWorldRenderable(itr); });
	m_dbObjs.clear();
	
	m_cachedWanderRadius.clear();
	m_cachedRespawnTimes_Go.clear();
	m_cachedRespawnTimes_Npc.clear();

	shared_ptr<SqlConnector> db = SqlConnector::create(SqlType::SQLite, sConfig->getString("GameDb", "Name"), sConfig->getString("GameDb", "Path"));
	db->query("DELETE FROM npc WHERE entry NOT IN (SELECT entry FROM npc_template)");
	db->query("DELETE FROM gameobject WHERE entry NOT IN (SELECT entry FROM gameobject_template)");

	m_maxGuidNpc = 0;
	m_maxGuidGo = 0;
	
	if (auto result = db->query(Util::fmtStr("SELECT MAX(guid) FROM gameobject")))
		m_maxGuidGo = result->fetchData()[0][0].getInt32();
	
	if (auto result = db->query(Util::fmtStr("SELECT MAX(guid) FROM npc")))
		m_maxGuidNpc = result->fetchData()[0][0].getInt32();
			
	if (auto result = db->query(Util::fmtStr("SELECT guid, entry, map, position_x, position_y, orientation, path_id, movement_type, wander_distance, death_state, respawn_time FROM npc WHERE map IN (SELECT id FROM map WHERE name = '%s');"
		, m_map->getName().c_str())))
	{
		for (auto& itr : result->fetchData())
		{
			auto guid = itr[0].getInt32();
			auto entry = itr[1].getInt32();
			auto map = itr[2].getInt32();
			auto position_x = itr[3].getFloat();
			auto position_y = itr[4].getFloat();
			auto orientation = itr[5].getFloat();
			auto path_id = itr[6].getInt32();
			auto movement_type = itr[7].getInt32();
			auto wander_distance = itr[8].getFloat();
			auto death_state = itr[9].getInt32();
			auto respawn_time = itr[10].getInt32();

			shared_ptr<ClientNpc> npc = make_shared<ClientNpc>(guid, m_map.get());
			npc->setEntry(entry);
			npc->setVariable(ObjDefines::Variable::ModelId, atoi(sContentMgr->db("npc_template").data(entry, "model_id").c_str()));
			npc->setVariable(ObjDefines::Variable::ModelScale, atoi(sContentMgr->db("npc_template").data(entry, "model_scale").c_str()));
			npc->setWorldPosition({ position_x, position_y });
			npc->setOrientation(orientation);
			npc->setRespectAppMouseover(false);

			m_map->addWorldRenderable(npc);
			m_dbObjs.push_back(npc);

			m_cachedRespawnTimes_Npc[guid] = respawn_time;
			m_cachedWanderRadius[guid] = wander_distance;
		}
	}

	if (auto result = db->query(Util::fmtStr("SELECT guid, entry, map, position_x, position_y, state, respawn FROM gameobject WHERE map IN (SELECT id FROM map WHERE name = '%s');"
		, m_map->getName().c_str())))
	{
		for (auto& itr : result->fetchData())
		{
			auto guid = itr[0].getInt32();
			auto entry = itr[1].getInt32();
			auto map = itr[2].getInt32();
			auto position_x = itr[3].getFloat();
			auto position_y = itr[4].getFloat();
			auto state = itr[5].getInt32();
			auto respawn_time = itr[6].getInt32();

			shared_ptr<ClientGameObj> obj = make_shared<ClientGameObj>(guid, entry, m_map.get());
			obj->setWorldPosition({ position_x, position_y });
			obj->setToggleState(GoDefines::ToggleState(state));
			obj->setRespectAppMouseover(false);
			obj->addFlag(GoDefines::Flags::ShowName);

			m_map->addWorldRenderable(obj);
			m_dbObjs.push_back(obj);

			m_cachedRespawnTimes_Go[guid] = respawn_time;
		}
	}
}

shared_ptr<ClientGameObj> MapEditor::createGameObj(const int entry)
{	
	shared_ptr<ClientGameObj> goObj = make_shared<ClientGameObj>(1, entry, nullptr);
	return goObj;	
}

shared_ptr<ClientNpc> MapEditor::createNpc(const int entry)
{
	shared_ptr<ClientNpc> npc = make_shared<ClientNpc>(1, nullptr);
	npc->setEntry(entry);
	npc->setVariable(ObjDefines::Variable::ModelId, atoi(sContentMgr->db("npc_template").data(entry, "model_id").c_str()));
	npc->setVariable(ObjDefines::Variable::ModelScale, atoi(sContentMgr->db("npc_template").data(entry, "model_scale").c_str()));
	return npc;
}

void MapEditor::deleteMousedRenderable()
{
	if (auto npc = dynamic_pointer_cast<ClientObject>(m_ctxRenderable))
		sApplication->spawnPopup("Delete " + npc->getName() + "?", ConfirmMessageBox::ConfirmBox_YesNo, ConfirmMessageBox::ConfirmCode_MapeditorDeleteNpc);
}

void MapEditor::rotateMousedWorldRenderable()
{
	if (auto npc = dynamic_pointer_cast<ClientNpc>(m_ctxRenderable))
	{
		auto dirBefore = npc->computeDirection(npc->getOrientation());

		while (dirBefore == npc->computeDirection(npc->getOrientation()))
			npc->setOrientation(npc->getOrientation() + 0.2f);

		queueQuery(Util::fmtStr("UPDATE npc SET orientation='%.2f' WHERE guid = %d;", npc->getOrientation(), npc->getGuid()));
	}
}

void MapEditor::editMousedWanderRadius()
{
	if (auto npc = dynamic_pointer_cast<ClientNpc>(m_ctxRenderable))
		sApplication->spawnPopupPrompt("Wander Radius", PopupPrompt::MapeditorWanderNpc, true, true);
}

void MapEditor::editMousedRespawnTime()
{
	if (auto npc = dynamic_pointer_cast<ClientNpc>(m_ctxRenderable))
		sApplication->spawnPopupPrompt("Respawn Time", PopupPrompt::MapeditorRespawnNpc, true);
	else if (auto npc = dynamic_pointer_cast<ClientGameObj>(m_ctxRenderable))
		sApplication->spawnPopupPrompt("Respawn Time", PopupPrompt::MapeditorRespawnGo, true);
}

void MapEditor::grabMousedWorldRenderable()
{
	if (auto obj = dynamic_pointer_cast<ClientObject>(m_ctxRenderable))
		m_objMoving = obj;
}

void MapEditor::editMousedWorldRenderable()
{
	if (auto npc = dynamic_pointer_cast<ClientNpc>(m_ctxRenderable))
	{
		m_dbEditor = make_unique<DbTemplateEditor>(*this, 0);
		m_dbEditor->setStage("npc_template");
		m_dbEditor->setToEntry(npc->getEntry());
	}
}

void MapEditor::notifyCtxMenuClicked(const int id, const string& lineClicked) /*final*/
{
	if (id == Interface::ObjectContextMenu)
	{
		if (lineClicked == "Rotate")
			rotateMousedWorldRenderable();
		else if (lineClicked == "Delete")
			deleteMousedRenderable();
		else if (lineClicked == "Edit Template")
			editMousedWorldRenderable();
		else if (lineClicked == "Wander Radius")
			editMousedWanderRadius();
		else if (lineClicked == "Respawn Time")
			editMousedRespawnTime();
		else if (lineClicked == "Move")
			grabMousedWorldRenderable();
	}
	else if (id == Interface::SelectTextureContextMenu)
	{
		if (!lineClicked.empty())
		{
			if (lineClicked.find("Select") != string::npos)
			{
				string cpy = lineClicked;
				Util::strReplaceAll(cpy, "Select ", "");

				if (!cpy.empty())
					m_spriteGrabbed = sContentMgr->spawnSprite(textureName(cpy));
			}
			else if (auto cell = dynamic_cast<MapCellClient*>(m_map->getCell(m_ctxCell)))
			{
				for (int l = 0; l <= GameMap::Defines::Layer3; ++l)
				{
					string textureName = cell->getTextureName(l);

					if (textureName.empty())
						continue;

					// If this is the texture we chose
					if (lineClicked.find(friendlyName(textureName)) != string::npos)
					{
						m_ctxLayer = l;
						sApplication->spawnPopupPrompt(friendlyName(textureName) + " Layer " + to_string(l + 1), PopupPrompt::Codes::MapeditorScaleObj);
						break;
					}
				}
			}
		}
	}
	else if (id == Interface::SetHotspotContextMenu)
	{
		if (auto spr = sContentMgr->ensureSpriteScript(m_spritePreview->getTextureName()))
			spr->setHotspot(m_spritePreview->getTextureName(), m_hotspotCache.x, m_hotspotCache.y);
	}
}