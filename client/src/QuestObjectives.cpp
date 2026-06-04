#include "stdafx.h"
#include "QuestObjectives.h"
#include "ContentMgr.h"
#include "SpriteRo.h"
#include "Application.h"
#include "Button.h"
#include "Text.h"
#include "Sprite.h"
#include "World.h"
#include "QuestLog.h"
#include "TextBoxRo.h"
#include "Application.h"

#include <assert.h>

QuestObjectives::QuestObjectives(World& owner, const int id) :
	WorldChild(owner, id, owner)
{
	setHidden(true);
}

QuestObjectives::~QuestObjectives()
{

}

void QuestObjectives::input() /*final*/
{	
	__super::input();

	for (int i = Interface::QuestTitleStart; i < Interface::QuestTitleStart + int(m_legend.size()); ++i)
	{
		if (auto ptr = dynamic_pointer_cast<TextBoxRo>(getRenderObject(i)))
		{
			if (ptr->isMousedOver() && !sf::Mouse::isButtonPressed(sf::Mouse::Left))
				ptr->getTextRef().setColor(sf::Color(255, 255, 255, 255), sf::Color(0, 0, 0, 50));
			else if (ptr->getTextRef().getTextRef().getString().find("(Complete)") != std::string::npos)
				ptr->getTextRef().setColor(sf::Color::Green, sf::Color(0, 0, 0, 50));
			else
				ptr->getTextRef().setColor(sf::Color(217, 194, 152, 255), sf::Color(0, 0, 0, 50));

			if (ptr->isMousedOver() && sApplication->mouseUp(sf::Mouse::Left))
			{
				auto itr = m_legend.find(i);

				// Open this quest in the log
				if (itr != m_legend.end())
				{
					world().openPanel(World::Interface::QuestLogPanel);					
					auto questLog = dynamic_pointer_cast<QuestLog>(world().getRenderObject(World::Interface::QuestLogPanel));
					questLog->selectQuestEntry(itr->second);
				}

				sApplication->clearMouseUp();
				break;
			}
		}
	}
}

void QuestObjectives::render() /*final*/
{
	if (m_bgSpr != nullptr)
	{
		int xPos = sApplication->sW() - int(m_bgSpr->getGlobalBounds().width) - 25;
		getTopLeftCornerRef() = { xPos, 340 };

		if (m_cachInvPanelBottom == 0)
			m_cachInvPanelBottom = world().getRenderObject(World::Interface::InventoryPanel)->getBottomRightCornerRef().y;

		if (world().isPanelOpen(World::Interface::InventoryPanel))
		{	
			// Move down if inventory is overlapping us
			if (world().getRenderObject(World::Interface::InventoryPanel)->getBottomRightCornerRef().x - 75 >= getTopLeftCornerRef().x)
				getTopLeftCornerRef().y = m_cachInvPanelBottom + 10;
		}
	}

	__super::render();
}

void QuestObjectives::recalc()
{
	clear();

	auto questLog = dynamic_pointer_cast<QuestLog>(world().getRenderObject(World::Interface::QuestLogPanel));
	ASSERT(questLog != nullptr);

	vector<QuestLog::QuestData> trackedQuests;
	questLog->getTrackedQuests(trackedQuests);

	if (trackedQuests.empty())
	{
		setHidden(true);
		return;
	}

	setHidden(false);

	// Background
	m_bgSpr = sContentMgr->spawnSprite("objectives_bg.png");
	addRenderObject(attachObj(make_shared<SpriteRo>(*this, m_bgSpr, Interface::BackgroundRo), sf::Vector2i{ -30, 0 }));

	// Header/banner "Objectives"
	auto banner = make_shared<TextBoxRo>(*this, Interface::BannerRo, "Ringbearer Medium.ttf", 330, 18, TextBox::AlignLeft, false, 2.f);
	banner->setString("Objectives", sf::Color(217, 194, 152, 255), sf::Color(0, 0, 0, 50));
	banner->getTextRef().getTextRef().setShadowOffset(1);
	addRenderObject(attachObj(banner, { 50, 15 }));
	
	// Objectives
	int y = 45;
	int x = -14;
	int count = 0;

	for (auto& questdata : trackedQuests)
	{	
		string titleStr = questdata.title;

		if (titleStr.empty())
			titleStr = "Unknown";

		if (questdata.done && questLog->hasTallyObjectives(questdata.id))
			titleStr += " (Complete)";

		m_legend[Interface::QuestTitleStart + count] = questdata.id;

		// Quest name
		auto banner = make_shared<TextBoxRo>(*this, Interface::QuestTitleStart + count, "Palatino Linotype Bold.ttf", 365, 18, TextBox::AlignLeft, false, 2.f);
		banner->setAnchor(&getTopLeftCornerRef());
		banner->setString(titleStr, sf::Color(217, 194, 152, 255), sf::Color(0, 0, 0, 50));
		banner->getTextRef().getTextRef().setShadowOffset(1);
		banner->setMouseable(true);
		addRenderObject(attachObj(banner, { x, y }));

		y += banner->getDrawnHeight() + 2;

		// Objectives
		auto objective = make_shared<TextBoxRo>(*this, Interface::QuestObjectivesStart + count, "Palatino Linotype Bold.ttf", 260, 15, TextBox::AlignLeft, false, 2.f);
		objective->setAnchor(&getTopLeftCornerRef());
		objective->setString(questLog->getQuestObjectiveString(questdata.id), sf::Color(182, 173, 139, 255), sf::Color(0, 0, 0, 50));
		objective->getTextRef().getTextRef().setShadowOffset(1);
		addRenderObject(attachObj(objective, { x + 20, y }));

		y += objective->getDrawnHeight();

		++count;
	}
}