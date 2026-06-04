#pragma once

#include "RenderObjectHolder.h"

class ClientMap;
class TickBox;
class Text;
class Sprite;
class TextLines;
class DraggableNode;
class ClientObject;
class WorldRenderable;
class ClientGameObj;
class ClientNpc;
class DbTemplateEditor;
class SpriteRo;

class MapEditor : public RenderObjectHolder
{
	enum Interface
	{
		HandButton = 1,
		TerrainButton,
		BrowserReturnButton,
		SaveButton,
		ZoneButton,
		AreaButton,
		Layer1_Tick,
		Layer2_Tick,
		Layer3_Tick,
		ShowUnwalkable_Tick,
		ShowBlocking_Tick,
		Grid_Tick,
		EditTerrain_Tick,
		TextureBrowser,
		GameMap,
		ObjectContextMenu,
		RevealOccupied,
		SelectTextureContextMenu,
		DayNightSlider,
		SetHotspotContextMenu
	};

	enum BrowserStages
	{
		Directory,
		TextureList,
		NpcList,
		GoList
	};

	public:
		MapEditor(RenderObject& owner, const int id);
		~MapEditor();

		void initMap();
		void initMinimap(const string& mapName);

		shared_ptr<ClientMap> getMap() const { return m_map; }

	protected:
		virtual void notifyCtxMenuClicked(const int id, const string& lineClicked) final;

	private:
		virtual void input() override;
		virtual void render() override;

		void initDatabaseObjects();
		void initCameraPosition();
		void saveCameraPosition();
		void parseMapScript();
		void setBrowserStage(const BrowserStages stg, const string& par = "");
		void addNpc(const int entry, const int x, const int y);
		void addGameObj(const int entry, const int x, const int y);
		void moveNpc(ClientObject& obj, const int x, const int y);
		void moveGameObj(ClientObject& obj, const int x, const int y);
		void queueQuery(const string& query);
		void rotateMousedWorldRenderable();
		void deleteMousedRenderable();
		void editMousedWorldRenderable();
		void editMousedWanderRadius();
		void editMousedRespawnTime();
		void grabMousedWorldRenderable();
		void verifyDbObjects();
		void drawMinimap();
		void dragMinimapCamera();

		bool isMinimapMousedOver() const;

		const string& friendlyName(const string& textureName) const;
		const string& textureName(const string& friendlyName) const;
		
		shared_ptr<ClientNpc> createNpc(const int entry);
		shared_ptr<ClientGameObj> createGameObj(const int entry);

		bool hasGrabbedSprite() const { return m_spriteGrabbed != nullptr; }

		int getHoveringCell() const;
		int getHoveringTerrain() const;

		// Which layer the user has selected
		int m_editingLayer{0};
		int m_plantingZone{0};
		int m_plantingArea{0};
		int m_maxGuidNpc{0};
		int m_maxGuidGo{0};
		int m_ctxCell{0};
		int m_ctxLayer{0};

		bool m_showMinimap{ false };

		sf::Vector2i m_hotspotCache;
		sf::Vector2f m_previewSpriteSpot;
		sf::Vector2f m_lastScale;

		unique_ptr<Text> m_tileinfoText;
		unique_ptr<Text> m_layerIdentifierText;
		unique_ptr<Text> m_spriteScaleText;
		unique_ptr<Text> m_currentFolderText;
		unique_ptr<Text> m_zoneIdentifierText;
		unique_ptr<Text> m_areaIdentifierText;

		shared_ptr<ClientMap> m_map;
		shared_ptr<sf::Font> m_font;
		shared_ptr<TextLines> m_textureList;
		shared_ptr<TickBox> m_editingTerrainTick;
		shared_ptr<SpriteRo> m_spriteBg;
		
		shared_ptr<Sprite> m_spriteGrabbed;
		shared_ptr<Sprite> m_spritePreview;
		shared_ptr<Sprite> m_spriteSelectedTile;
		shared_ptr<Sprite> m_spriteSelectedTerrain;
		shared_ptr<Sprite> m_spriteMinimapMyLocX;

		shared_ptr<ClientObject> m_objPreview;
		shared_ptr<ClientObject> m_objGrabbed;
		shared_ptr<ClientObject> m_objMoving;

		shared_ptr<WorldRenderable> m_ctxRenderable;

		unique_ptr<DbTemplateEditor> m_dbEditor;

		vector<string> m_pendingQueries;
		vector<string> m_knownTextures;
		vector<shared_ptr<WorldRenderable>> m_dbObjs;
		vector<pair<sf::Vector2i, shared_ptr<Text>>> m_minimapLabels;

		set<string> m_terrainDef;
		map<string, string> m_textureNameLibrary;
		map<string, string> m_textureNameCache;
		map<string, vector<string>> m_textureLists;

		map<int, float> m_cachedWanderRadius;
		map<int, int> m_cachedRespawnTimes_Npc;
		map<int, int> m_cachedRespawnTimes_Go;

		BrowserStages m_browserStage;

		sf::Sprite m_minimapSprite;
		sf::Texture m_minimapTexture;
};

