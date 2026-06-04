#include "stdafx.h"
#include "TooltipGlyph.h"
#include "Sprite.h"
#include "ContentMgr.h"
#include "Text.h"
#include "ItemIcon.h"

#include "..\SqlConnector\QueryResult.h"

TooltipGlyph::TooltipGlyph(const Types type, const int gem1, const int gem2, const string& gemFont, const int fontSize) : 
	m_type(type)
{
	switch (m_type)
	{
		case TooltipGlyph::Types::Divider:
		{
			m_glyphDivider = sContentMgr->spawnSprite("tooltip_divder.png");
			break;
		}
		case TooltipGlyph::Gems_x1:
		case TooltipGlyph::Gems_x2:
		{
			auto font = sContentMgr->getFont(gemFont);
			auto& sv = sContentMgr->db("item_gems");

			m_gem1_label = make_shared<Text>(font);
			m_gem2_label = make_shared<Text>(font);
			m_glyphGemEmpty = sContentMgr->spawnSprite("tooltip_gem_slot.png");			

			auto assign = [&](Text& txt, const int gemId, shared_ptr<Sprite>& icon)
			{
				txt.setCharacterSize(fontSize);

				if (gemId != 0)
				{
					txt.setOriginalString(sv.data(gemId, "name"));
					txt.setOriginalColor(ItemIcon::itemColor(static_cast<ItemDefines::Quality>(atoi(sv.data(gemId, "quality").c_str()))));
					icon = sContentMgr->spawnSprite(sv.data(gemId, "icon"));
				}
				else
				{
					txt.setOriginalString("Empty Slot");
					txt.setOriginalColor(sf::Color(128, 128, 128));
				}
			};
			
			assign(*m_gem1_label.get(), gem1, m_glyphGemSlot1);
			assign(*m_gem2_label.get(), gem2, m_glyphGemSlot2);
			break;
		}
	}
}

int TooltipGlyph::getHeight() const
{
	switch (m_type)
	{
		case TooltipGlyph::Types::Divider:
			return 10;
		case TooltipGlyph::Gems_x1:
		case TooltipGlyph::Gems_x2:
			return 25;
	}

	return 1;
}

void TooltipGlyph::draw(const sf::Vector2i pos, const int width) const
{
	switch (m_type)
	{
		case TooltipGlyph::Types::Divider:
		{
			sf::Vector2i topLeft(pos.x, pos.y + 7);
			m_glyphDivider->renderStretch(sf::Vector2f(topLeft), sf::Vector2f(topLeft + sf::Vector2i{ width, 1 }));
			break;
		}
		case TooltipGlyph::Gems_x1:
		{
			// Empty always rendered, it's filled in if there's a gem
			sf::Vector2i topLeft(pos.x, pos.y + 4);
			m_glyphGemEmpty->renderStretch(sf::Vector2f(topLeft), sf::Vector2f(topLeft + sf::Vector2i{ 20, 20 }));
			m_gem1_label->draw(topLeft.x + 30, topLeft.y + ( m_gem1_label->getCharacterSize() / 2) - 5);

			if (m_glyphGemSlot1 != nullptr)
				m_glyphGemSlot1->renderStretch(sf::Vector2f(sf::Vector2i{ topLeft.x + 1, topLeft.y + 1 }), sf::Vector2f(topLeft + sf::Vector2i{ 19, 19 }));

			break;
		}
		case TooltipGlyph::Gems_x2:
		{
			sf::Vector2 topLeft(pos.x, pos.y + 4);
			m_glyphGemEmpty->renderStretch(sf::Vector2f(topLeft), sf::Vector2f(topLeft + sf::Vector2i{ 20, 20 }));
			m_gem1_label->draw(topLeft.x + 30, topLeft.y + ( m_gem1_label->getCharacterSize() / 2) - 5);
			
			if (m_glyphGemSlot1 != nullptr)
				m_glyphGemSlot1->renderStretch(sf::Vector2f(sf::Vector2i{ topLeft.x + 1, topLeft.y + 1 }), sf::Vector2f(topLeft + sf::Vector2i{ 19, 19 }));

			topLeft = sf::Vector2i(pos.x + (width / 2), pos.y + 4);
			m_glyphGemEmpty->renderStretch(sf::Vector2f(topLeft), sf::Vector2f(topLeft + sf::Vector2i{ 20, 20 }));
			m_gem2_label->draw(topLeft.x + 30, topLeft.y + ( m_gem2_label->getCharacterSize() / 2) - 5);

			if (m_glyphGemSlot2 != nullptr)
				m_glyphGemSlot2->renderStretch(sf::Vector2f(sf::Vector2i{ topLeft.x + 1, topLeft.y + 1 }), sf::Vector2f(topLeft + sf::Vector2i{ 19, 19 }));
				
			break;
		}
	}
}
