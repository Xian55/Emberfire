#include "stdafx.h"
#include "MapQuester.h"
#include "Application.h"
#include "DraggableNode.h"
#include "World.h"
#include "Button.h"
#include "ClientPlayer.h"
#include "UnitFrame.h"
#include "Sprite.h"
#include "ContentMgr.h"
#include "GameObjTemplateEditor.h"
#include "ConfirmMessageBox.h"
#include "Connector.h"

#include "..\SqlConnector\SqlConnector.h"
#include "..\Shared\Config.h"
#include "..\StringHelpers.h"
#include "..\Shared\GamePacketClient.h"

#include <assert.h>

MapQuester::MapQuester(World& owner, const int id) :
	WorldPanel(owner, id, "mapgui.png", sf::Vector2i(1100, 25))
{
	resetPosition();

	m_dragNode = make_unique<DraggableNode>();
	m_dragNode->addAnchor(&m_topLeftCorner);
	m_dragNode->addAnchor(&m_bottomRightCorner);
	m_dragNode->setDragNodeHeight(60);

	m_mapquester_map = sContentMgr->spawnSprite("mapquester_map.png");
	
	m_npcMarker = sContentMgr->spawnSprite("npc_marker_map.png");
	m_npcMarker->setHotspotEasy(true, true);

	m_questDone = sContentMgr->spawnSprite("quest_done_map.png");
	m_questDone->setHotspotEasy(true, true);

	m_questAvail = sContentMgr->spawnSprite("quest_ready_map.png");
	m_questAvail->setHotspotEasy(true, true);

	m_npcKillMarker = sContentMgr->spawnSprite("quest_kill_map.png");
	m_npcKillMarker->setHotspotEasy(true, true);

	m_goToggleMarker = sContentMgr->spawnSprite("quest_kill_gear.png");
	m_goToggleMarker->setHotspotEasy(true, true);
	
	m_slotHider = sContentMgr->spawnSprite("mapquester_tilehider.png");
	m_slotHider->setHotspotEasy(true, false);

	addRenderObject(attachObj(make_shared<Button>(*this, "generic_highlight", Interface::ButtonGoBack), sf::Vector2i(537, 677)));
}

MapQuester::~MapQuester()
{

}

void MapQuester::render() /*final*/
{	
	if (world().getNumLeftPanel() <= 1 || world().getNumLeftPanel() > 2)
	{
		// These are computated based on the topleft corner, so we can't call the functions while editing values below
		static int w = mouseableWidth();
		static int h = mouseableHeight();

		getTopLeftCornerRef().x = (sApplication->sW() / 2) - (w / 2);
		getTopLeftCornerRef().y = (sApplication->sH() / 2) - (h / 2);
	}

	if (!m_queueRevealItems_AppendNpcGameObj.empty())
	{
		// Must come before npc's/obj's
		parseRevealItems(m_queueRevealItems_AppendNpcGameObj);
		m_queueRevealItems_AppendNpcGameObj.clear();
	}

	if (!m_queueRevealNpcs.empty())
	{
		revealObjects("npc", m_queueRevealNpcs, m_npc_RenderLoc);
		m_queueRevealNpcs.clear();
	}

	if (!m_queueRevealKillNpcs.empty())
	{
		revealObjects("npc", m_queueRevealKillNpcs, m_killnpc_RenderLoc);
		m_cacheRevealKillNpcs = m_queueRevealKillNpcs;
		m_queueRevealKillNpcs.clear();
	}

	if (!m_queueRevealGameObjs.empty())
	{
		revealObjects("gameobject", m_queueRevealGameObjs, m_lootobj_RenderLoc);
		m_cacheRevealGameObjs = m_queueRevealGameObjs;
		m_queueRevealGameObjs.clear();
	}

	__super::render();

	if (world().myself() == nullptr)
		return;

	m_mapquester_map->render(sf::Vector2f(getTopLeftCornerRef() + sf::Vector2i(Defines::MapRenderOffsetX, Defines::MapRenderOffsetY)));
	
	auto renderPortrait = [&](UnitFrame* ptr)
	{
		if (ptr == nullptr || ptr->getUnitPtr() == nullptr)
			return;

		if (auto portrait = ptr->getPortraitPtr())
		{
			// World position -> Render position -> Location on the 957x539 sprite
			Geo2d::Vector2 maptexLoc = ptr->getUnitPtr()->getWorldPosition();
			maptexLoc = world().getMap().getCellRenderPosF(maptexLoc.x, maptexLoc.y);
			mapRenderPosToMinimapPixelCord(maptexLoc.x, maptexLoc.y, world().myself()->getMap()->getMapWidth());

			sf::Color borderColor;

			if (ptr->getUnitPtr()->isLocal())
				borderColor = sf::Color::Magenta;
			else
				borderColor = sf::Color::Green;

			portrait->renderAsCircle(sf::Vector2f(getTopLeftCornerRef()) + sf::Vector2f(ceil(maptexLoc.x), ceil(maptexLoc.y)), 
				m_showingWaypoints ? 7 : 11, { 0, ptr->getPortraitOffset() }, 1.f, borderColor);
		}
	};
	
	renderPortrait(dynamic_pointer_cast<UnitFrame>(world().getRenderObject(World::Interface::Party1UnitFrame)).get());
	renderPortrait(dynamic_pointer_cast<UnitFrame>(world().getRenderObject(World::Interface::Party2UnitFrame)).get());
	renderPortrait(dynamic_pointer_cast<UnitFrame>(world().getRenderObject(World::Interface::Party3UnitFrame)).get());
	renderPortrait(dynamic_pointer_cast<UnitFrame>(world().getRenderObject(World::Interface::PlayerUnitFrame)).get());
	
	if (!m_showingWaypoints)
	{
		for (auto& itr : m_killnpc_RenderLoc)
			m_npcKillMarker->render(sf::Vector2f(getTopLeftCornerRef()) + itr);

		for (auto& itr : m_lootobj_RenderLoc)
			m_goToggleMarker->render(sf::Vector2f(getTopLeftCornerRef()) + itr);

		for (auto& itr : m_avaiQuests_RenderLoc)
			m_questAvail->render(sf::Vector2f(getTopLeftCornerRef()) + itr);

		for (auto& itr : m_doneQuests_RenderLoc)
			m_questDone->render(sf::Vector2f(getTopLeftCornerRef()) + itr);

		for (auto& itr : m_npc_RenderLoc)
			m_npcMarker->render(sf::Vector2f(getTopLeftCornerRef()) + itr);
	}
	else 
	{
		// Not on a waypoint anymore
		if (m_world.myself() != nullptr && m_world.myself()->getWaypointStandingGuid() == 0)
			m_world.closePanel(World::Interface::MapQuesterPanel);
		else
		{
			for (auto& itr : m_waypointButtons)
				itr.first->attemptRender();
		}
	}
}

void MapQuester::input() /*final*/
{
	__super::input();
	m_dragNode->updateDraggableObject(*this);

	int poppedButton = popFirstButtonId();

	if (poppedButton == Interface::ButtonGoBack)
		m_world.closePanel(World::Interface(getId()));

	for (auto& itr : m_waypointButtons)
	{
		itr.first->attemptInput();

		if (itr.first->popActivated())
		{
			m_clickedWaypointGuid = itr.second;
			sApplication->spawnPopup("Teleport to this waypoint?",  ConfirmMessageBox::ConfirmBox_YesNo, ConfirmMessageBox::ConfirmCode_ActivateWaypoint);
			break;
		}
	}

	// ConfirmCode_ActivateWaypoint
	if (auto confirmBox = sApplication->popConfirmBox({ ConfirmMessageBox::ConfirmCode_ActivateWaypoint }))
	{		
		if (confirmBox->getResult() == ConfirmMessageBox::ConfirmBoxResult::ConfirmBox_Yes && m_world.myself() != nullptr)
		{
			GP_Client_ActivateWaypoint packet;
			packet.m_nearbyWaypointGuid = m_world.myself()->getWaypointStandingGuid();
			packet.m_targetWaypointGuid = m_clickedWaypointGuid;
			sConnector->sendPacket(packet.build(StlBuffer{}));

			sContentMgr->playSound(SfxId::Teleport);
			m_clickedWaypointGuid = 0;
			m_world.closePanels({}, false);
		}
	}
		
	// Right clicking on blue npc marks gets rid of them
	for (auto itr = m_npc_RenderLoc.begin(); itr != m_npc_RenderLoc.end();)
	{
		sf::Vector2i topLeft(*itr);
		sf::Vector2i bottomRight(int(itr->x) + int(m_npcMarker->getTexture()->getSize().x), int(itr->y) + int(m_npcMarker->getTexture()->getSize().y));
		
		topLeft += getTopLeftCornerRef();
		bottomRight += getTopLeftCornerRef();
		
		topLeft -= m_npcMarker->getHotspot();
		bottomRight -= m_npcMarker->getHotspot();

		if (Util::cordsInBox(sApplication->mousePos(), topLeft, bottomRight.x - topLeft.x, bottomRight.y -topLeft.y))
		{
			if (sf::Mouse::isButtonPressed(sf::Mouse::Right))
			{
				itr = m_npc_RenderLoc.erase(itr);
				continue;
			}
		}

		++itr;
	}
}

void MapQuester::onClose() /*final*/
{
	stopWaypointSelection();
}
		
bool MapQuester::isNpcEntryMarkedKill(const int entry) const
{
	return m_cacheRevealKillNpcs.find(entry) != m_cacheRevealKillNpcs.end() || m_queueRevealKillNpcs.find(entry) != m_queueRevealKillNpcs.end();
}
		
bool MapQuester::isGameObjMarkedLoot(const int entry) const
{
	return m_queueRevealGameObjs.find(entry) != m_queueRevealGameObjs.end() || m_queueRevealGameObjs.find(entry) != m_queueRevealGameObjs.end();
}
		
void MapQuester::loadCache()
{
	//
}
		
void MapQuester::saveCache()
{
	//
}

void MapQuester::setDoneQuests(const vector<uint16_t>& quests)
{
	m_doneQuests_RenderLoc.clear();

	for (auto& questId : quests)
	{
		pair<int, MutualObject::Type> inf = world().getQuestEndInfo(questId);
		
		vector<sf::Vector2f> positions;
		world().getObjectPositions(world().getMapId(), inf.first, inf.second, positions);

		for (auto& worldPosition : positions)
		{
			// World position -> Render position -> Location on the 957x539 sprite
			Geo2d::Vector2 maptexLoc(worldPosition.x, worldPosition.y);
			maptexLoc = world().getMap().getCellRenderPosF(maptexLoc.x, maptexLoc.y);
			mapRenderPosToMinimapPixelCord(maptexLoc.x, maptexLoc.y, world().myself()->getMap()->getMapWidth());
			m_doneQuests_RenderLoc.push_back(sf::Vector2f(maptexLoc.x, maptexLoc.y));
		}
	}
}
		
void MapQuester::queueRevealNpcs(const set<int>& entries)
{
	m_queueRevealNpcs = entries;
	m_npc_RenderLoc.clear();
}

void MapQuester::queueRevealKillNpcs(const set<int>& entries)
{ 
	m_queueRevealKillNpcs = entries;
	m_killnpc_RenderLoc.clear();
	m_cacheRevealKillNpcs.clear();
}

void MapQuester::queueRevealGameObjs(const set<int>& entries)
{ 
	m_queueRevealGameObjs = entries;
	m_lootobj_RenderLoc.clear();
	m_cacheRevealGameObjs.clear();
}
		
void MapQuester::stopWaypointSelection()
{
	m_showingWaypoints = false;
}

void MapQuester::startWaypointSelection(const vector<int32_t>& waypoints)
{
	if (m_showingWaypoints)
		stopWaypointSelection();

	if (waypoints.empty())
		return;

	m_showingWaypoints = true;	
	map<int, sf::Vector2f> objs;
	
	for (auto& dbGuid : waypoints)
	{
		sf::Vector2f pos =
		{
			(float)atof(sContentMgr->db("gameobject").data(dbGuid, "position_x").c_str()),
			(float)atof(sContentMgr->db("gameobject").data(dbGuid, "position_y").c_str())
		};		

		objs[dbGuid] = pos;
	}

	int idx = 0;
	m_waypointButtons.clear();

	for (auto& obj : objs)
	{
		// World position -> Render position -> Location on the 957x539 sprite
		Geo2d::Vector2 maptexLoc(obj.second.x, obj.second.y);
		maptexLoc = world().getMap().getCellRenderPosF(maptexLoc.x, maptexLoc.y);
		mapRenderPosToMinimapPixelCord(maptexLoc.x, maptexLoc.y, world().myself()->getMap()->getMapWidth());
		
		sf::Vector2f offset = sf::Vector2f(maptexLoc.x, maptexLoc.y);		
		m_waypointButtons.push_back({ dynamic_pointer_cast<Button>(attachObj(make_shared<Button>(*this, "map_portal_icon", 0), sf::Vector2i(offset) - sf::Vector2i(16, 15))), obj.first });
	}
}
		
void MapQuester::queueRevealItems_AppendNpcGameObj(const set<int>& entries)
{
	m_queueRevealItems_AppendNpcGameObj = entries;
}

void MapQuester::parseRevealItems(const set<int>& entries)
{
	if (entries.empty())
		return;

	shared_ptr<SqlConnector> db = SqlConnector::create(SqlType::SQLite, sConfig->getString("GameDb", "Name"), sConfig->getString("GameDb", "Path"));

	string itemEntries;

	for (auto& entry : entries)
		itemEntries += to_string(entry) + ",";

	itemEntries.erase(itemEntries.end() - 1);
	
	if (auto result = db->query(Util::fmtStr("SELECT entry FROM npc_template WHERE custom_loot IN (SELECT lootId FROM loot WHERE item IN (%s));", itemEntries.c_str())))
	{
		for (auto& fields : result->fetchData())
			m_queueRevealKillNpcs.insert(fields[0].getInt32());
	}

	vector<pair<int, string>> objTypes;
	GameObjTemplateEditor::getFieldsWithItems(objTypes);

	for (auto& itr : objTypes)
	{
		if (auto result = db->query(Util::fmtStr("SELECT entry FROM gameobject_template WHERE type = %d AND %s IN (%s);", itr.first, itr.second.c_str(), itemEntries.c_str())))
		{
			for (auto& fields : result->fetchData())
				m_queueRevealGameObjs.insert(fields[0].getInt32());
		}
	}	
}
		
void MapQuester::revealObjects(const string& table, const set<int>& entries, vector<sf::Vector2f>& renderLocs) const
{
	renderLocs.clear();

	if (entries.empty())
		return;

	shared_ptr<SqlConnector> db = SqlConnector::create(SqlType::SQLite, sConfig->getString("GameDb", "Name"), sConfig->getString("GameDb", "Path"));

	string entriesStr;

	for (auto& entry : entries)
		entriesStr += to_string(entry) + ",";

	entriesStr.erase(entriesStr.end() - 1);

	vector<Geo2d::Vector2> existingLocs;

	if (auto result = db->query(Util::fmtStr("SELECT position_x, position_y FROM %s WHERE entry IN (%s) AND map = '%d';", table.c_str(), entriesStr.c_str(), world().getMapId())))
	{
		for (auto& fields : result->fetchData())
		{
			Geo2d::Vector2 maptexLoc(fields[0].getFloat(), fields[1].getFloat());
			maptexLoc = world().getMap().getCellRenderPosF(maptexLoc.x, maptexLoc.y);
			mapRenderPosToMinimapPixelCord(maptexLoc.x, maptexLoc.y, world().myself()->getMap()->getMapWidth());

			// Avoid too much repition in similar area
			bool redundant = false;

			for (auto& itr : existingLocs)
			{
				if (itr.getDist(maptexLoc) < m_npcKillMarker->getGlobalBounds().width)
				{
					redundant = true;
					break;
				}
			}

			if (redundant)
				continue;

			existingLocs.push_back(maptexLoc);
			renderLocs.push_back(sf::Vector2f(maptexLoc.x, maptexLoc.y));
		}
	}
}

void MapQuester::setAvailableQuests(const vector<uint16_t>& quests)
{
	m_avaiQuests_RenderLoc.clear();

	for (auto& questId : quests)
	{
		pair<int, MutualObject::Type> inf = world().getQuestStartInfo(questId);
		
		vector<sf::Vector2f> positions;
		world().getObjectPositions(world().getMapId(), inf.first, inf.second, positions);

		for (auto& worldPosition : positions)
		{
			// World position -> Render position -> Location on the 957x539 sprite
			Geo2d::Vector2 maptexLoc(worldPosition.x, worldPosition.y);
			maptexLoc = world().getMap().getCellRenderPosF(maptexLoc.x, maptexLoc.y);
			mapRenderPosToMinimapPixelCord(maptexLoc.x, maptexLoc.y, world().myself()->getMap()->getMapWidth());
			m_avaiQuests_RenderLoc.push_back(sf::Vector2f(maptexLoc.x, maptexLoc.y));
		}
	}
}
	
void MapQuester::resetPosition() /*final*/
{	

}
		
void MapQuester::mapRenderPosToMinimapPixelCord(float& x, float& y, const int mapWidth) const
{
	float fullW = float(mapWidth) * GameMap::BaseCellWidth;
	float scale = 3200 / fullW;

	x += fullW / 2.f;
	x *= scale;
	y *= scale;
	y += 64.f;

	float finalScale = float(MapPixelWidth) / 3200.f;

	x *= finalScale;
	y *= finalScale;

	x += Defines::MapRenderOffsetX;
	y += Defines::MapRenderOffsetY;
}