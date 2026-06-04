#include "stdafx.h"
#include "SpriteScript.h"

#include "..\Shared\Config.h"
#include "..\StringHelpers.h"
#include "..\SqlConnector\SqlConnector.h"

#include <fstream>

SpriteScript::SpriteScript() :
	m_promixitySoundDistFactor(0.f),
	m_dummyTexture(false),
	m_lightIntensity(0),
	m_lightOnGround(false),
	m_lightAboveAll(false),
	m_lightScale(1.f)
{

}

SpriteScript::~SpriteScript()
{

}

void SpriteScript::setHotspot(const int x, const int y)
{
	m_hotspot.x = x;
	m_hotspot.y = y;
}

void SpriteScript::setHotspot(const string& scriptname, const int x, const int y)
{
	m_hotspot.x = x;
	m_hotspot.y = y;
	
	shared_ptr<SqlConnector> db = SqlConnector::create(SqlType::SQLite, sConfig->getString("GameDb", "Name"), sConfig->getString("GameDb", "Path"));
	db->query(Util::fmtStr("REPLACE INTO `sprite_hotspot` (`filename`, `x_offset`, `y_offset`) VALUES ('%s', '%d', '%d');", scriptname.c_str(), x, y));
}

void SpriteScript::editHotspot(const string& scriptname, const int xChange, const int yChange)
{
	setHotspot(scriptname, m_hotspot.x + xChange, m_hotspot.y + yChange);
}

void SpriteScript::setLight(const sf::Color& color, const int x_offset, const int y_offset, const int intensity, const bool lightOnGround, const bool lightAboveAll, float lightScale)
{
	m_lightColor = color;
	m_lightOffset.x = x_offset;
	m_lightOffset.y = y_offset;
	m_lightIntensity = intensity;
	m_lightOnGround = lightOnGround;
	m_lightAboveAll = lightAboveAll;
	m_lightScale = lightScale;
}

void SpriteScript::setPsi(const string& filename, const sf::Vector2i& pos)
{
	SpritePSI data;
	data.systemName = filename;
	data.relativePos = pos;
	m_psi.push_back(data);
}