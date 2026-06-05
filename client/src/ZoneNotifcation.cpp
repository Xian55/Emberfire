#include "stdafx.h"
#include "ZoneNotifcation.h"
#include "World.h"
#include "ContentMgr.h"
#include "Sprite.h"
#include "TextBox.h"
#include "Application.h"
#include "ClientMap.h"
#include "ClientPlayer.h"
#include "Minimap.h"
#include "lua/LuaEngine.h"
#include "lua/LuaEvents.h"

#include "..\Shared\GameDefines.h"
#include "..\SqlConnector\QueryResult.h"

#include <sstream>

ZoneNotifcation::ZoneNotifcation(World& owner, const int id) :
	WorldChild(owner, id, owner),
	m_alpha(1.f),
	m_solidTimerSeconds(0.f),
	m_currentZoneId(0),
	m_checkZoneTimer(0.f),
	m_fadeIntro(false)
{
	m_frame = sContentMgr->spawnSprite("zone_notifaction.png");

	m_title = make_shared<TextBox>(sContentMgr->getFont(FontId::Ringbearer), 36);
	m_caption = make_shared<TextBox>(sContentMgr->getFont(FontId::Palatino), 18);
}

ZoneNotifcation::~ZoneNotifcation()
{

}

void ZoneNotifcation::input() /*final*/
{

}

void ZoneNotifcation::render() /*final*/
{
	__super::render();

	if (auto myself = world().myself())
	{
		m_checkZoneTimer += sApplication->delta();

		if (m_checkZoneTimer > 1.f)
		{
			m_checkZoneTimer = 0.f;

			// Derive the terrain cell from the player's WORLD position (row-major terrain
			// index). The screen-space iso inverse (renderPosToTerrainIdRelative) is
			// imprecise and lands on the wrong terrain cell, mislabelling the zone.
			int currentTerrainId = world().getMap().worldPosToTerrainId(myself->getWorldPosition());
			int currentZone = world().getMap().getZoneId(currentTerrainId);
			int currentArea = world().getMap().getAreaId(currentTerrainId);

			if (currentZone != m_currentZoneId)
				playZone(currentZone);

			if (currentArea != m_currentAreaId)
				playArea(currentArea);
					
			if (currentArea != 0)
				setMinimapArea(currentArea);
			else
				setMinimapZone(currentZone);
		}
	}

	// The banner display (frame + title + caption + fade) is now rendered by Lua (ZoneNotification.lua),
	// driven by the ZONE_CHANGED event fired from playZone(). This class keeps only the zone/area
	// detection + the music/ambience/minimap/light side effects.
}
	
void ZoneNotifcation::setMinimapArea(const int areaId)
{
	auto& zone_template = sContentMgr->db("area_template");

	// Let's just go ahead and update the minimap, too
	if (auto minimap = dynamic_pointer_cast<Minimap>(world().getRenderObject(World::MinimapObj)))
		minimap->setTitle(zone_template.data(areaId, "name"));
}
		
void ZoneNotifcation::setMinimapZone(const int zoneId)
{
	auto& zone_template = sContentMgr->db("zone_template");

	// Let's just go ahead and update the minimap, too
	if (auto minimap = dynamic_pointer_cast<Minimap>(world().getRenderObject(World::MinimapObj)))
		minimap->setTitle(zone_template.data(zoneId, "name"));
}

void ZoneNotifcation::playZone(const int zoneId)
{
	auto& zone_template = sContentMgr->db("zone_template");

	// Put existing zone on cooldown, too
	m_lastSound[m_zoneName] = std::clock();

	m_currentZoneId = zoneId;
	m_zoneName = zone_template.data(zoneId, "name");

	setMinimapZone(zoneId);

	// Sound on a new zone is suppose to be after traveling some distance from A to B, not running back and forth between zone lines
	bool skipAnimation = false;

	if (m_lastSound[m_zoneName] != 0 && std::clock() - m_lastSound[m_zoneName] < 60000)
		skipAnimation = true;

	int zoneType = atoi(zone_template.data(zoneId, "type").c_str());

	if (zoneType == GameDefines::ZoneTypes::Contested)
		skipAnimation = false;
	
	if (m_fadeIntro)
		m_introAlpha = 1.f;

	if (!skipAnimation)
	{
		m_alpha = 1.f;
		m_solidTimerSeconds = 3.f;
		m_currentZoneId = zoneId;
		m_zoneName_Display = m_zoneName;

		switch (zoneType)
		{
			case GameDefines::ZoneTypes::Friendly:
			{
				m_zoneTypeStr = "Friendly Territory";
				m_territoryColor = sf::Color::Green;
				sContentMgr->playSound(SfxId::NewZone, 0.5f);
				break;
			}
			case GameDefines::ZoneTypes::Neutral:
			{
				m_zoneTypeStr = "Neutral Territory";
				m_territoryColor = sf::Color::Yellow;
				break;
			}
			case GameDefines::ZoneTypes::Hostile:
			{
				m_zoneTypeStr = "Hostile Territory";
				m_territoryColor = sf::Color::Red;
				sContentMgr->playSound(SfxId::PowerZone, 0.5f);
				break;
			}
			case GameDefines::ZoneTypes::Contested:
			{
				m_zoneTypeStr = "Contested Territory (PvP)";
				m_territoryColor = sf::Color(255, 165, 0);
				sContentMgr->playSound(SfxId::PowerZone, 0.5f);
				break;
			}
			default:
			{
				m_zoneTypeStr = "Unknown Territory";
				m_territoryColor = sf::Color::Yellow;
				break;
			}
		}

		// The banner display lives in Lua (ZoneNotification.lua). Hand it the zone name, territory label
		// and color; Lua runs the fade-in/solid/fade-out animation. Encoded as "name|type|r|g|b".
		std::stringstream payload;
		payload << m_zoneName_Display << '|' << m_zoneTypeStr << '|'
		        << int(m_territoryColor.r) << '|' << int(m_territoryColor.g) << '|' << int(m_territoryColor.b);
		sLua->fire(LuaEvents::ZONE_CHANGED, payload.str());
	}

	m_fadeIntro = false;

	// Swap music track	
	resetZomeMusicAmbience();

	// Possible night effect
	world().getMap().transitionGlobalLight(1.0f - (float)atof(zone_template.data(zoneId, "night_pct").c_str()));

	m_lastSound[m_zoneName] = std::clock();
	m_lastZoneChangeWorldPos = world().myself()->getWorldPosition();
}
		
void ZoneNotifcation::resetZomeMusicAmbience()
{
	auto& zone_template = sContentMgr->db("zone_template");

	string music = zone_template.data(m_currentZoneId, "music");
	string ambience = zone_template.data(m_currentZoneId, "ambience");

	setMusic(music);
	setAmbience(ambience);
}
		
void ZoneNotifcation::setAmbience(const string& ambience)
{
	if (!sContentMgr->compareAmbienceTrack(ambience))
		sContentMgr->queueAmbienceTransition(ambience);
}
		
void ZoneNotifcation::setMusic(const string& music)
{
	string str;
	stringstream ss(music);
	vector<string> musics;

	while (getline(ss, str, ','))
		musics.push_back(str);

	if (musics.empty())
		musics.push_back(music);

	if (!sContentMgr->compareMusicPlaylist(musics))
		sContentMgr->setMusicPlaylist(musics);
}

void ZoneNotifcation::playArea(const int areaId)
{
	m_currentAreaId = areaId;

	if (areaId == 0)
	{
		resetZomeMusicAmbience();
	}
	else
	{
		auto& area_template = sContentMgr->db("area_template");

		if (m_areaNotifyCooldown[areaId] < std::clock())
		{
			world().pushScrollingMessage(area_template.data(areaId, "name"), sf::Color(232, 205, 182, 255));
			m_areaNotifyCooldown[areaId] = std::clock() + 60000;
		}

		string music = area_template.data(m_currentAreaId, "music");
		string ambience = area_template.data(m_currentAreaId, "ambience");

		// Let zone music/ambience continue if we have nothing
		if (!music.empty())
			setMusic(music);

		if (!ambience.empty())
			setAmbience(ambience);
	}
}

void ZoneNotifcation::onMapChange() 
{ 
	m_alpha = 1.f;
	m_solidTimerSeconds = 0.f;
	m_currentZoneId = 0;
	m_checkZoneTimer = 0.f;
	m_fadeIntro = true;
	m_checkZoneTimer = 0.f;
	m_lastSound.clear();
	m_areaNotifyCooldown.clear();
	m_lastZoneChangeWorldPos.clear();
}