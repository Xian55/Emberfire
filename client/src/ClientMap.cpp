#include "stdafx.h"
#include "ClientMap.h"
#include "Application.h"
#include "ContentMgr.h"
#include "Text.h"
#include "MapCellClient.h"
#include "Sprite.h"
#include "ClientObject.h"
#include "WorldSpellAnimation.h"
#include "ClientObjectOverhead.h"
#include "SpriteAnimation.h"

#include "..\Rand.h"
#include "..\Files.h"

#include "..\Shared\MapLogic.h"
#include "..\Shared\StlBuffer.h"

#include <assert.h>

ClientMap::ClientMap(RenderObject& owner, const int id) :
	RenderObject(&owner, id),
	m_done(false),
	m_renderEmptyCells(false),
	m_allowCameraDragging(false),
	m_renderUnwalkableCells(false),
	m_renderBlockingCells(false),
	m_showClouds(true),
	m_mousedRenderable(0),
	m_globalLightPower(1.f),
	m_globalLightPower_Target(1.f)
{
	setMouseable(false);
	loadFoliageNames();
	m_terrain.resize(getTerrainWidth() * getTerrainWidth());	

	m_lightShader = sContentMgr->spawnSprite("shader_light.png");

	m_emptyCellSprite = sContentMgr->spawnSprite("map_blank_cell.png");
	m_emptyCellSprite->setHotspotEasy(true, false);

	m_blockingCellSprite = sContentMgr->spawnSprite("mapeditor_block_tile.png");
	m_blockingCellSprite->setHotspotEasy(true, false);

	m_unwalkableCellSprite = sContentMgr->spawnSprite("mapeditor_unwalkable_tile.png");
	m_unwalkableCellSprite->setHotspotEasy(true, false);

	m_occupiedCellSprite = sContentMgr->spawnSprite("mapeditor_occupied_tile.png");
	m_occupiedCellSprite->setHotspotEasy(true, false);
	
	m_lightSourceSprite = sContentMgr->spawnSprite("light_source.png");
	m_lightSourceSprite->setHotspotEasy(true, true);
	m_lightSourceSprite->setBlendMode(sf::BlendAdd);

	m_cloudsTexture = sContentMgr->getTexture("clouds.png");
	m_cloudsTexture->setRepeated(true);

	m_bottomRightCorner = { sApplication->sW(), sApplication->sH() };

	startCalculationThreads();
}

ClientMap::~ClientMap()
{
	endCalculationThreads();

	for (auto& itr : m_proxmitySounds)
		itr.second->setLoop(false);

	m_proxmitySounds.clear();
}

void ClientMap::input()
{
	if (m_allowCameraDragging)
	{
		setLoneAnchor(&m_cameraPosition);
		updateDraggableObject(*this, true);
	}

	updateWorldRenderables();
}

void ClientMap::render()
{
	m_bottomRightCorner = { sApplication->sW(), sApplication->sH() };

	// DEBUG: F9 toggles the cell-FLAG explorer — labels each visible cell with its raw .map flag byte
	// so we can read which flag value actually sits on obstacles (trees/fences/water) vs walkable ground.
	// (The reconstructed MapCellT bit names Unwalkable=0x10/CollideBlock=0x08 are unverified; this shows
	//  ground truth. Common bits 0x20/0x40/0x60 are on walkable ground, so they're skipped as noise.)
	{
		const bool f9Down = sf::Keyboard::isKeyPressed(sf::Keyboard::F9);
		if (f9Down && !m_debugKeyWasDown)
			m_renderCellIds = !m_renderCellIds;
		m_debugKeyWasDown = f9Down;
	}

	if (sApplication->isAudioDeviceChangedThisTick())
	{
		for (auto& itr : m_proxmitySounds)
			itr.second->stop();

		m_proxmitySounds.clear();
	}

	// The loudest one needs to be reconsidered every frame, otherwise a loud volume will be stuck as the closest even if we aren't close to it anymore
	for (auto& itr : m_proxmitySounds)
		itr.second->rememberVolume(0.f);
		
	// Draw everything in the map onto this canvas object
	if (m_canvas.getSize() != sf::Vector2u(sApplication->sW(), sApplication->sH()))
		m_canvas.create(sApplication->sW(), sApplication->sH());
	
	m_canvas.clear();
	sApplication->registerCanvas(&m_canvas);

	processDataToRender();
	updateProxmitySounds();
	updateFoliage();
	
	// We're done drawing to the canvas
	m_canvas.display();
	sApplication->resetCanvas();
	sApplication->canvas().draw(sf::Sprite(m_canvas.getTexture()), &sContentMgr->getShader(Assets::ShaderId::BrightContrast));

	drawUnitOverheads();
	drawTopmostSprAnims();

	// If something is on top of the world, like the options menu, then we're no longer processing input, but we still need to update world renderables
	// Normally we call this function in the input function because we need to input in the same cycle that we call the updating for clicking on in game objects
	// However, if we're not inputing, then this is a "backup plan" that will still pump the logic for world object movement etc
	if (!wasInputLastFrame())
		updateWorldRenderables();

	m_cloudOffset.x += sApplication->delta() * 50;
	m_cloudOffset.y += sApplication->delta() * 2;
}

void ClientMap::processDataToRender()
{
	set<int> cellsToUnload;
	getCellsToUnload(cellsToUnload);

	for (int cellId : cellsToUnload)
	{		
		if (auto cell = static_cast<MapCellClient*>(getCell(cellId)))
			cell->unloadSprites();
	}

	RenderData dataToRender;
	getDataToRender(dataToRender);

	for (auto& terrainId : dataToRender.terrain)
	{
		if (auto terrain = getTerrain(terrainId))
		{
			// Render position needs to be decided every frame based on where the camera is
			const auto renderPosition = getTerrainRenderPosRelative(terrainId);
			terrain->render({ renderPosition.x, renderPosition.y });
		}
	}
	
	// Must be cleared before every render
	m_footstepCells.clear();

	vector<MapCellClient::LightSource> lightSources;
	vector<MapCellClient::LightSource> lightSourcesLocal;
	const Geo2d::Vector2 flooredCamera = getFlooredCamera();

	// This is approaching the limit of how many times we want to call draw() every frame
	for (int l = 0; l < GameMap::Defines::NumLayers; ++l)
	{
		if (!m_skipLayerRender.empty() && m_skipLayerRender.find(static_cast<GameMap::Defines>(l)) != m_skipLayerRender.end())
			continue;

		for (auto& cellId : dataToRender.cells[l])
		{
			// Render position needs to be decided every frame based on where the camera now is since the calculations were made.
			const Geo2d::Vector2 renderPosition = cellId.baseRenderPos + flooredCamera;
			auto cell = static_cast<MapCellClient*>(getCell(cellId.cellId));

			// Register lighting
			if (l == GameMap::Defines::Layer3 && cell != nullptr)
			{
				if (cell->emitsLight(lightSourcesLocal, sf::Vector2i{ static_cast<int>(renderPosition.x), static_cast<int>(renderPosition.y) }) && !lightSourcesLocal.empty())
				{
					for (auto& itr : lightSourcesLocal)
					{
						if (itr.appearOnGround)
						{
							m_lightSourceSprite->setColor(itr.color);
							m_lightSourceSprite->setScale({itr.scale, itr.scale});
							m_lightSourceSprite->render(sf::Vector2f(itr.screenPos));
						}

						lightSources.push_back(itr);
					}

					lightSourcesLocal.clear();
				}
			}

			if (cell != nullptr)
			{
				cell->renderLayer({ renderPosition.x, renderPosition.y + (GameMap::Defines::BaseCellHeight / 2) }, l, *this);

				if (cell->hasFootsteps())
					m_footstepCells.push_back({cellId.cellId, renderPosition});
			}

			// For map editor
			else if (m_renderEmptyCells && l == GameMap::Defines::Layer1)
			{
				m_emptyCellSprite->render({ renderPosition.x, renderPosition.y });
			}
		}

		// Use lighting
		if (l == GameMap::Defines::Layer3 && !lightSources.empty())
		{
			for (auto& itr : lightSources)
			{
				if (itr.appearAboveAll)
				{
					m_lightSourceSprite->setColor(itr.color);
					m_lightSourceSprite->setScale({itr.scale, itr.scale});
					m_lightSourceSprite->render(sf::Vector2f(itr.screenPos) + sf::Vector2f(GameMap::BaseCellWidth / 4.f, GameMap::BaseCellHeight / 4.f));
				}
			}
		}

		// Some sprites that got registered to be rendered last
		auto itr = m_extraRenders.find(GameMap::Defines(l));

		if (itr != m_extraRenders.end())
		{
			for (auto& subitr : itr->second)
				subitr.first->render(sf::Vector2f(subitr.second) + sf::Vector2f(flooredCamera.x, flooredCamera.y));

			m_extraRenders.erase(itr);
		}

		if (l == GameMap::Layer2)
		{
			// Cover ground only with clouds
			if (m_showClouds)
			{
				sf::Sprite cloudsSprite(*m_cloudsTexture.get(),
					sf::IntRect(-int(m_cameraPosition.x + m_cloudOffset.x), -int(m_cameraPosition.y + m_cloudOffset.y), m_cloudsTexture->getSize().x, m_cloudsTexture->getSize().y));
				sApplication->canvas().draw(cloudsSprite);
			}
		}
	}

	// For map editor
	if (m_renderUnwalkableCells || m_renderBlockingCells || !m_revealOccupied.empty())
	{
		for (auto& cellId : dataToRender.cells[GameMap::Defines::Layer3])
		{
			if (auto cell = getCell(cellId.cellId))
			{
				const Geo2d::Vector2 renderPosition = cellId.baseRenderPos + flooredCamera;

				if (m_renderUnwalkableCells && cell->hasFlag(MapCellT::Flags::Unwalkable))
					m_unwalkableCellSprite->render({ renderPosition.x, renderPosition.y });

				if (m_renderBlockingCells && cell->hasFlag(MapCellT::Flags::CollideBlock))
					m_blockingCellSprite->render({ renderPosition.x, renderPosition.y });
			}
		}

		set<int> cellsToUse;

		for (auto& itr : m_revealOccupied)
		{
			for (auto& cellId : dataToRender.cells[itr])
			{
				if (auto cell = static_cast<MapCellClient*>(getCell(cellId.cellId)))
				{
					if (cell->hasTextureForLayer(itr))
						cellsToUse.insert(cellId.cellId);
				}
			}
		}

		for (auto& itr : cellsToUse)
		{
			const auto renderPosition = getCellRenderPosRelative(itr);
			m_occupiedCellSprite->render({ renderPosition.x, renderPosition.y });
		}
	}

	float gldiff = m_globalLightPower_Target -  m_globalLightPower;

	if (gldiff != 0.f)
		m_globalLightPower += gldiff * sApplication->delta();

	// Lighting
	if (m_globalLightPower < 1.f)
	{
		if (m_lightingOverlay.getSize() != sf::Vector2u(sApplication->sW(), sApplication->sH()))
			m_lightingOverlay.create(sApplication->sW(), sApplication->sH());

		m_lightingOverlay.clear(sf::Color(0, 0, 0, static_cast<int>(255.f * (1.f - m_globalLightPower))));

		for (auto& itr : lightSources)
		{
			// 0,0 isn't the top corner, adjust for that
			const float fx = static_cast<float>(0 + itr.screenPos.x);
			const float fy = static_cast<float>(itr.screenPos.y);
			const float foriginx = static_cast<float>(m_lightShader->getTexture()->getSize().x) / 2.f;
			const float foriginy = static_cast<float>(m_lightShader->getTexture()->getSize().y) / 2.f;

			sf::Sprite sprite;
			sprite.setTexture(*m_lightShader->getTexture());
			sprite.setPosition({ fx + (GameMap::BaseCellWidth / 4.f), fy + (GameMap::BaseCellHeight / 4.f) });
			sprite.setScale({ itr.scale, itr.scale });
			sprite.setOrigin({ foriginx, foriginy });
			sprite.setColor(sf::Color(25, 25, 25, 255));

			m_lightingOverlay.draw(sprite, sf::BlendMultiply);
		}

		// We're done drawing to the canvas
		m_lightingOverlay.display();
		sApplication->canvas().draw(sf::Sprite(m_lightingOverlay.getTexture()));
	}

	// DEBUG: numbered cell/terrain id overlay (drawn last so it sits on top of lighting).
	if (m_renderCellIds)
		drawCellIdOverlay(dataToRender, flooredCamera);
}

void ClientMap::drawCellIdOverlay(const RenderData& data, const Geo2d::Vector2& flooredCamera)
{
	if (!m_debugIdText)
	{
		m_debugIdText = make_shared<Text>(sContentMgr->getFont(FontId::Helvetica));
		m_debugIdText->setCharacterSize(9);
		m_debugIdText->setShadowOffset(1);
	}

	// COLLIDER overlay, data-backed from fanadin.map flag<->texture correlation:
	//   water  = flag & 0x08            (water_cell_tile / water_shallow)   -> blue '~'
	//   wall   = (flag & 0x30) == 0x30  (rockwall*gray)                     -> red  'X'
	// Bare 0x10 (dirt/leaf detail) and 0x20/0x40/0x60 (ground) are walkable -> not shown.
	size_t water = 0, wall = 0;
	m_debugIdText->setCharacterSize(14);
	for (const auto& rc : data.cells[GameMap::Defines::Layer3])
	{
		auto cell = getCell(rc.cellId);
		if (!cell) continue;
		const int f = cell->getFlags();
		const bool isWater = (f & 0x08) != 0;
		const bool isWall  = (f & 0x30) == 0x30;
		if (!isWater && !isWall) continue;
		const Geo2d::Vector2 p = rc.baseRenderPos + flooredCamera;
		if (isWall) { m_debugIdText->setColorIfNot(sf::Color(255, 60, 60), sf::Color::Black); m_debugIdText->setStringf("X"); ++wall; }
		else        { m_debugIdText->setColorIfNot(sf::Color(60, 160, 255), sf::Color::Black); m_debugIdText->setStringf("~"); ++water; }
		m_debugIdText->draw(static_cast<int>(p.x), static_cast<int>(p.y));
	}

	m_debugIdText->setCharacterSize(15);
	m_debugIdText->setColorIfNot(sf::Color(255, 255, 255), sf::Color::Black);
	m_debugIdText->setStringf("COLLIDER (F9)  red X=wall(0x30)  blue ~=water(0x08)\nin view: walls=%zu water=%zu  mapW=%d",
		wall, water, getMapWidth());
	m_debugIdText->draw(8, 70);
}

// Do the slow math in a separate thread
void ClientMap::renderCalculationsThread()
{
	set<int> loadedCells;
	clock_t lastCellPurge = std::clock();

	while (!isDone())
	{
		// We could change this loop to pause until the camera changes or "until we need to",
		//	however, "when we need to" is overly complex when you consider world objects will be moving around of their own volition.
		//	Therefore, simply update periodically.
		const clock_t lastCalc = std::clock();
		
		set<int> cellsThisFrame;
		set<int> cellsToUnload;
		bool unloadStaleCells = false;

		if (std::clock() - lastCellPurge > 3000)
		{
			lastCellPurge = std::clock();
			unloadStaleCells = true;
		}

		const Geo2d::Vector2 cachedCameraPos = m_cameraPosition;
		const Geo2d::Vector2 centerOfScreen(sApplication->sWf() / 2.0f, sApplication->sHf() / 2.0f);
		const int centerCell = renderPosToCellIdRelative(centerOfScreen);
		const int centerTerrain = renderPosToTerrainIdRelative(centerOfScreen);
		const int screenSize = max(sApplication->sW(), sApplication->sH());
		const int cellSize = min(GameMap::Defines::BaseCellHeight, GameMap::Defines::BaseCellWidth);
		const int terrainSize = cellSize * ClientMap::Defines::TerrainToCellRatio;

		// Dynamic Radius + A fixed amount to allow big sprites to appear off screen		
		const int cellSearchSize = ((screenSize / cellSize) / 2) + 14;
		const int terrainSearchSize = ((screenSize / terrainSize) / 2) + 2;

		vector<int> cells;
		MapLogic::getIndexesAround(cells, centerCell, getMapWidth(), cellSearchSize);

		RenderData result;
		MapLogic::getIndexesAround(result.terrain, centerTerrain, getTerrainWidth(), terrainSearchSize);
		
		// Careful of sort crashes
		auto sortTopdown = [&](const int i, const int j)
		{
			int iX, iY;
			Geo2d::computeCellPos(i, iX, iY, getMapWidth());

			int jX, jY;
			Geo2d::computeCellPos(j, jX, jY, getMapWidth());

			// We render from a diagonal perspective, therefore every half X is a full Y gain
			iY += iX / 2;
			jY += jX / 2;

			if (iY == jY)
				return iX < jX;

			return iY < jY;
		};

		// A neat trick is that the sorting by cellId will work for terrainId since they're both the same isometric layout
		std::sort(cells.begin(), cells.end(), sortTopdown);
		std::sort(result.terrain.begin(), result.terrain.end(), sortTopdown);		

		vector<RenderCell> everyLayerOne;

		// We could cache these results but that would eat up a ton of memory on big maps where you have 1000 * 1000 cells
		// This populateIndexiesToRender function should take about 50 ms on a slow machine (in release build), and it would be fine it took 500 ms
		for (auto& cellId : cells)
		{
			for (int l = 0; l < GameMap::Defines::NumLayers; ++l)
			{
				auto cell = dynamic_pointer_cast<MapCellClient>(getCellRef(cellId));

				if (cell != nullptr)
				{
					if (!cell->isLoaded())
					{
						cell->loadSprites();
						loadedCells.insert(cellId);
					}
				}

				Geo2d::Vector2 baseRenderPos = getCellRenderPos(cellId);

				bool fitonscreen = false;
				const bool haveData = isCellHaveRenderDataForLayer(cell.get(), l);

				if (haveData || l == GameMap::Defines::Layer1)
					fitonscreen = isCellFitOnScreen(cell.get(), cellId, true);

				if (haveData && fitonscreen)
					result.cells[l].push_back({ cellId, baseRenderPos });

				if (l == GameMap::Defines::Layer1 && fitonscreen)
					everyLayerOne.push_back({ cellId, baseRenderPos });
			}
		}

		// Calculations complete, push to the map to be rendered
		cacheCalcThreadCells(everyLayerOne);
		setDataToRender(result);
		m_lastProcessedCameraPos = cachedCameraPos;

		if (unloadStaleCells)
		{
			for (auto itr = loadedCells.begin(); itr != loadedCells.end();)
			{
				if (find(cells.begin(), cells.end(), *itr) == cells.end())
				{
					cellsToUnload.insert(*itr);
					itr = loadedCells.erase(itr);
				}
				else
				{
					++itr;
				}
			}

			setCellsToUnload(cellsToUnload);
		}
		
		const clock_t timeToCalc = std::clock() - lastCalc;
		constexpr decltype(timeToCalc) desiredSleepTime = 300;

		if (timeToCalc < desiredSleepTime)
			this_thread::sleep_for(chrono::milliseconds(desiredSleepTime - timeToCalc));
	}
}

void ClientMap::drawTopmostSprAnims()
{
	for (auto& itr : m_topmostSprAnims)
		itr.first->render(itr.second);

	m_topmostSprAnims.clear();
}

void ClientMap::drawUnitOverheads()
{
	// These need to always be visible on top of the map game world
	for (auto& itr : m_objOverheads)
	{
		itr->setDrawOptions(true, false);
		itr->draw();
	}

	for (auto& itr : m_objOverheads)
	{
		itr->setDrawOptions(false, true);
		itr->draw();
	}

	m_objOverheads.clear();
}

void ClientMap::endCalculationThreads()
{
	m_done = true;

	for (auto& itr : m_calculationThreads)
	{
		if (itr.joinable())
			itr.join();
	}

	m_calculationThreads.clear();
}

void ClientMap::startedLoading()
{
	m_worldRenderables.clear();
	endCalculationThreads();
	setDataToRender(ClientMap::RenderData{});
}

void ClientMap::startCalculationThreads()
{
	m_calculationThreads.push_back(thread(&ClientMap::renderCalculationsThread, this));
}

void ClientMap::finishedLoading()
{
	endCalculationThreads();
	m_done = false;
	startCalculationThreads();
	m_lastProcessedCameraPos = { -100.f, 100.f };
}

void ClientMap::setDataToRender(const RenderData& incoming)
{
	lock_guard<recursive_mutex> lock(m_mutexRender);
	m_cellsToRender = incoming;
}

void ClientMap::setCellsToUnload(set<int>& incoming)
{
	lock_guard<recursive_mutex> lock(m_mutexRender);

	if (m_cellsToUnload.empty())
	{
		m_cellsToUnload.swap(incoming);
	}
	else
	{
		for (auto& itr : incoming)
			m_cellsToUnload.insert(itr);
	}
}

void ClientMap::getCellsToUnload(set<int>& output)
{
	lock_guard<recursive_mutex> lock(m_mutexRender);
	output.swap(m_cellsToUnload);
}

void ClientMap::getDataToRender(RenderData& output)
{
	lock_guard<recursive_mutex> lock(m_mutexRender);
	output = m_cellsToRender;
}

void ClientMap::cacheCalcThreadCells(const vector<RenderCell>& cells)
{
	lock_guard<recursive_mutex> lock(m_mutexRender);
	m_cachedCalcThreadCells = cells;
}

void ClientMap::getCalcThreadCells(vector<RenderCell>& cells)
{
	lock_guard<recursive_mutex> lock(m_mutexRender);
	cells = m_cachedCalcThreadCells;
}

void ClientMap::assignRenderableIntoCells(shared_ptr<WorldRenderable> wr)
{
	const int prevCel = wr->getCurrentCell();
	const int newCellId = Geo2d::computeCellId(static_cast<int>(wr->getWorldPosition().x), static_cast<int>(wr->getWorldPosition().y), getMapWidth());

	if (prevCel != newCellId && newCellId >= 0 && newCellId < static_cast<int>(getNumCells()))
	{
		// Remove from old
		if (auto oldCell = dynamic_cast<MapCellClient*>(getCell(wr->getCurrentCell())))
			oldCell->eraseWorldRenderable(wr);

		// Add to new
		ensureCellExists(newCellId);
		auto newContainer = dynamic_pointer_cast<MapCellClient>(getCellRef(newCellId));
		newContainer->addWorldRenderable(wr);
		wr->setCurrentCell(newCellId, *newContainer.get());
	}
}

shared_ptr<Sprite> ClientMap::getTerrain(const int terrainId) const
{
	if (static_cast<size_t>(terrainId) >= m_terrain.size())
		return nullptr;

	return m_terrain[terrainId];
}

void ClientMap::ensureCellExists(const int cellId)
{
	ASSERT(static_cast<size_t>(cellId) < getNumCells());

	if (m_cells[cellId] != nullptr)
		return;

	m_cells[cellId] = make_shared<MapCellClient>(Defines::NumLayers);
}

void ClientMap::clearCellLayer(const int cellId, const int layer)
{
	ASSERT(static_cast<size_t>(cellId) < getNumCells());

	if (auto cell = dynamic_cast<MapCellClient*>(getCell(cellId)))
		cell->clearSprite(layer);
}

void ClientMap::getCellSpriteDimensions(MapCellClient const* cell, int& width, int& height) const
{
	if (cell != nullptr)
	{
		width = cell->getMaxWidth();
		height = cell->getMaxHeight();

		if (cell->emitsLightCached())
		{
			width = static_cast<int>(m_lightShader->getGlobalBounds().width);
			height = static_cast<int>(m_lightShader->getGlobalBounds().height);
		}
	}
	else
	{
		width = GameMap::Defines::BaseCellWidth;
		height = GameMap::Defines::BaseCellHeight;
	}
}

float ClientMap::readyDistance() const
{
	return sApplication->sHf();
}

bool ClientMap::isReady() const
{
	return m_lastProcessedCameraPos.getDist(m_cameraPosition) < readyDistance();
}

bool ClientMap::isCellHaveRenderDataForLayer(MapCellClient const* cell, const int layer) const
{
	// For map editor
	if (layer == GameMap::Defines::Layer1 && m_renderEmptyCells)
		return true;
	
	if (layer == GameMap::Defines::Layer3)
		return true;

	if (cell != nullptr)
		return cell->hasDataForLayer(layer);

	return false;
}

bool ClientMap::isCellFitOnScreen(MapCellClient const* cell, const int cellId, const bool padding /*= false*/)
{
	int width, height;
	getCellSpriteDimensions(cell, width, height);

	// If the camera is moving fast then the calculations thread won't be giving answers quickly enough and the edge will have some black spaces
	// This padding is so that we can say "pretend our screen is a little bigger"
	if (padding)
	{
		width += GameMap::Defines::BaseCellWidth * m_paddingRatio;
		height += GameMap::Defines::BaseCellHeight * m_paddingRatio;
	}

	const auto screenPos = getCellRenderPosRelative(cellId);

	if (
		// Vertical
		screenPos.y - (height) > sApplication->sH() || screenPos.y < -(height) ||

		// Horizontal
		screenPos.x - (width / 2.0f) > sApplication->sW() || screenPos.x < -(width / 2.0f))
	{
		return false;
	}

	return true;
}

int ClientMap::renderPosToTerrainIdRelative(Geo2d::Vector2 renderPos, const bool floorCamera /*= true*/) const
{
	const int customIsleWidth = getTerrainWidth();
	const int terrainWidth = GameMap::Defines::BaseCellWidth * ClientMap::Defines::TerrainToCellRatio;
	const int terrainHeight = GameMap::Defines::BaseCellHeight * ClientMap::Defines::TerrainToCellRatio;

	renderPos.subtract(floorCamera ? getFlooredCamera() : m_cameraPosition);
	return getCellId(renderPos, &customIsleWidth, &terrainWidth, &terrainHeight);
}

int ClientMap::getAreaId(const int areaId) const
{
	auto itr = m_terrainArea.find(areaId);

	if (itr == m_terrainArea.end())
		return 0;

	return itr->second;
}

int ClientMap::getZoneId(const int terrainId) const
{
	auto itr = m_terrainZone.find(terrainId);

	if (itr == m_terrainZone.end())
		return 0;

	return itr->second;
}

int ClientMap::renderPosToCellIdRelative(Geo2d::Vector2 renderPos, const bool floorCamera /*= true*/) const
{
	renderPos.subtract(floorCamera ? getFlooredCamera() : m_cameraPosition);
	return getCellId(renderPos);
}

Geo2d::Vector2 ClientMap::getTerrainRenderPos(const int terrainId, const bool floorCamera /*= true*/) const
{
	const int customIsleWidth = getTerrainWidth();
	const int terrainWidth = GameMap::Defines::BaseCellWidth * ClientMap::Defines::TerrainToCellRatio;
	const int terrainHeight = GameMap::Defines::BaseCellHeight * ClientMap::Defines::TerrainToCellRatio;
	return getCellRenderPos(terrainId, &customIsleWidth, &terrainWidth, &terrainHeight);
}

Geo2d::Vector2 ClientMap::getTerrainRenderPosRelative(const int terrainId, const bool floorCamera /*= true*/) const
{
	auto result = getTerrainRenderPos(terrainId, floorCamera);
	result.add(floorCamera ? getFlooredCamera() : m_cameraPosition);
	return result;
}

void ClientMap::getCellPositionRelative(int& x, int& y, Geo2d::Vector2 screenPos, const bool floorCamera /*= true*/)
{
	screenPos.subtract(floorCamera ? getFlooredCamera() : m_cameraPosition);
	getCellPosition(x, y, screenPos);
}

void ClientMap::getCellWorldPosRelative(float& x, float& y, Geo2d::Vector2 screenPos, const bool floorCamera /*= true*/)
{
	screenPos.subtract(floorCamera ? getFlooredCamera() : m_cameraPosition);
	const Geo2d::Vector2 result = renderPosToWorldPos(screenPos, nullptr, nullptr);
	x = result.x;
	y = result.y;
}

Geo2d::Vector2 ClientMap::getCellRenderPosRelative(const int cellId, const bool floorCamera /*= true*/) const
{
	auto result = getCellRenderPos(cellId);
	result.add(floorCamera ? getFlooredCamera() : m_cameraPosition);
	return result;
}

Geo2d::Vector2 ClientMap::getCellRenderPosRelative(const int cellX, const int cellY, const bool floorCamera /*= true*/) const
{
	auto result = getCellRenderPos(cellX, cellY);
	result.add(floorCamera ? getFlooredCamera() : m_cameraPosition);
	return result;
}

Geo2d::Vector2 ClientMap::getFlooredCamera() const
{
	Geo2d::Vector2 result = m_cameraPosition;
	result.floorSelf();
	return result;
}

void ClientMap::onCellDataLoaded(const int cellId, const int flags, const vector<shared_ptr<string>>& textures, const vector<float>& layerScale) /*final*/
{
	// m_cells is a sparse map keyed by cell index; validate against the grid extent
	// (the old size()-based bound was a vector-era artifact and is 0 before any insert).
	ASSERT(cellId >= 0 && cellId < getMapWidth() * getMapHeight());
	auto cell = make_shared<MapCellClient>(GameMap::Defines::NumLayers);
	cell->setFlags(flags);

	for (size_t i = 0; i < textures.size(); ++i)
	{
		cell->setLayerScale(i, layerScale[i]);

		if (textures[i] != nullptr && !sContentMgr->isTextureExist(*textures[i]) && !sContentMgr->isPsiExist(*textures[i]))
			blog(Logger::LOG_ERROR, "ClientMap: Texture '%s' does not exist.", textures[i]->c_str());
		else
			cell->setTexture(textures[i], static_cast<int>(i));
	}

	m_cells[cellId] = cell;
}

void ClientMap::onTerrainTextureLoaded(const int terrainId, const string& texture) /*final*/
{
	setTerrain(terrainId, texture);
}

void ClientMap::onTerrainAreaLoaded(const int terrainId, const int areaId) /*final*/
{
	setTerrainArea(terrainId, areaId);
}

void ClientMap::onTerrainZoneLoaded(const int terrainId, const int zoneId) /*final*/
{
	setTerrainZone(terrainId, zoneId);
}

void ClientMap::onFinishedLoadingCells() /*final*/
{
	m_terrain.clear();
	m_terrain.resize(getTerrainWidth() * getTerrainWidth());
	m_terrainZone.clear();
	m_terrainArea.clear();

	// this needs to change
	m_outdoors = getMapWidth() >= 1000;
	m_showClouds = m_outdoors;
}

void ClientMap::onResize() /*final*/
{
	m_terrain.clear();
	m_terrain.resize(getTerrainWidth() * getTerrainWidth());
}

void ClientMap::setTerrainArea(const int id, const int areaId)
{
	auto itr = m_terrainArea.find(id);

	// Just remove from the map if its 0
	if (itr != m_terrainArea.end() && areaId == 0)
	{
		m_terrainArea.erase(itr);
		return;
	}

	m_terrainArea[id] = areaId;
}

void ClientMap::setTerrainZone(const int id, const int zoneId)
{
	auto itr = m_terrainZone.find(id);

	// Just remove from the map if its 0
	if (itr != m_terrainZone.end() && zoneId == 0)
	{
		m_terrainZone.erase(itr);
		return;
	}

	m_terrainZone[id] = zoneId;
}

void ClientMap::setTerrain(const int id, const string& textureName)
{
	if (static_cast<size_t>(id) >= m_terrain.size())
		return;

	if (textureName.empty())
	{
		m_terrain[id] = nullptr;
		return;
	}

	if (auto spr = sContentMgr->spawnSprite(textureName))
	{
		spr->setHotspotEasy(true, true);
		m_terrain[id] = spr;
	}
	else
	{
		blog(Logger::LOG_ERROR, "Tried to set terrain %u to %s but did not find such a texture.", id, textureName.c_str());
	}
}

shared_ptr<ClientMap::Foliage> ClientMap::getFoliageRef(const string& textureName)
{
	auto itr = m_foilage.find(textureName);

	if (itr == m_foilage.end())
		return nullptr;

	if (!itr->second->init)
	{
		itr->second->init = true;
		itr->second->texture = sContentMgr->getTexture(textureName);
	}

	return itr->second;
}

float ClientMap::calcFoliageSkew(const ClientMap::Foliage& foilage) const
{
	const float result = getFoilageSkewFromWindTimer(foilage.pump);

	// Skew of 0 also means "don't skew" (which looks a little different than a drawing a skew of 0)
	// Kind of a hack fix, I guess? Not sure
	if (result == 0.0f)
		return 0.01f;

	return result;
}

/*static*/
float ClientMap::getFoilageSkewFromWindTimer(const float timerAmount)
{
	// Back to middle
	if (timerAmount >= BaseFoliageWindSkew * 3)
	{
		return timerAmount - (BaseFoliageWindSkew * 4);
	}

	// Towards negative
	else if (timerAmount >= BaseFoliageWindSkew * 2)
	{
		const float backWardAmount = (BaseFoliageWindSkew * 2) - timerAmount;
		return backWardAmount;
	}

	// Back to middle
	else if (timerAmount >= BaseFoliageWindSkew)
	{
		const float backWardAmount = (BaseFoliageWindSkew * 2) - timerAmount;
		return backWardAmount;
	}

	// Fist forward
	else
	{
		return timerAmount;
	}

	return 0.0f;
}

void ClientMap::loadFoliageNames()
{
	vector<string> filenames;
	Util::readLinesFromFile("scripts\\mapeditor_defs\\foliage.txt", filenames);

	m_foilage.clear();

	for (auto& str : filenames)
	{
		shared_ptr<Foliage> data = make_shared<Foliage>();
		data->pump = Util::frand(0.0f, BaseFoliageWindSkew * 4);
		data->init = false;
		m_foilage[str] = data;
	}
}

void ClientMap::addWorldRenderable(shared_ptr<WorldRenderable> wr)
{
	wr->setCameraPtr(&m_cameraPosition);
	m_worldRenderables.insert(wr);
}

void ClientMap::registerExtraRender(shared_ptr<Sprite> sprite, const GameMap::Defines layer, const sf::Vector2i& baseScreenPos)
{
	m_extraRenders[layer].push_back({ sprite, baseScreenPos });
}

void ClientMap::removeWorldRenderable(shared_ptr<WorldRenderable> wo)
{
	// Remove from cell
	if (auto oldCell = dynamic_cast<MapCellClient*>(getCell(wo->getCurrentCell())))
		oldCell->eraseWorldRenderable(wo);

	m_worldRenderables.erase(wo);
}

float ClientMap::calcProximityVolume(const sf::Vector2f& screenPos, const float distanceModifier /*= 1.f*/)
{
	// The further away, the quieter it is
	float distFromCenter = Geo2d::distance2d(static_cast<float>(m_soundOrigin.x),static_cast<float>(m_soundOrigin.y),
		static_cast<float>(screenPos.x), static_cast<float>(screenPos.y));
	
	// @distanceModifier
	distFromCenter *= distanceModifier;	
	return 1.f - (distFromCenter / 800.f);
}

void ClientMap::updateProxmitySounds()
{
	for (auto itr = m_proxmitySounds.begin(); itr != m_proxmitySounds.end();)
	{
		auto cachedCopy = m_proxSoundThisFrame.find(itr->first);

		// If it no longer exists on our screen
		if (cachedCopy == m_proxSoundThisFrame.end())
		{
			itr->second->setLoop(false);
			itr = m_proxmitySounds.erase(itr);
		}
		else
		{
			itr->second->popScreenPos();
			++itr;
		}
	}

	m_proxSoundThisFrame.clear();
}

void ClientMap::setLosOrigin(const Geo2d::Vector2& dat)
{ 
	m_losOrigin = dat;
}

void ClientMap::registryProximitySound(const sf::Vector2f& screenPos, const string& filename, const float distanceModifier /*= 1.f*/)
{
	m_proxSoundThisFrame.insert(filename);

	auto existingEntry = m_proxmitySounds.find(filename);

	if (existingEntry != m_proxmitySounds.end())
	{
		float newVol = calcProximityVolume(screenPos, distanceModifier);

		// Already playing this sound with stronger volume
		// The sound that plays is whichever is closest to the player
		if (existingEntry->second->getRememberedVolume() < newVol)
		{
			existingEntry->second->rememberVolume(newVol);
			existingEntry->second->rememberScreenPosition(screenPos);
		}

		return;
	}

	if (auto newsound = sContentMgr->playSound(filename, calcProximityVolume(screenPos, distanceModifier), screenPos))
	{
		newsound->setLoop(true);
		m_proxmitySounds[filename] = newsound;
	}
}

void ClientMap::updateFoliage()
{
	for (auto& itr : m_foilage)
	{
		if (itr.second->texture == nullptr)
			continue;

		const float textureHeight = float(itr.second->texture->getSize().y);

		float divider = 3.0f;

		// Shorter will move faster, taller will move slower
		if (textureHeight < 128.0f)
			divider = 10.0f;

		const float windRate = textureHeight / divider;
		const float skewAmount = getFoilageSkewFromWindTimer(itr.second->pump);
		const float skewMaxRatio = 1.0f - (abs(skewAmount) / BaseFoliageWindSkew);
		const float deltaDivide = min(windRate * 3, skewMaxRatio ? windRate / skewMaxRatio : windRate);

		itr.second->pump += sApplication->delta() / deltaDivide;

		if (itr.second->pump > BaseFoliageWindSkew * 4.0f)
			itr.second->pump = 0.0f;
	}
}

void ClientMap::updateWorldRenderables()
{
	m_mousedRenderable = nullptr;
	auto itr = m_worldRenderables.begin();

	while (itr != m_worldRenderables.end())
	{
		auto& ptr = *itr;

		if (ptr->done())
		{
			itr = m_worldRenderables.erase(itr);
		}
		else
		{
			ptr->update();
			assignRenderableIntoCells(ptr);

			if (auto clientObj = dynamic_pointer_cast<ClientObject>(ptr))
			{
				if (clientObj->isMousedOver(true))
					m_mousedRenderable = ptr;
			}

			++itr;
		}
	}
}

void ClientMap::saveToDisk()
{
	if (getName().empty())
		return;

	const string path = "maps\\" + getName() + ".map";
	blog(Logger::LOG_INFO, "Saving %s...", path.c_str());

	/**
	* Compile data
	*/

	set<string> textures;
	set<string> terrainTextures;

	vector<int> cells;
	vector<int> terrains;

	for (size_t i = 0; i < m_cells.size(); ++i)
	{
		auto cell = dynamic_pointer_cast<MapCellClient>(m_cells[i]);

		if (cell == nullptr)
			continue;

		if (!cell->hasData())
			continue;

		cells.push_back(static_cast<int>(i));
		
		for (int l = 0; l < Defines::NumLayers; ++l)
		{
			if (cell->getTextureName(l).size() > 0)
				textures.insert(cell->getTextureName(l));
		}
	}

	for (size_t i = 0; i < m_terrain.size(); ++i)
	{
		auto& itr = m_terrain[i];

		if (itr != nullptr && itr->getTextureName().size() > 0)
		{
			terrainTextures.insert(itr->getTextureName());
			terrains.push_back(i);
		}
	}

	/**
	* Save data
	*/

	StlBuffer buffer;
	buffer << static_cast<int>(getMapWidth());
	buffer << static_cast<int>(textures.size());

	for (auto& str : textures)
		buffer << str;

	// This isn't "the number of cells in the map", this is "the number of cells with data".
	buffer << static_cast<int>(cells.size());

	for (int cellId : cells)
	{
		auto cell = dynamic_pointer_cast<MapCellClient>(m_cells[cellId]);
		ASSERT(cell != nullptr);

		buffer << cellId;
		buffer << (unsigned char)cell->getFlags();
		
		for (int l = 0; l < Defines::NumLayers; ++l)
		{
			const string textureName = cell->getTextureName(l);
			const bool layerHasTexture = textureName.size() > 0;

			buffer << layerHasTexture;

			if (layerHasTexture)
			{
				auto itr = textures.find(textureName);
				const int textureIdx = std::distance(textures.begin(), itr);
				ASSERT(static_cast<size_t>(textureIdx) < textures.size());
				buffer << textureIdx;
				buffer << cell->getLayerScale(l);
			}
		}
	}
	
	/**
	* Terrain
	*/

	buffer << static_cast<int>(terrainTextures.size());
	
	// A map might not have terrain, this is the tell
	if (terrainTextures.size() > 0)
	{
		for (auto& str : terrainTextures)
			buffer << str;

		buffer << static_cast<int>(terrains.size());
		
		for (auto terrainId : terrains)
		{
			ASSERT(static_cast<size_t>(terrainId) < m_terrain.size());
			const string textureName = m_terrain[terrainId]->getTextureName();

			auto itr = terrainTextures.find(textureName);
			const int textureIdx = std::distance(terrainTextures.begin(), itr);
			ASSERT(static_cast<size_t>(textureIdx) < terrainTextures.size());

			buffer << static_cast<int>(terrainId);
			buffer << static_cast<int>(textureIdx);
		}
	}

	buffer << static_cast<int>(m_terrainZone.size());

	for (auto& itr : m_terrainZone)
	{
		buffer << static_cast<int>(itr.first);
		buffer << static_cast<int>(itr.second);
	}

	buffer << static_cast<int>(m_terrainArea.size());

	for (auto& itr : m_terrainArea)
	{
		buffer << static_cast<int>(itr.first);
		buffer << static_cast<int>(itr.second);
	}

	/**
	* Write to disk
	*/

	buffer.writeFile(path);
}