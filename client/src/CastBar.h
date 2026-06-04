#pragma once

#include "RenderObject.h"

class Sprite;
class ClientUnit;
class SpellIcon;
class Text;

class CastBar : public RenderObject
{
	public:
		CastBar(RenderObject& owner, const int id, const sf::Vector2i& pos = sf::Vector2i{});
		virtual ~CastBar();
		
		void setUnit(shared_ptr<ClientUnit> ptr);

	protected:
		void input() final;
		void render() final;

	private:
		shared_ptr<Sprite> m_sprite;
		shared_ptr<Sprite> m_bar;

		shared_ptr<Text> m_timerTxt;
		shared_ptr<Text> m_spellNameTxt;	
		
		shared_ptr<ClientUnit> m_unit;
		shared_ptr<SpellIcon> m_icon;
};