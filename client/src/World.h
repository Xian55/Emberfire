#pragma once

#include "BuffDebuffRenderer.h"
#include "CombatMessage.h"
#include "ScrollingMessage.h"

#include "..\Geo2d.h"
#include "..\Shared\MutualObject.h"
#include "..\Shared\CooldownHolder.h"

class CastBar;
class ClientMap;
class ClientObject;
class ClientPlayer;
class ClientUnit;
class DraggableNode;
class Game;
class GameIcon;
class LevelupNotify;
class Sprite;
class StlBuffer;
class TextBox;
class WorldSpellAnimation;
class ZoneNotifcation;
class Equipment;
class UnitFrame;
class ItemIcon;
class Toolbar;
class MapQuester;

class World : public BuffDebuffRenderer, public CooldownHolder
{
	public:
		enum Interface
		{
			MapObject = 1,

			// Toolbar rows
			ToolbarSprite,
			Toolbar1,
			Toolbar2,
			Toolbar3,
			ToolbarXpObj,

			// Toolbar buttons
			SocialButton,
			QuestsButton,
			OptionsButton,
			EquipmentButton,
			InventoryButton,
			SpellsButton,

			// Panels
			Panels_Start,
			EquipmentPanel,
			QuestLogPanel,
			GuildPanel,
			AbilitiesPanel,
			InventoryPanel,
			BankPanel,
			QuestOfferPanel,
			QuestCompletePanel,
			DialogNpcPanel,
			VendorNpcPanel,
			LootWindowPanel,
			MapQuesterPanel,
			TradeWindowPanel,
			Panels_End,

			// Misc
			PlayerUnitFrame,
			TargetUnitFrame,
			Party1UnitFrame,
			Party2UnitFrame,
			Party3UnitFrame,
			CtxMenuCurrent,
			GameChatBox,
			PlayerCastBar,
			MinimapObj,
			ObjectivesObj,
			LevelupNotifyObj,
			SpendExpButtonObj,
			MinimapButton,
			WaypointButton,
			ZoneNotify,
			ButtonTargetSelf,
			ButtonTargetParty1,
			ButtonTargetParty2,
			ButtonTargetParty3,

			// Context Menues
			NpcContextMenu,
			PlayerContextMenu,
			ChannelCtxMenu,

			// Lua addon layer
			LuaFrameRoot
		};

		enum WASDCooldown
		{
			Up,
			Down,
			Left,
			Right,
			UpLeft,
			UpRight,
			DownLeft,
			DownRight,
		};

		enum Hint
		{
			SpendExp,
		};

	public:
		World(Game& owner, const int id);
		~World();

		void setMap(const int mapId);
		void setController(const int guid);
		void openPanel(const Interface id, const bool playsound = true);
		void closePanel(const Interface id, const bool playsound = true);
		void closePanels(const set<Interface>& exceptions = {}, const bool playsound = true);
		void switchDialogPanel(const Interface id);
		void closeDialogPanels();
		void togglePanel(const Interface id);
		void pushCombatMessage(const string& str, const sf::Vector2i& pos, const sf::Color color = sf::Color::White, const bool crit = false, const float textScale = 1.f, const int sourceGuid = 0, const bool floatSideways = false, const float decayRate = 1.f, const bool pushUpward = true, const bool ignoreCameraChanges = false, const bool makeRoomAtSide = true, const bool skipFadeIn = false);
		void pushScrollingMessage(const string& str, const sf::Color color = sf::Color::White);
		void formatWorldText(string& str, const string& objName);
		void formatQuestText(string& str, const int questId);
		void addClientObject(shared_ptr<ClientObject> obj);
		void removeClientObject(const int guid);

		// Bridge to spawn the controlled player. NewWorld(op77) carries the player's
		// pos+vars but NOT its guid; SetController(op80) carries the guid but no
		// class/gender/portrait. Char-select stashes class/gender/portrait here at
		// EnterWorld; NewWorld stashes pos; setController builds the ClientPlayer.
		struct PendingEquip { std::uint8_t slot = 0; ItemDefines::ItemDefinition item; };
		struct PendingSelf { int guid = 0, classId = 0, gender = 0, portrait = 0, mapId = 1; float x = 0.f, y = 0.f; bool hasPos = false;
		                     std::string name;
		                     std::vector<std::pair<std::uint8_t, std::int32_t>> vars;
		                     std::vector<PendingEquip> equipment; };
		static PendingSelf s_pendingSelf;
		static clock_t s_enterStartClock;   // set when EnterWorld is sent; used for entry-timing logs
		void setGrabbedIcon(shared_ptr<GameIcon> icon);
		void setSelectedGuid(const int guid);
		void addWorldSpellAnimation(shared_ptr<WorldSpellAnimation> ptr);
		void updateGuiPositions();
		void error(const int worldError);
		void chatError(const int chatError);
		void queueObjectivesRecalc() { m_recalcObjectives = true; }
		void setGroundActionSpell(const int spellId) { m_groundActionSpell = spellId; }
		void setItemAction(shared_ptr<ItemIcon> icon) { m_itemAction = icon; }
		void setLastMousedGuid(const int guid) { m_lastMousedGuid = guid; }
		void onLevelTo(const int lvl);
		void getObjectPositions(const int mapId, const int entry, const MutualObject::Type type, vector<sf::Vector2f>& output);
		void refreshToolbarTooltips();
		void computePendingLevelupCost();
		void requestLevelup();
		void setNextCastSound(const int spellId, const string& filename) { m_nextCastSound = { spellId, filename }; }
		void exclaimHint(const Hint hint);
		void setGossipGuid(const int guid) { m_gossipGuid = guid; }
		void setParty(const vector<int>& mbrs, const int m_leaderGuid);
		void flushCombatMsgs(const int sourceGuid);

		bool isIconGrabbed() const;
		bool isPanelOpen(const Interface id) const;
		bool isPartyMember(const int guid) const;
		bool mouseInWorld() const;
		bool canMove() const;
		bool canAct() const;
		bool popEmptyToolbars();
		bool isEmptyToolbars() const { return m_emptyToolbars; }
		bool isPartyLeader(const int guid) const;

		int getSelectedGuid() const;
		int getMyselfGuid() const;
		int getPanelMaxXpos() const;
		int getNumLeftPanel() const { return (int)m_leftPanel.size(); }
		int getCachedPendingLevelupCost() const { return m_pendingLevelupCost; }
		int getGossipGuid() const { return m_gossipGuid; }

		auto getMapId() const { return m_mapId; }
		auto getGrabbedIcon() const { return m_grabbedIcon; }
		auto getGroundActionSpell() const { return m_groundActionSpell; }
		auto getItemAction() const { return m_itemAction.get(); }
		
		pair<int, MutualObject::Type> getQuestStartInfo(const int questId) const;
		pair<int, MutualObject::Type> getQuestEndInfo(const int questId) const;

		string getMyselfName() const;
		string getGossipString(const int entry) const;
		string getGossipOptionsString(const int entry) const;

		ClientMap& getMap() { return *m_map; }
		ClientPlayer* myself() const { return m_myself.get(); }
		ClientUnit* selectedUnit() const { return m_selectedUnit.get(); }
		MapQuester* getMapQuester() const { return m_mapQuester.get(); }

		shared_ptr<ClientPlayer> myselfRef() { return m_myself; }
		shared_ptr<ClientUnit> selectedUnitRef() { return m_selectedUnit; }

		sf::Vector2f getCameraOffset() const { return { m_cameraOffset.x, m_cameraOffset.y }; }

		shared_ptr<ClientObject> getClientObject(const int guid) const;

	private:
		void input() final;
		void render() final;
		
		void cameraFollow();
		void drawCombatMsgs();
		void drawTopCenterMsgs();
		void computeTopCenterMsgPositions();
		void updateButtonBind(const Interface id, const string& bindname);
		void updateActionBarBind(const Interface id, const int startI, const int endI);
		void requestMove(const sf::Vector2f& worldPos, const bool wasd);
		void refreshPanelXpos();
		void wasd();
		void cacheQuests();
		void cacheGossipOptions();
		void updateCameraDrag();
		void updateFadingObjects();
		void tabTargeting();
		void launchSpendExp();
		void queryWaypoints();
		void updatePartyFrames();
		void selectUnitByButtonId(const Interface id);

		bool inputPointerAction();
		bool popEscapeKeyUp();

		sf::Vector2i getControllerCameraPosition() const;
		
		shared_ptr<Button> makeBtnBindOnly(shared_ptr<Button> input) const;

		int m_mapId{0};
		int m_groundActionSpell{0};
		int m_pendingLevelupCost{0};
		int m_lastMousedGuid{0};
		int m_gossipGuid{0};
		int m_partyLeaderGuid{0};

		float m_introAlpha;
		clock_t m_introEndClock{0};   // real-time deadline for "world ready" -> PLAYER_LOGIN (FPS-independent)

		bool m_emptyToolbars{false};
		bool m_recalcObjectives{false};

		const float m_topcenterStartPct;

		__time64_t m_tabTargetTimer{0};

		bool m_lastIsRenderingNativeDPI = false;
		sf::Vector2u m_lastScreenSize;
		Geo2d::Vector2 m_cameraOffset;

		Game& m_game;
		
		shared_ptr<ClientMap> m_map;
		shared_ptr<GameIcon> m_grabbedIcon;
		shared_ptr<GameIcon> m_grabbedIconCopy;
		shared_ptr<ClientPlayer> m_myself;
		shared_ptr<CastBar> m_castBar;
		shared_ptr<ZoneNotifcation> m_zoneIndicator;
		shared_ptr<ClientUnit> m_selectedUnit;
		shared_ptr<Equipment> m_equipment;
		shared_ptr<Button> m_equipmentButton;
		shared_ptr<Button> m_spendExpButton;
		shared_ptr<Button> m_waypointButton;
		shared_ptr<ItemIcon> m_itemAction;
		shared_ptr<MapQuester> m_mapQuester;

		unique_ptr<DraggableNode> m_cameraDrag;

		set<int> m_tabTargetList;

		vector<int> m_partyMembers;
		vector<Interface> m_leftPanel;
		vector<ScrollingMsg> m_topCenterMsgs;
		vector<shared_ptr<UnitFrame>> m_partyFrames;
		vector<shared_ptr<Toolbar>> m_toolbars;

		pair<int, string> m_nextCastSound;

		map<WASDCooldown, clock_t> m_wasdCooldown;				
		map<int, vector<CombatMsg>> m_combatMessages;
		map<int, shared_ptr<ClientObject>> m_objects;
		map<int, pair<shared_ptr<ClientObject>, float>> m_fadingObjects;

		// questId, { entry, objectType }
		map<int, pair<int, MutualObject::Type>> m_questStartCache;
		map<int, pair<int, MutualObject::Type>> m_questEndCache;
		
		// mapId, { { entry, objectType }, positions }
		map<int, map<pair<int, MutualObject::Type>, vector<sf::Vector2f>>> m_questRelatedObjects;

		// entry,text
		map<int, string> m_gossipOptions;
		map<int, string> m_gossips;
};

