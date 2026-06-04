#include "stdafx.h"
#include "QuestComplete.h"
#include "TextBoxRo.h"
#include "ContentMgr.h"
#include "QuestRewards.h"
#include "Button.h"
#include "World.h"
#include "Connector.h"
#include "DraggableNode.h"
#include "Application.h"
#include "ClientObject.h"
#include "QuestLog.h"
#include "Text.h"
#include "ScrollBar.h"
#include "SpriteRo.h"

#include "..\SqlConnector\QueryResult.h"
#include "..\Shared\GamePacketClient.h"
#include "..\Shared\PlayerDefines.h"
#include "..\StringHelpers.h"

QuestComplete::QuestComplete(World& owner, const int id) :
	DialogPanel(owner, id, "questcomplete.png", sf::Vector2i(365, 20))
{

}

QuestComplete::~QuestComplete()
{

}

void QuestComplete::input()
{
	__super::input();

	switch (popFirstButtonId())
	{
		case Interface::CompleteButton:
		{
			if (m_rewards->isChoosing() && !m_rewards->isChoiceMade())
			{				
				m_world.error(PlayerDefines::WorldError::RewardNotChosen);
			}
			else
			{
				GP_Client_CompleteQuest packet;
				packet.m_questGiverGuid = m_questGiverGuid;
				packet.m_questId = m_questId;
				packet.m_itemChoiceIdx = m_rewards->getChoiceIdx();
				sConnector->sendPacket(packet.build(StlBuffer{}));
				m_world.closePanel(World::Interface(getId()));
			}

			break;
		}
	}
	
	if (m_descScrollBar != nullptr)
	{
		if (isMousedOver())
			m_descScrollBar->attemptInput();

		m_description->pumpScrollOffset(m_descScrollBar->getScrollOffset());
	}
}

void QuestComplete::render()
{
	__super::render();

	if (m_descScrBarBg != nullptr)
		m_descScrBarBg->attemptRender();

	if (m_descScrollBar != nullptr)
		m_descScrollBar->attemptRender();
}

void QuestComplete::setForQuest(const int questId)
{
	m_questId = questId;
	getTopLeftCornerRef() = { sApplication->sW() / 5, sApplication->sH() / 7 };

	auto& qt = sContentMgr->db("quest_template");

	string titleStr = qt.data(questId, "name");
	string descriptionStr = qt.data(questId, "offer_reward_text");

	if (titleStr.empty())
		titleStr = "Unknown";

	if (descriptionStr.empty())
		descriptionStr = "Error, description not found.";

	Util::trimStr(titleStr);
	Util::trimStr(descriptionStr);

	m_world.formatQuestText(descriptionStr, questId);

	// Title
	auto title = make_shared<TextBoxRo>(*this, Interface::QuestTitle, "Palatino Linotype Regular.ttf", 300, 16, TextBox::AlignLeft, false, 2.f);
	title->setAnchor(&getTopLeftCornerRef());
	title->setOffset({ 78, 95 });
	title->setString(titleStr, sf::Color(189, 166, 145, 255), sf::Color(0, 0, 0, 50));
	title->getTextRef().getTextRef().setShadowOffset(1);
	addRenderObject(title);
	
	int maxDescriptionLines = 11;

	/*if (auto questLog = dynamic_pointer_cast<QuestLog>(world().getRenderObject(World::Interface::QuestLogPanel)))
	{
		string reqItems = questLog->getRequiredItemsString(questId);

		if (!reqItems.empty())
			maxDescriptionLines = 10;
	}*/

	// Description
	m_description = make_shared<TextBoxRo>(*this, Interface::QuestDescription, "Palatino Linotype Regular.ttf", 330, 14, TextBox::AlignLeft, false, 2.f);
	m_description->setAnchor(&getTopLeftCornerRef());
	m_description->setOffset({ 55, 135 });
	m_description->setString(descriptionStr, sf::Color(142, 119, 98, 255), sf::Color(0, 0, 0, 50), maxDescriptionLines, true);
	m_description->getTextRef().getTextRef().setShadowOffset(1);
	addRenderObject(m_description);

	/*if (auto questLog = dynamic_pointer_cast<QuestLog>(world().getRenderObject(World::Interface::QuestLogPanel)))
	{
		string reqItems = questLog->getRequiredItemsString(questId);

		if (!reqItems.empty())
		{
			int bannerY = min(305, 135 + (int)m_description->getTextRef().getTextRef().getGlobalBounds().height + 20);

			// Required Items
			auto objectiveBanner = make_shared<TextBoxRo>(*this, Interface::QuestObjectivesBanner, "Palatino Linotype Regular.ttf", 330, 15, TextBox::AlignLeft, false, 2.f);
			objectiveBanner->setAnchor(&getTopLeftCornerRef());
			objectiveBanner->setOffset({ 55, bannerY });
			objectiveBanner->setString("Required Items", sf::Color(189, 166, 145, 255), sf::Color(0, 0, 0, 50));
			objectiveBanner->getTextRef().getTextRef().setShadowOffset(1);
			addRenderObject(objectiveBanner);

			// Items 5/5
			auto objective = make_shared<TextBoxRo>(*this, Interface::QuestObjectives, "Palatino Linotype Regular.ttf", 300, 12, TextBox::AlignLeft, false, 2.f);
			objective->setAnchor(&getTopLeftCornerRef());
			objective->setOffset({ 75, bannerY + 25  });
			objective->setString(reqItems, sf::Color(139, 116, 95, 255), sf::Color(0, 0, 0, 50));
			objective->getTextRef().getTextRef().setShadowOffset(1);
			addRenderObject(objective);
		}
		else
		{
			destroyObjectById(Interface::QuestObjectivesBanner);
			destroyObjectById(Interface::QuestObjectives);
		}
	}*/

	// Rewards
	m_rewards = make_shared<QuestRewards>(*this, Interface::QuestRewardsObject, questId);
	m_rewards->setAnchor(&getTopLeftCornerRef());
	m_rewards->setOffset({ 95, 420 });
	addRenderObject(m_rewards);

	if (m_rewards->hasChoices())
		m_rewards->enableChoosing(true);

	// Accept
	auto acceptButton = make_shared<Button>(*this, "generic_highlight", Interface::CompleteButton);
	acceptButton->setOffset({ 145, 500 });
	acceptButton->setAnchor(&getTopLeftCornerRef());
	addRenderObject(acceptButton);

	if (m_description->getScrollMax() > 0)
	{
		m_descScrollBar = make_shared<ScrollBar>(*this, "scrollbar_thin_blank", "scrollbar_thin_blank", ScrollBar::ScrollTopDown, "scrollbar_thin", Interface::DescriptionScroll);
		attachObj(m_descScrollBar->getScrollUpButton(), sf::Vector2i(30, 135));
		attachObj(m_descScrollBar->getScrollDownButton(), sf::Vector2i(30, 230));
		m_descScrollBar->getScrollUpButton()->setHidden(true);
		m_descScrollBar->getScrollDownButton()->setHidden(true);
		m_descScrollBar->setMaxOffset(m_description->getScrollMax());
		attachObj(m_descScrBarBg = make_shared<SpriteRo>(*this, sContentMgr->spawnSprite("questoffer_scroll_bg.png"), Interface::DescriptionScrollBg), { 34, 147 });
		attachObj(m_descScrollBar, {});
	}
	else
	{
		m_descScrollBar = nullptr;
		m_descScrBarBg = nullptr;
	}
}