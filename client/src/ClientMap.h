#pragma once

#include "RenderObject.h"
#include "DraggableNode.h"
#include "..\Shared\GameMap.h"

#include <mutex>
#include <thread>

class Sprite;
class WorldRenderable;
class MapCellClient;
class ClientObjectOverhead;
class SpriteAnimation;
class Text;

class ClientMap : public GameMap, public RenderObject, public DraggableNode
{
	#define BaseFoliageWindSkew 0.05f
	
	public:
		struct RenderCell
		{
			int cellId{0};
			Geo2d::Vector2 baseRenderPos;
		};

		struct RenderData
		{
			vector<int> terrain;
			vector<RenderCell> cells[GameMap::Defines::NumLayers];
		};

		struct Foliage
		{
			bool init{0};
			float pump{0};
			shared_ptr<sf::Texture> texture;
		};

	public:
		ClientMap(RenderObject& owner, const int id);
		virtual ~ClientMap();
		
		void saveToDisk();
		void setTerrain(const int id, const string& textureName);
		void getCellPositionRelative(int& x, int& y, Geo2d::Vector2 screenPos, const bool floorCamera = true);
		void getCellWorldPosRelative(float& x, float& y, Geo2d::Vector2 screenPos, const bool floorCamera = true);
		void loadFoliageNames();
		void addWorldRenderable(shared_ptr<WorldRenderable> wo);
		void removeWorldRenderable(shared_ptr<WorldRenderable> wo);
		void registryProximitySound(const sf::Vector2f& screenPos, const string& filename, const float distanceModifier = 1.f);
		void setLosOrigin(const Geo2d::Vector2& dat);
		void ensureCellExists(const int cellId);
		void clearCellLayer(const int cellId, const int layer);
		void getCellSpriteDimensions(MapCellClient const* cell, int& width, int& height) const;
		void setTerrainZone(const int terrainId, const int zondId);
		void setTerrainArea(const int terrainId, const int areaId);
		
		void setSkipLayerRender(const GameMap::Defines layer, const bool add) { if (add) m_skipLayerRender.insert(layer); else m_skipLayerRender.erase(layer); }
		void registerExtraRender(shared_ptr<Sprite> sprite, const GameMap::Defines layer, const sf::Vector2i& baseScreenPos);
		void setRevealOccupied(const GameMap::Defines layer, const bool val) { if (val) m_revealOccupied.insert(layer); else m_revealOccupied.erase(layer); }
		void setRenderEmptyCells(const bool b) { m_renderEmptyCells = b; }
		void setRenderUnwalkableCells(const bool b) { m_renderUnwalkableCells = b; }
		void setRenderBlockingCells(const bool b) { m_renderBlockingCells = b; }
		// DEBUG: numbered cell/terrain id overlay (toggle in-game with F9).
		void setRenderCellIds(const bool b) { m_renderCellIds = b; }
		void setAllowCameraDrag(const bool b) { m_allowCameraDragging = b; }
		void setGlobalLightPower(const float val) { m_globalLightPower = val; }
		void setShowClouds(const bool b) { m_showClouds = b; }
		void registerObjectOverhead(shared_ptr<ClientObjectOverhead> ptr) { m_objOverheads.push_back(ptr); }
		void transitionGlobalLight(const float pct) { m_globalLightPower_Target = pct; }

		void setSoundOrigin(const float x, const float y) { m_soundOrigin = { x, y }; }
		void registerTopmostSpriteAnim(shared_ptr<SpriteAnimation> spr, const sf::Vector2i& pos) { m_topmostSprAnims.push_back({ spr, pos }); }

		// Represents how many cells of extra space to render - by default it's 2
		void setPaddingRatio(const int val) { m_paddingRatio = val; }

		bool isReady() const;
		bool isCellHaveRenderDataForLayer(MapCellClient const* cell, const int layer) const;

		// The calculations thread checks for this and ends when is true
		bool isDone() const { return m_done; }
		bool isCellFitOnScreen(MapCellClient const* cell, const int cellId, const bool padding = false);
		bool outdoors() const { return m_outdoors; }
		
		int getZoneId(const int terrainId) const;
		int getAreaId(const int areaId) const;
		int renderPosToCellIdRelative(Geo2d::Vector2 renderPos, const bool floorCamera = true) const;
		int renderPosToTerrainIdRelative(Geo2d::Vector2 renderPos, const bool floorCamera = true) const;

		float readyDistance() const;
		float calcFoliageSkew(const Foliage& foilage) const;
		float calcProximityVolume(const sf::Vector2f&, const float distanceModifier = 1.f);

		const auto& getFootstepCellsRef() const { return m_footstepCells; }

		Geo2d::Vector2& getCameraRef() { return m_cameraPosition; }
		
		Geo2d::Vector2 getTerrainRenderPos(const int terrainId, const bool floorCamera = true) const;
		Geo2d::Vector2 getTerrainRenderPosRelative(const int terrainId, const bool floorCamera = true) const;
		Geo2d::Vector2 getCellRenderPosRelative(const int cellId, const bool floorCamera = true) const;
		Geo2d::Vector2 getCellRenderPosRelative(const int cellX, const int cellY, const bool floorCamera = true) const;		
		Geo2d::Vector2 getFlooredCamera() const;
		
		shared_ptr<Foliage> getFoliageRef(const string& textureName);
		shared_ptr<Sprite> getTerrain(const int terrainId) const;

		shared_ptr<WorldRenderable> getMousedRenderable() const { return m_mousedRenderable; }
		
	private:
		void input() final;
		void render() final;
		void startedLoading() final;
		void finishedLoading() final;
		void onFinishedLoadingCells() final;
		void onResize() final;
		void onTerrainTextureLoaded(const int terrainId, const string& texture) final;
		void onTerrainZoneLoaded(const int terrainId, const int zoneId) final;
		void onTerrainAreaLoaded(const int terrainId, const int areaId) final;
		void onCellDataLoaded(const int cellId, const int flags, const vector<shared_ptr<string>>& textures, const vector<float>& layerScale) final;
		
		void processDataToRender();
		void endCalculationThreads();
		void startCalculationThreads();
		void updateFoliage();
		void updateWorldRenderables();
		void updateProxmitySounds();
		void drawUnitOverheads();
		void drawTopmostSprAnims();

		inline void renderCalculationsThread();

		// Thread safe
		void setDataToRender(const RenderData& incoming);
		void getDataToRender(RenderData& output);
		void setCellsToUnload(set<int>& incoming);
		void getCellsToUnload(set<int>& output);
		void assignRenderableIntoCells(shared_ptr<WorldRenderable> wr);
		void cacheCalcThreadCells(const vector<RenderCell>& cells);
		void getCalcThreadCells(vector<RenderCell>& cells);

		static float getFoilageSkewFromWindTimer(const float timerAmount);

		// DEBUG: draws cell ids (+x,y) and terrain ids over the map when m_renderCellIds is on.
		void drawCellIdOverlay(const RenderData& data, const Geo2d::Vector2& flooredCamera);

		bool m_done;
		bool m_renderEmptyCells;
		bool m_renderUnwalkableCells;
		bool m_renderBlockingCells;
		bool m_renderCellIds{false};      // DEBUG overlay toggle (F9)
		bool m_debugKeyWasDown{false};    // F9 edge-detect
		shared_ptr<Text> m_debugIdText;   // lazy-init font for the overlay
		bool m_allowCameraDragging;
		bool m_showClouds;
		bool m_outdoors;

		int m_paddingRatio{4};

		float m_globalLightPower;
		float m_globalLightPower_Target;

		// All cells and terrain that are to be rendered
		RenderData m_cellsToRender;
		set<int> m_cellsToUnload;

		// Every cell that the player can see, includes cellId's that are nullptr, which is why this is not a part of RenderData
		vector<RenderCell> m_cachedCalcThreadCells;

		// CellId, ScreenPosition
		vector<pair<int, Geo2d::Vector2>> m_footstepCells;

		Geo2d::Vector2 m_losOrigin;
		Geo2d::Vector2 m_cameraPosition;
		Geo2d::Vector2 m_lastProcessedCameraPos;
		Geo2d::Vector2 m_cloudOffset;		
		Geo2d::Vector2 m_soundOrigin;

		sf::RenderTexture m_canvas;
		sf::RenderTexture m_lightingOverlay;
		sf::Vector2u m_lastScreenSize;
		
		recursive_mutex m_mutexRender;

		vector<thread> m_calculationThreads;

		set<string> m_proxSoundThisFrame;
		unordered_map<string, shared_ptr<SfSoundSafe>> m_proxmitySounds;

		shared_ptr<Sprite> m_emptyCellSprite;
		shared_ptr<Sprite> m_blockingCellSprite;
		shared_ptr<Sprite> m_unwalkableCellSprite;
		shared_ptr<Sprite> m_occupiedCellSprite;
		shared_ptr<Sprite> m_lightShader;
		shared_ptr<Sprite> m_lightSourceSprite;

		shared_ptr<sf::Texture> m_cloudsTexture;
		shared_ptr<WorldRenderable> m_mousedRenderable;

		vector<pair<shared_ptr<SpriteAnimation>, sf::Vector2i>> m_topmostSprAnims;
		vector<shared_ptr<ClientObjectOverhead>> m_objOverheads;
		vector<shared_ptr<Sprite>> m_terrain;

		set<shared_ptr<WorldRenderable>> m_worldRenderables;
		set<GameMap::Defines> m_skipLayerRender;
		set<GameMap::Defines> m_revealOccupied;

		map<int, int> m_terrainZone;
		map<int, int> m_terrainArea;

		unordered_map<string, shared_ptr<Foliage>> m_foilage;

		// Layer, vector<data>
		map<GameMap::Defines, vector<pair<shared_ptr<Sprite>, sf::Vector2i>>> m_extraRenders;
};

