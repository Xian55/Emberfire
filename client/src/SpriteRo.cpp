#include "stdafx.h"
#include "SpriteRo.h"
#include "Sprite.h"

#include <assert.h>

SpriteRo::SpriteRo(RenderObject& owner, shared_ptr<Sprite> spr, const int id, const sf::Vector2i topLeftCorner /*= sf::Vector2i()*/) :
	RenderObject(&owner, id),
	m_sprite(spr)
{
	ASSERT(m_sprite != nullptr);
	m_topLeftCorner = topLeftCorner;
	m_bottomRightCorner = { m_topLeftCorner.x + static_cast<int>(m_sprite->getGlobalBounds().width), m_topLeftCorner.y + static_cast<int>(m_sprite->getGlobalBounds().height) };
}

SpriteRo::~SpriteRo()
{

}

void SpriteRo::render()
{
	m_bottomRightCorner = { m_topLeftCorner.x + static_cast<int>(m_sprite->getGlobalBounds().width), m_topLeftCorner.y + static_cast<int>(m_sprite->getGlobalBounds().height) };
	m_sprite->render({ static_cast<float>(m_topLeftCorner.x), static_cast<float>(m_topLeftCorner.y) });
}
