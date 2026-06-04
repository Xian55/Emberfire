#include "stdafx.h"
#include "CastBar.h"
#include "ClientUnit.h"
#include "Sprite.h"
#include "ContentMgr.h"
#include "SpellIcon.h"
#include "Text.h"

#include "..\StringHelpers.h"
#include "..\SqlConnector\QueryResult.h"

#include <assert.h>

CastBar::CastBar(RenderObject& owner, const int id, const sf::Vector2i& pos) :
	RenderObject(&owner, id)
{
	m_timerTxt = make_shared<Text>(sContentMgr->getFont(FontId::Palatino));
	m_timerTxt->setCharacterSize(11);
	m_timerTxt->setOutlineThickness(2.f);
	m_timerTxt->setColorIfNot(sf::Color(124, 106, 94, 255));
	m_timerTxt->setOutlineColor(sf::Color(0, 0, 0, 100));

	m_spellNameTxt = make_shared<Text>(sContentMgr->getFont(FontId::Palatino));
	m_spellNameTxt->setCharacterSize(11);
	m_spellNameTxt->setOutlineThickness(2.f);
	m_spellNameTxt->setColorIfNot(sf::Color(124, 106, 94, 255));
	m_spellNameTxt->setOutlineColor(sf::Color(0, 0, 0, 100));

	m_icon = make_shared<SpellIcon>(owner, id, "gameicon19");
	ASSERT(m_sprite = sContentMgr->spawnSprite("castbar.png"));
	ASSERT(m_bar = sContentMgr->spawnSprite("castbar_fill.png"));
	getTopLeftCornerRef() = pos;
}

CastBar::~CastBar()
{

}

void CastBar::input() /*final*/
{

}

void CastBar::render() /*final*/
{
	if (m_unit == nullptr || m_unit->getCastSpellId() == 0)
		return;

	// New entry detected
	if (m_icon->getEntry() != m_unit->getCastSpellId())
	{
		auto& sv = sContentMgr->db("spell_template");
		m_spellNameTxt->setOriginalString(sv.data(m_unit->getCastSpellId(), "name"));
		m_icon->changeEntry(m_unit->getCastSpellId());
	}

	m_sprite->render(sf::Vector2f(getTopLeftCornerRef()));

	m_icon->getTopLeftCornerRef() = getTopLeftCornerRef() + sf::Vector2i(9, 7);
	m_icon->attemptRender();

	m_spellNameTxt->draw(getTopLeftCornerRef().x + 32, getTopLeftCornerRef().y + 5);	

	int milisecondsLeft = int(m_unit->getCastStopTime()) - int(std::clock());
	int totalTime = int(m_unit->getCastStopTime()) - int(m_unit->getCastStartTime());
	int elapsedTime = totalTime - milisecondsLeft;
	int pct = int((float(elapsedTime) / float(totalTime)) * 100.f);
	pct = min(100, pct);

	m_timerTxt->setOriginalString(Util::fmtStr("%d%%", pct).c_str());
	m_timerTxt->draw(getTopLeftCornerRef().x + 150, getTopLeftCornerRef().y + 5);

	int barWidth = int((float(pct) / 100.f) * float(m_bar->getTexture()->getSize().x));
	m_bar->setTextureRect(sf::IntRect(0, 0, barWidth, m_bar->getTexture()->getSize().y));
	m_bar->render(sf::Vector2f(getTopLeftCornerRef() + sf::Vector2i(33, 22)));
}

void CastBar::setUnit(shared_ptr<ClientUnit> ptr)
{
	m_unit = ptr;
}