#include "stdafx.h"
#include "TimedMessageBox.h"
#include "ContentMgr.h"
#include "Application.h"
#include "..\Math.h"
#include "TextBox.h"
#include "Sprite.h"

#include <assert.h>

TimedMessageBox::TimedMessageBox(RenderObject& owner, const int id, const float numSeconds, const string& msg) :
	RenderObject(&owner, id),
	m_spriteBackdrop(sContentMgr->spawnSprite("error_popup.png")),
	m_timer(numSeconds),
	m_msg(msg)
{
	m_text = make_shared<TextBox>(sContentMgr->getFont(FontId::Palatino), 16);

	ASSERT(dynamic_cast<RenderObjectHolder*>(getOwner()) != nullptr);
}

TimedMessageBox::~TimedMessageBox()
{

}

void TimedMessageBox::input()
{
	//
}

void TimedMessageBox::render()
{
	m_topLeftCorner.x = static_cast<int>((sApplication->sWf() / 2.0f) - (m_spriteBackdrop->getGlobalBounds().width / 2.0f));
	m_topLeftCorner.y = static_cast<int>((sApplication->sHf() / 2.0f) - (m_spriteBackdrop->getGlobalBounds().height / 2.0f));

	m_text->setColor(sf::Color(139, 116, 95, 255));
	m_text->setData(m_topLeftCorner.x + 25, m_topLeftCorner.y + 60, m_msg, 510, TextBox::AlignCenterBounds, true);
	
	m_timer -= sApplication->delta();

	if (m_timer <= 0)
	{
		if (RenderObjectHolder* roh = dynamic_cast<RenderObjectHolder*>(getOwner()))
			roh->destroyObjectById(getId());
	}

	m_spriteBackdrop->render({ static_cast<float>(m_topLeftCorner.x), static_cast<float>(m_topLeftCorner.y) });
	m_text->draw();
}
