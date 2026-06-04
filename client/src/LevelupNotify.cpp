#include "stdafx.h"
#include "LevelupNotify.h"
#include "World.h"
#include "ContentMgr.h"
#include "Sprite.h"
#include "TextBox.h"
#include "Application.h"
#include "ClientMap.h"
#include "ClientPlayer.h"
#include "Minimap.h"

#include "..\Shared\GameDefines.h"
#include "..\SqlConnector\QueryResult.h"

#include <sstream>

LevelupNotify::LevelupNotify(World& owner, const int id) :
	WorldChild(owner, id, owner),
	m_alpha(1.f),
	m_solidTimerSeconds(0.f),
	m_introAlpha(0.f)
{
	m_frame = sContentMgr->spawnSprite("levelup_notify.png");

	m_title = make_shared<TextBox>(sContentMgr->getFont(FontId::Ringbearer), 44);
}

LevelupNotify::~LevelupNotify()
{

}

void LevelupNotify::input() /*final*/
{

}

void LevelupNotify::render() /*final*/
{
	__super::render();

	if (m_introAlpha >= 0.f)
	{
		m_introAlpha -= sApplication->delta();
		m_alpha = 1.f - m_introAlpha;
	}
	else
	{
		if (m_solidTimerSeconds <= 0.f)
			m_alpha -= sApplication->delta();
		else
			m_solidTimerSeconds -= sApplication->delta();
	}

	if (m_alpha > 1.f)
		m_alpha = 1.f;

	if (m_alpha <= 0.f)
		return;

	// Positions	
	getTopLeftCornerRef().x = sApplication->centerOfScreen().x - int(m_frame->getGlobalBounds().width / 2);
	getTopLeftCornerRef().y = sApplication->centerOfScreen().y + 100;

	m_title->setData(getTopLeftCornerRef().x + 286, getTopLeftCornerRef().y + 45, m_txt, 699, TextBox::AlignCenterAbsolute, true, 1.f);

	// Alpha is dynamic
	m_title->setColor(sf::Color(232, 205, 182, uint8_t(255.f * m_alpha)), sf::Color(0, 0, 0, uint8_t(125.f * m_alpha)));
	m_frame->setColor(sf::Color(255, 255, 255, uint8_t(255.f * m_alpha)));

	// Draw
	m_frame->render(sf::Vector2f(getTopLeftCornerRef()));
	m_title->draw();
}

void LevelupNotify::onLevelTo(const int level)
{ 
	m_alpha = 1.f;
	m_solidTimerSeconds = 3.f;
	m_introAlpha = 1.f;
	m_txt = sContentMgr->db("player_exp_levels").data(level, "name");
	transform(m_txt.begin(), m_txt.end(), m_txt.begin(), ::tolower);
}