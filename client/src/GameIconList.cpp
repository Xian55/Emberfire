#include "stdafx.h"
#include "GameIconList.h"
#include "ContentMgr.h"
#include "Text.h"
#include "Tooltip.h"
#include "Text.h"
#include "TextBox.h"
#include "ContentMgr.h"
#include "Sprite.h"
#include "ItemIcon.h"
#include "SpellIcon.h"

GameIconList::GameIconList(World& world, RenderObjectHolder& owner, const int id, const GameIcon::Type type, const int numVerticalElements, const int graphicalWidth,
			const sf::Vector2i& iconOffsetWithinSlot, const string& slotBg, const int slotHeight, const string& titleFont, const int titleFontSize, 
			const sf::Color& titleColor, const sf::Color& titleOutlineColor, const float titleBorderThickness, const sf::Vector2i& titleOffset) :
	WorldChild(owner, id, world),
	m_graphicalWidth(graphicalWidth),
	m_numVerticalElements(numVerticalElements),
	m_scrollOffset(0),
	m_iconOffsetWithinSlot(iconOffsetWithinSlot),
	m_titleOffset(titleOffset),
	m_descriptionWidth(0),
	m_descriptionMaxLines(0),
	m_descriptionBorderThickness(0),
	m_lowercaseTxt(false)
{
	setMultiInput(true);

	for (int i = 0; i < m_numVerticalElements; ++i)
	{
		shared_ptr<GameIcon> icon;
		
		if (type == GameIcon::Type::Item)
			icon = make_shared<ItemIcon>(*this, i, "gameicon40");
		else
			icon = make_shared<SpellIcon>(*this, i, "gameicon40");

		icon->setAnchor(&getTopLeftCornerRef());
		icon->setOffset({ iconOffsetWithinSlot.x, (slotHeight * i) + iconOffsetWithinSlot.y });
		icon->setHidden(true);
		addRenderObject(icon);

		m_iconBgFades.push_back({ make_shared<Button>(*this, slotBg), icon });

		auto label = make_shared<Text>(sContentMgr->getFont(titleFont));
		label->setCharacterSize(titleFontSize);
		label->setColorIfNot(titleColor, titleOutlineColor);
		label->setOutlineThickness(titleBorderThickness);
		m_labels.push_back(label);
	}
}
		
void GameIconList::setAllowDraggingIcons(const bool v)
{
	for (auto& itr : m_iconBgFades)
		itr.second->setEnableDragActivate(v);	
}
		
void GameIconList::registerDongleTooltip(const string& buttonName, const int iconEntry, shared_ptr<Tooltip> tooltip)
{
	m_dongleTooltips[buttonName][iconEntry] = tooltip;
}

void GameIconList::registerDongle(const string& buttonName)
{
	m_dongles[buttonName].clear();
	m_dongleExceptions.clear();

	for (int i = 0; i < m_numVerticalElements; ++i)
		m_dongles[buttonName].push_back({ make_shared<Button>(*this, buttonName), nullptr });
}

void GameIconList::enableDescriptions(const string& descriptionFont,
	const int descriptionFontSize, const sf::Color& descriptionColor, const sf::Color& descriptionOutlineColor, const float descriptionBorderThickness,
	const sf::Vector2i& descriptionOffset, const int descriptionWidth, const int descriptionMaxLines)
{
	m_descriptionOffset = descriptionOffset;
	m_descriptionColor = descriptionColor;
	m_descriptionOutlineColor = descriptionOutlineColor;
	m_descriptionBorderThickness = descriptionBorderThickness;
	m_descriptionWidth = descriptionWidth;
	m_descriptionMaxLines = descriptionMaxLines;

	m_descriptions.clear();

	for (int i = 0; i < m_numVerticalElements; ++i)
		m_descriptions.push_back(make_shared<TextBox>(sContentMgr->getFont(descriptionFont), descriptionFontSize));
}

bool GameIconList::isAllHidden() const
{
	for (auto& itr : m_entries)
	{
		if (itr.second)
			return false;
	}

	return true;
}

bool GameIconList::upMaxxed() const
{
	const int supposedVal = m_scrollOffset - 1;
	return supposedVal < 0;
}

bool GameIconList::downMaxxed() const
{
	const int supposedVal = m_scrollOffset + 1;
	return supposedVal + m_numVerticalElements > static_cast<int>(m_entries.size());
}

void GameIconList::setScroll(const int val)
{
	if (m_scrollOffset == val)
		return;

	m_scrollOffset = val;

	if (m_scrollOffset + m_numVerticalElements > static_cast<int>(m_entries.size()))
		m_scrollOffset = static_cast<int>(m_entries.size()) - m_numVerticalElements;

	if (m_scrollOffset < 0)
		m_scrollOffset = 0;

	refreshView();
}

void GameIconList::scroll(const int amount)
{
	setScroll(m_scrollOffset + amount);
}

void GameIconList::hideIndex(const int idx)
{
	if (idx > static_cast<int>(m_entries.size()))
		return;

	if (auto obj = getRenderObject(idx - m_scrollOffset))
		obj->setHidden(true);

	m_entries[idx].second = false;
}

pair<shared_ptr<GameIcon>, int> GameIconList::popClickedGameIcon()
{
	// The icon itself
	if (auto clicked = dynamic_pointer_cast<GameIcon>(popFirstButton()))
	{
		int idx = m_scrollOffset + clicked->getId();
		return { clicked, idx };
	}

	// The faded background
	for (auto& itr : m_iconBgFades)
	{
		if (itr.first->popActivated())
		{
			int idx = m_scrollOffset + itr.second->getId();
			return { itr.second, idx };
		}
	}

	return { nullptr, 0 };
}
		
pair<shared_ptr<GameIcon>, int> GameIconList::popClickedDongle(const string& dongleName)
{
	auto mapitr = m_dongles.find(dongleName);

	if (mapitr == m_dongles.end())
		return {};

	// Click on a dongle and get the gameicon associated with it
	for (auto& itr : mapitr->second)
	{
		if (itr.first == nullptr || itr.second == nullptr)
			continue;
		
		if (itr.first->popActivated())
		{
			int idx = m_scrollOffset + itr.second->getId();
			return { itr.second, idx };
		}
	}
	
	return {};
}

void GameIconList::refreshView()
{
	m_needDescriptionRefresh = true;

	decltype(m_entries) entriesToSee;

	// Pick icons that we see and assign them to the render object
	// m_entries has properties about each one: entry, stack, and whether or not its hidden
	for (int i = m_scrollOffset; i < static_cast<int>(m_entries.size()); ++i)
		entriesToSee.push_back(m_entries[i]);

	for (int i = 0; i < m_numVerticalElements; ++i)
	{
		if (auto gameIcon = dynamic_pointer_cast<GameIcon>(getRenderObject(i)))
		{
			if (i < static_cast<int>(entriesToSee.size()))
			{
				if (auto itemEntry = dynamic_pointer_cast<ItemEntry>(entriesToSee[i].first))
				{
					gameIcon->setStackCount(itemEntry->stack);

					if (itemEntry->def.m_itemId != 0)
					{
						if (auto itemIcon = dynamic_pointer_cast<ItemIcon>(gameIcon))
							itemIcon->setItemDef(itemEntry->def);
					}
					else
					{		
						gameIcon->changeEntry(entriesToSee[i].first->id);
					}
				}

				if (auto spellEntry = dynamic_pointer_cast<SpellEntry>(entriesToSee[i].first))
				{
					gameIcon->setLevel(spellEntry->lvl);					
					gameIcon->setBasePoints(spellEntry->bpoints);
					gameIcon->changeEntry(entriesToSee[i].first->id);
				}

				gameIcon->setHidden(!entriesToSee[i].second);

				if (i < static_cast<int>(m_labels.size()) && gameIcon->getType() == GameIcon::Type::Item)
					m_labels[i]->setColorIfNot(ItemIcon::itemColor(gameIcon->getQuality()), sf::Color(0, 0, 0, 30));
			}
			else
			{
				gameIcon->setHidden(true);
			}
		}
	}
}

void GameIconList::addItemIcon(const ItemDefines::ItemDefinition def, const int stack)
{
	shared_ptr<ItemEntry> ptr = make_shared<ItemEntry>();
	ptr->id = def.m_itemId;
	ptr->def = def;
	ptr->stack = stack;

	m_entries.push_back({ ptr, true });

	refreshView();
}

bool GameIconList::itemEntryAt(int index, ItemDefines::ItemDefinition& def, int& stack) const
{
	if (index < 0 || index >= static_cast<int>(m_entries.size()))
		return false;

	auto item = dynamic_pointer_cast<ItemEntry>(m_entries[index].first);
	if (!item)
		return false;

	def   = item->def;
	stack = item->stack;
	return true;
}

void GameIconList::addSpellIcon(const int entry, const int lvl, const vector<pair<int16_t, int16_t>>& bpoints)
{
	shared_ptr<SpellEntry> ptr = make_shared<SpellEntry>();
	ptr->id = entry;
	ptr->lvl = lvl;
	ptr->bpoints = bpoints;

	m_entries.push_back({ ptr, true });

	refreshView();
}

void GameIconList::updateSpellIcon(const int entry, const int lvl, const vector<pair<int16_t, int16_t>>& bpoints)
{
	for (auto& itr : m_entries)
	{
		if (itr.first->id != entry)
			continue;

		if (auto spellEntry = dynamic_pointer_cast<SpellEntry>(itr.first))
		{
			spellEntry->lvl = lvl;
			spellEntry->bpoints = bpoints;
		}
	}
	
	refreshView();
	refreshTooltips();
}

void GameIconList::refreshTooltips()
{
	for (int i = 0; i < m_numVerticalElements; ++i)
	{
		if (auto gameIcon = dynamic_pointer_cast<GameIcon>(getRenderObject(i)))
			gameIcon->refreshTooltip();
	}
}

void GameIconList::clearEntries()
{
	m_scrollOffset = 0;
	m_entries.clear();
	m_dongles.clear();
	m_dongleExceptions.clear();
	refreshView();
}

void GameIconList::input() /*final*/
{
	__super::input();

	for (auto& itr : m_iconBgFades)
		itr.first->attemptInput();

	for (auto& itr : m_dongles)
	{
		for (auto& subItr : itr.second)
		{
			if (subItr.second == nullptr)
				continue;

			subItr.first->attemptInput();
		}
	}
}

void GameIconList::render() /*final*/
{
	if (!m_dongles.empty())
	{
		for (auto& itr : m_dongles)
		{
			for (auto& subItr : itr.second)
				subItr.second = nullptr;
		}
	}

	for (int i = 0; i < m_numVerticalElements && i < static_cast<int>(m_labels.size()); ++i)
	{
		if (auto gameIcon = dynamic_pointer_cast<GameIcon>(getRenderObject(i)))
		{
			if (gameIcon->isHidden() || gameIcon->getEntry() == 0)
				continue;

			if (!m_darkenedEntries.empty())
				gameIcon->setDarken(m_darkenedEntries.find(gameIcon->getEntry()) != m_darkenedEntries.end());
			else
				gameIcon->setDarken(false);

			gameIcon->setTooltipOffset({ m_iconBgFades[i].first->mouseableWidth() - gameIcon->mouseableWidth(), 0 });

			m_iconBgFades[i].first->updateTopLeftCorner(sf::Vector2i(gameIcon->getTopLeftCornerRef().x - m_iconOffsetWithinSlot.x, gameIcon->getTopLeftCornerRef().y - m_iconOffsetWithinSlot.y));
			m_iconBgFades[i].first->setTooltip(gameIcon->getTooltip());
			m_iconBgFades[i].first->attemptRender();
			
			if (m_iconBgFades[i].first->MouseableNode::isMousedOver(true))
				gameIcon->popTooltipRefresh();
			
			const auto& newPos = gameIcon->getTopLeftCornerRef();
			auto label = m_labels[i].get();

			if (m_needDescriptionRefresh)
			{
				string title = gameIcon->deduceTitle();

				if (m_lowercaseTxt)
					transform(title.begin(), title.end(), title.begin(), ::tolower);
				
				label->setStringMaxWidth(title, m_graphicalWidth, false, false, "", true);
			}

			label->draw(newPos.x + m_titleOffset.x, newPos.y + m_titleOffset.y);

			if (!m_descriptions.empty())
			{
				if (m_needDescriptionRefresh)
				{
					// deduceDescription is slow for spells
					m_descriptions[i]->setData(newPos.x + m_descriptionOffset.x, newPos.y + m_descriptionOffset.y, gameIcon->deduceDescription(), m_descriptionWidth,
						TextBox::AlignLeft, false, m_descriptionBorderThickness, m_descriptionOutlineColor, m_descriptionMaxLines);
				}
				else
				{
					m_descriptions[i]->moveTo(newPos + m_descriptionOffset);
				}
				
				m_descriptions[i]->setColor(m_descriptionColor, m_descriptionOutlineColor);
				m_descriptions[i]->draw();
			}

			for (auto& dongleMap : m_dongles)
			{
				// Skip if bad index or if this entry is purposefully excluded
				if (i < int(dongleMap.second.size()) && m_dongleExceptions[dongleMap.first].find(gameIcon->getEntry()) == m_dongleExceptions[dongleMap.first].end())
					dongleMap.second[i].second = gameIcon;
			}
		}
	}

	m_needDescriptionRefresh = false;

	__super::render();


	bool renderedttthisframe = false;

	static bool didlastframe = false;

	// Always on top
	for (auto& itr : m_dongles)
	{
		for (auto& subItr : itr.second)
		{
			if (subItr.second == nullptr)
				continue;

			const auto& offset = m_dongleOffset[itr.first];
			subItr.first->getTopLeftCornerRef() = subItr.second->getTopLeftCornerRef() + offset;
			subItr.first->attemptRender();

			if (subItr.first->MouseableNode::isMousedOver())
			{
				if (auto& tooltip = m_dongleTooltips[itr.first][subItr.second->getEntry()])
				{
					tooltip->moveTo(subItr.first->getTopLeftCornerRef() + tooltip->getOffset());
					tooltip->draw();
					renderedttthisframe = true;
				}
			}
		}
	}

	didlastframe = renderedttthisframe;
}