#include "stdafx.h"
#include "TickBox.h"
#include "ContentMgr.h"
#include "ButtonScript.h"

TickBox::TickBox(RenderObject& owner, const int id, string bscriptUntick, string bscriptTick,
		const sf::Vector2i& position /*= sf::Vector2i()*/, const bool ticked /*= false*/, SfKeyEvent activateKey /*= SfKeyEvent{}*/) :
	Button(owner, bscriptUntick, id, position, activateKey),
	m_scriptUntickName(bscriptUntick),
	m_scriptTickName(bscriptTick),
	m_ticked(ticked)
{
	// Force script change
	m_ticked = !ticked;
	setTicked(ticked);
}

TickBox::~TickBox()
{

}

void TickBox::input()
{
	Button::input();

	if (popActivated())
		setTicked(!m_ticked);
}

void TickBox::setTicked(const bool ticked)
{
	if (m_ticked == ticked)
		return;

	if (auto script = sContentMgr->buttonScript(ticked ? m_scriptTickName : m_scriptUntickName))
	{
		m_spriteIdle = sContentMgr->spawnSprite(script->textureIdle());
		m_spriteHover = sContentMgr->spawnSprite(script->textureHover());
		m_spritePress = sContentMgr->spawnSprite(script->texturePress());
	}

	m_ticked = ticked;
}
