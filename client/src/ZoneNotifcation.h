#pragma once

#include "WorldChild.h"

#include "..\Geo2d.h"

class Sprite;
class TextBox;

class ZoneNotifcation : public WorldChild
{
	public:
		ZoneNotifcation(World& owner, const int id);
		virtual ~ZoneNotifcation();
		
		void onMapChange();

	private:
		void input() final;
		void render() final;
		
		void playArea(const int areaId);
		void playZone(const int zoneId);
		void setMinimapZone(const int zoneId);
		void setMinimapArea(const int areaId);

		void resetZomeMusicAmbience();
		void setMusic(const string& music);
		void setAmbience(const string& ambience);
		
		int m_currentZoneId{0};
		int m_currentAreaId{0};

		float m_alpha;
		float m_introAlpha;
		float m_solidTimerSeconds;
		float m_checkZoneTimer;

		bool m_fadeIntro;
		
		map<string, clock_t> m_lastSound;
		map<int, clock_t> m_areaNotifyCooldown;
		
		string m_zoneName;
		string m_zoneName_Display;
		string m_zoneTypeStr;

		sf::Color m_territoryColor;

		shared_ptr<Sprite> m_frame;
		shared_ptr<TextBox> m_title;
		shared_ptr<TextBox> m_caption;
		
		Geo2d::Vector2 m_lastZoneChangeWorldPos;
};