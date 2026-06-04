#pragma once

#include "RenderObject.h"

class Sprite;

// Just a sprite that we want to be able to add into a RenderObjectHolder
// Please don't try to expand this with more children
class SpriteRo : public RenderObject
{
	public:
		SpriteRo(RenderObject& owner, shared_ptr<Sprite> spr, const int id, const sf::Vector2i topLeftCorner = sf::Vector2i());
		virtual ~SpriteRo();

		void changeSprite(shared_ptr<Sprite> spr) { m_sprite = spr; }

		auto getRaw() const { return m_sprite; }

	private:
		void input() final {}
		void render() final;

		shared_ptr<Sprite> m_sprite;
};

