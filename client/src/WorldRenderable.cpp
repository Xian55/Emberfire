#include "stdafx.h"
#include "WorldRenderable.h"
#include "MapCellClient.h"
#include "ClientMap.h"
#include "ContentMgr.h"
#include "Application.h"
#include "World.h"
#include "Game.h"

#include <assert.h>

WorldRenderable::WorldRenderable(ClientMap* clientMap) :
	m_done(false),
	m_currentCell(-1),
	m_cameraPtr(nullptr),
	m_map(clientMap)
{
	if (auto game = dynamic_pointer_cast<Game>(sApplication->getRenderObject(Application::Ro::RoGame)))
		m_world = dynamic_pointer_cast<World>(game->getRenderObject(game->Ro::RoWorld)).get();
}

WorldRenderable::~WorldRenderable()
{

}

void WorldRenderable::render()
{
	computeScreenPosition();
}

void WorldRenderable::update()
{
	computeScreenPosition();
}

void WorldRenderable::computeScreenPosition()
{
	if (m_cameraPtr != nullptr)
	{
		Geo2d::Vector2 pos = computeRawScreenPosition();
		Geo2d::Vector2 flooredCamera = *m_cameraPtr;
		flooredCamera.floorSelf();
		
		pos.x += flooredCamera.x;
		pos.y += flooredCamera.y;

		m_screenPosition = { static_cast<int>(pos.x), static_cast<int>(pos.y) };
	}
}

void WorldRenderable::setCurrentCell(const int c, MapCellClient& cell)
{
	m_currentCell = c;
}

void WorldRenderable::setCameraPtr(Geo2d::Vector2 const* ptr)
{
	m_cameraPtr = ptr;
	initCamera();
}

Geo2d::Vector2 WorldRenderable::computeRawScreenPosition() const
{
	return GameMap::computeRawScreenPosition(m_worldPosition);
}

sf::Vector2i WorldRenderable::computeRawScreenPositioni() const
{
	const Geo2d::Vector2 pos = GameMap::computeRawScreenPosition(m_worldPosition);
	return { static_cast<int>(pos.x), static_cast<int>(pos.y) };
}

void WorldRenderable::playSound(const string& soundname, const float distanceModifier /*= 1.f*/, const float volume /*= 1.f*/, const int delayms /*= 0*/)
{
	float baseVol = 1.f;

	if (m_map != nullptr)
		baseVol = m_map->calcProximityVolume(sf::Vector2f(getScreenPosition()), distanceModifier);

	float vol = baseVol * volume;

	if (vol > 0)
	{
		if (delayms)
			sContentMgr->queueSound(soundname, delayms, vol);
		else
			sContentMgr->playSound(soundname, vol);
	}
}