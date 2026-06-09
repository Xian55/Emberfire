#include "stdafx.h"
#include "QuestOffer.h"
#include "TextBoxRo.h"
#include "ContentMgr.h"
#include "QuestRewards.h"
#include "Button.h"
#include "World.h"
#include "Connector.h"
#include "DraggableNode.h"
#include "Application.h"
#include "ClientObject.h"
#include "Text.h"
#include "ScrollBar.h"
#include "SpriteRo.h"

#include "..\SqlConnector\QueryResult.h"
#include "..\Shared\GamePacketClient.h"
#include "..\StringHelpers.h"

QuestOffer::QuestOffer(World& owner, const int id) :
	DialogPanel(owner, id, "quest.png", sf::Vector2i(365, 20))
{

}

QuestOffer::~QuestOffer()
{

}

void QuestOffer::input()
{
	__super::input();
	
	switch (popFirstButtonId())
	{
		case Interface::AcceptButton:
		{
			acceptQuest();
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

void QuestOffer::render()
{
	__super::render();

	if (m_descScrBarBg != nullptr)
		m_descScrBarBg->attemptRender();

	if (m_descScrollBar != nullptr)
		m_descScrollBar->attemptRender();
}

void QuestOffer::acceptQuest()
{
	GP_Client_AcceptQuest packet;
	packet.m_questGiverGuid = m_questGiverGuid;
	packet.m_questId = m_questId;
	sConnector->sendPacket(packet.build(StlBuffer{}));
	m_world.closePanel(World::Interface(getId()));
}

void QuestOffer::setForQuest(const int questId)
{
	m_questId = questId;
	resetPosition();

	auto& qt = sContentMgr->db("quest_template");

	string titleStr = qt.data(questId, "name");
	string descriptionStr = qt.data(questId, "description");
	string objectivesStr = qt.data(questId, "objective");

	if (titleStr.empty())
		titleStr = "Unknown";

	if (descriptionStr.empty())
		descriptionStr = "Error, description not found.";

	if (objectivesStr.empty())
		objectivesStr = "Error, objectives not found.";

	Util::trimStr(objectivesStr);
	Util::trimStr(titleStr);
	Util::trimStr(descriptionStr);
	
	m_world.formatQuestText(objectivesStr, questId);
	m_world.formatQuestText(descriptionStr, questId);

	// Capture for the Lua view.
	m_titleStr = titleStr;
	m_descriptionStr = descriptionStr;
	m_objectivesStr = objectivesStr;

	const int maxDescriptionLines = 8;

	// Title
	auto title = make_shared<TextBoxRo>(*this, Interface::QuestTitle, FontId::Palatino, 300, 16, TextBox::AlignLeft, false, 2.f);
	title->setAnchor(&getTopLeftCornerRef());
	title->setOffset({ 78, 95 });
	title->setString(titleStr, sf::Color(189, 166, 145, 255), sf::Color(0, 0, 0, 50));
	title->getTextRef().getTextRef().setShadowOffset(1);
	addRenderObject(title);

	// Description
	m_description = make_shared<TextBoxRo>(*this, Interface::QuestDescription, FontId::Palatino, 330, 14, TextBox::AlignLeft, false, 2.f);
	m_description->setAnchor(&getTopLeftCornerRef());
	m_description->setOffset({ 55, 135 });
	m_description->setString(descriptionStr, sf::Color(142, 119, 98, 255), sf::Color(0, 0, 0, 50), maxDescriptionLines, true);
	m_description->getTextRef().getTextRef().setShadowOffset(1);
	addRenderObject(m_description);

	int objectivesBannerOffset = 0;

	if (m_description->getTextRef().getNumLines() >= maxDescriptionLines)
		objectivesBannerOffset = -10;

	if (m_description->getScrollMax() > 0)
	{
		// Given to the binds list
		m_descScrollBar = make_shared<ScrollBar>(*this, "scrollbar_thin_blank", "scrollbar_thin_blank", ScrollBar::ScrollTopDown, "scrollbar_thin", Interface::DescriptionScroll);
		attachObj(m_descScrollBar->getScrollUpButton(), sf::Vector2i(30, 135));
		attachObj(m_descScrollBar->getScrollDownButton(), sf::Vector2i(30, 230));
		m_descScrollBar->getScrollUpButton()->setHidden(true);
		m_descScrollBar->getScrollDownButton()->setHidden(true);
		m_descScrollBar->setMaxOffset(m_description->getScrollMax());
		attachObj(m_descScrollBar, {});
		attachObj(m_descScrBarBg = make_shared<SpriteRo>(*this, sContentMgr->spawnSprite("questoffer_scroll_bg.png"), Interface::DescriptionScrollBg), { 34, 147 });
	}
	else
	{
		m_descScrollBar = nullptr;
		m_descScrBarBg = nullptr;
	}

	// Objectives banner
	auto objectiveBanner = make_shared<TextBoxRo>(*this, Interface::QuestObjectivesBanner, FontId::Palatino, 330, 15, TextBox::AlignLeft, false, 2.f);
	objectiveBanner->setAnchor(&getTopLeftCornerRef());
	objectiveBanner->setOffset({ 55, 135 + m_description->getTextRef().getHeight() + 20 + objectivesBannerOffset });
	objectiveBanner->setString("Objectives", sf::Color(189, 166, 145, 255), sf::Color(0, 0, 0, 50));
	objectiveBanner->getTextRef().getTextRef().setShadowOffset(1);
	addRenderObject(objectiveBanner);

	// Objectives
	auto objectives = make_shared<TextBoxRo>(*this, Interface::QuestObjectives, FontId::Palatino, 330, 12, TextBox::AlignLeft, false, 2.f);
	objectives->setAnchor(&getTopLeftCornerRef());
	objectives->setOffset({ 55, 135 + m_description->getTextRef().getHeight() + 45 + objectivesBannerOffset  });
	objectives->setString(objectivesStr, sf::Color(142, 119, 98, 255), sf::Color(0, 0, 0, 50));
	objectives->getTextRef().getTextRef().setShadowOffset(1);
	addRenderObject(objectives);

	// Rewards
	auto rewards = make_shared<QuestRewards>(*this, Interface::QuestRewardsObject, questId);
	rewards->setAnchor(&getTopLeftCornerRef());
	rewards->setOffset({ 95, 420 });
	addRenderObject(rewards);

	// Accept
	auto acceptButton = make_shared<Button>(*this, "generic_highlight", Interface::AcceptButton);
	acceptButton->setOffset({ 145, 500 });
	acceptButton->setAnchor(&getTopLeftCornerRef());
	addRenderObject(acceptButton);
}