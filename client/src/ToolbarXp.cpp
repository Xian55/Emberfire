#include "stdafx.h"
#include "ToolbarXp.h"
#include "UnitFrame.h"
#include "ContentMgr.h"
#include "World.h"
#include "ClientPlayer.h"
#include "Text.h"
#include "Sprite.h"

#include "..\SqlConnector\QueryResult.h"
#include "..\StringHelpers.h"

ToolbarXp::ToolbarXp(World& owner, const int id) :
	WorldChild(owner, id, owner)
{
	setMultiInput(true);
	m_sprite = sContentMgr->spawnSprite("xp_bar.png");
	m_text = make_unique<Text>(sContentMgr->getFont(FontId::Palatino));
	m_text->setCharacterSize(12);
	m_text->setShadowOffset(1);
}

ToolbarXp::~ToolbarXp()
{

}

void ToolbarXp::input()
{
	__super::input();
}

void ToolbarXp::render()
{
	__super::render();

	if (world().myself() != nullptr)
	{
		auto& sv = sContentMgr->db("player_exp_levels");
		int requiredAmount = atoi(sv.data(world().myself()->getLevel() + 1, "exp").c_str());
		int curAmount = world().myself()->getVariable(ObjDefines::Variable::Progression);

		if (requiredAmount <= 0)
			return;

		float pct = float(curAmount) / float(requiredAmount);

		if (pct < 0)
			pct = 0;

		if (pct > 1.f)
			pct = 1.f;

		UnitFrame::renderBarCrop(*m_sprite, getTopLeftCornerRef(), pct);
		getBottomRightCornerRef() = getTopLeftCornerRef() + sf::Vector2i(m_sprite->getTexture()->getSize());

		if (MouseableNode::isMousedOver(true))
		{
			m_text->setOriginalString(Util::fmtStr("%d / %d", curAmount, requiredAmount));
			m_text->draw(getTopLeftCornerRef().x + int(m_sprite->getTexture()->getSize().x / 2) - int(m_text->getGlobalBounds().width / 2.f), getTopLeftCornerRef().y - 2);
		}
	}
}