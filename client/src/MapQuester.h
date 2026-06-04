#pragma once

#include "WorldPanel.h"

#include "..\Shared\PlayerDefines.h"

class Sprite;
class DraggableNode;

class MapQuester : public WorldPanel
{
	enum Defines
	{
		MapPixelWidth = 1221,
		MapPixelHeight = 687,

		MapRenderOffsetX = -39,
		MapRenderOffsetY = 70
	};

	enum Interface
	{
		ButtonGoBack = 1,
	};

	public:
		MapQuester(World& owner, const int id);
		virtual ~MapQuester();
		
		void resetPosition() final;
		
		void loadCache();
		void setAvailableQuests(const vector<uint16_t>& quests);
		void setDoneQuests(const vector<uint16_t>& quests);
		void queueRevealKillNpcs(const set<int>& entries);
		void queueRevealGameObjs(const set<int>& entries);
		void queueRevealNpcs(const set<int>& entries);
		void queueRevealItems_AppendNpcGameObj(const set<int>& entries);
		void startWaypointSelection(const vector<int32_t>& gameobjGuids);
		void stopWaypointSelection();
		
		bool isNpcEntryMarkedKill(const int entry) const;
		bool isGameObjMarkedLoot(const int entry) const;

		bool isFrontInsertPanel() const final { return false; }

	private:
		void input() final;
		void render() final;	
		void onClose() final;
		
		void saveCache();
		void parseRevealItems(const set<int>& entries);
		void mapRenderPosToMinimapPixelCord(float& x, float& y, const int mapWidth) const;
		void revealObjects(const string& table, const set<int>& entries, vector<sf::Vector2f>& renderLocs) const;

		int m_clickedWaypointGuid{0};

		bool m_queueRevealItems{false};
		bool m_showingWaypoints{false};
		
		set<int> m_queueRevealNpcs;
		set<int> m_queueRevealKillNpcs;
		set<int> m_queueRevealGameObjs;
		set<int> m_queueRevealItems_AppendNpcGameObj;

		set<int> m_cacheRevealKillNpcs;
		set<int> m_cacheRevealGameObjs;
		
		vector<pair<shared_ptr<Button>, int>> m_waypointButtons;

		vector<sf::Vector2f> m_avaiQuests_RenderLoc;
		vector<sf::Vector2f> m_doneQuests_RenderLoc;
		vector<sf::Vector2f> m_killnpc_RenderLoc;
		vector<sf::Vector2f> m_lootobj_RenderLoc;
		vector<sf::Vector2f> m_npc_RenderLoc;

		unique_ptr<DraggableNode> m_dragNode;

		shared_ptr<Sprite> m_questAvail;
		shared_ptr<Sprite> m_questDone;
		shared_ptr<Sprite> m_npcKillMarker;
		shared_ptr<Sprite> m_npcMarker;
		shared_ptr<Sprite> m_goToggleMarker;
		shared_ptr<Sprite> m_slotHider;
		shared_ptr<Sprite> m_mapquester_hiddenmap;
		shared_ptr<Sprite> m_mapquester_map;
};

