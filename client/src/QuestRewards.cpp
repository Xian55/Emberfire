#include "stdafx.h"
#include "QuestRewards.h"
#include "WorldPanel.h"
#include "TextBoxRo.h"
#include "ContentMgr.h"
#include "ItemIcon.h"
#include "SelectionButtons.h"
#include "Text.h"
#include "WorldChild.h"
#include "World.h"
#include "ClientPlayer.h"

#include "..\StringHelpers.h"
#include "..\SqlConnector\QueryResult.h"

QuestRewards::QuestRewards(WorldPanel& owner, const int id, const int questId) :
	RenderObjectHolder(&owner, id)
{
	setMultiInput(true);

	auto& qt = sContentMgr->db("quest_template");

	string rewardsStr;

	// Basic rewards
	const string rewMoney = Util::formatMoneyCommas(atoi(qt.data(questId, "rew_money").c_str()));
	const string rewXp = qt.data(questId, "rew_xp");

	int lines = 0;

	if (!rewXp.empty())
	{
		++lines;
		rewardsStr.append(rewXp +  "xp\n");
	}

	if (!rewMoney.empty())
	{
		++lines;
		rewardsStr.append("Gold: " + rewMoney + "\n");
	}

	if (rewardsStr.empty())
	{
		++lines;
		rewardsStr = "No basic reward.";
	}

	struct QItem
	{
		int slotIdx, itemId, stackCount;
	};

	vector<QItem> choices;
	vector<QItem> mandatorys;

	for (int i = 0; i < 4; ++i)
	{
		string rew_choice = "rew_choice" + to_string(i + 1) + "_item";
		string rew_choice_count = "rew_choice" + to_string(i + 1) + "_count";

		if (int rewardItemEntry = atoi(qt.data(questId, rew_choice).c_str()))
			choices.push_back({ i, rewardItemEntry, atoi(qt.data(questId, rew_choice_count).c_str()) });
	}

	for (int i = 0; i < 4; ++i)
	{
		string rew_item = "rew_item" + to_string(i + 1);
		string rew_item_count = "rew_item" + to_string(i + 1) + "_count";

		if (int rewardItemEntry = atoi(qt.data(questId, rew_item).c_str()))
			mandatorys.push_back({ i, rewardItemEntry, atoi(qt.data(questId, rew_item_count).c_str()) });
	}

	int startX = 130;
	int startY = 5;
	int xIncr = 42;

	// Center our X a bit
	if (max(choices.size(), mandatorys.size()) >= 3)
		startX -= 20;

	string buttonName = "gameicon40";

	// Adjust Y if we have 2 rows
	if (!choices.empty() && !mandatorys.empty())
		startY -= 15;
	
	m_choices = make_shared<SelectionButtons>(*this, Interface::ChoicesObj);
	m_hasChoices = choices.size() > 0;
	
	auto& it = sContentMgr->db("item_template");

	// Choices
	for (int i = 0, count = 0; i < int(choices.size()); ++i)
	{
		if (canNeverUseItemEntry(choices[i].itemId))
			continue;

		ItemDefines::ItemDefinition def;
		def.m_itemId = uint16_t(choices[i].itemId);
		def.m_durability = atoi(it.data(def.m_itemId, "durability").c_str());

		auto gi = make_shared<ItemIcon>(*this, Interface::ChoiceIcon1 + choices[i].slotIdx, buttonName);
		gi->setIgnoreLevelErrorForIcon(true);
		gi->setOffset({ sf::Vector2i(startX + (xIncr * count), startY) });
		gi->setAnchor(&getTopLeftCornerRef());
		gi->setTooltipHorizontalAdju(false);
		gi->setItemDef(def);
		gi->setStackCount(choices[i].stackCount);
		m_choices->addRenderObject(gi);
		++count;
	}

	addRenderObject(m_choices);

	int yIdx = 0;

	if (!choices.empty())
	{
		yIdx = 1;
		buttonName = "gameicon32";
		xIncr = 34;
		startY += 2;
	}

	// Rewards
	for (int i = 0, count = 0; i < int(mandatorys.size()); ++i)
	{
		if (canNeverUseItemEntry(mandatorys[i].itemId))
			continue;
		
		ItemDefines::ItemDefinition def;
		def.m_itemId = uint16_t(mandatorys[i].itemId);
		def.m_durability = atoi(it.data(def.m_itemId, "durability").c_str());

		auto gi = make_shared<ItemIcon>(*this, Interface::RewardIcon1 + mandatorys[i].slotIdx, buttonName);
		gi->setIgnoreLevelErrorForIcon(true);
		gi->setOffset({ sf::Vector2i(startX + (xIncr * count), startY + (42 * yIdx)) });
		gi->setAnchor(&getTopLeftCornerRef());
		gi->setEnableHighlight(false);
		gi->setIgnoreSpellbookNotifyForIcon(true);
		gi->setItemDef(def);
		gi->setStackCount(mandatorys[i].stackCount);
		addRenderObject(gi);
		++count;

		if (!choices.empty())
			m_hasChoicesAndExtra = true;
	}

	if (m_hasChoicesAndExtra)
	{
		if (lines == 0)
			rewardsStr += "\n\n";
		else if (lines == 1)
			rewardsStr += "\n";

		rewardsStr += "Extra Items: ";
	}

	// Rewards
	auto rewardsText = make_shared<TextBoxRo>(*this, Interface::QuestRewardsText, FontId::Palatino, 225, 14, TextBox::AlignLeft, false, 2.f);
	rewardsText->setAnchor(&getTopLeftCornerRef());
	rewardsText->setOffset({ 0, 5 });
	rewardsText->setString(rewardsStr, sf::Color(139, 116, 95, 255), sf::Color(0, 0, 0, 50));
	rewardsText->getTextRef().getTextRef().setShadowOffset(1);
	addRenderObject(rewardsText);
}

QuestRewards::~QuestRewards()
{

}

void QuestRewards::input() /*final*/
{
	if (!m_chosing)
	{
		m_choices->clearChosen();
	}
	else if (m_choices->popChange())
	{
		sContentMgr->playSound(SfxId::AlertEntry);
	}

	__super::input();
}

void QuestRewards::render() /*final*/
{
	m_choices->getTopLeftCornerRef() = getTopLeftCornerRef();
	__super::render();
}

int QuestRewards::getChoiceIdx() const
{
	int chosenId = m_choices->getChosen();

	if (chosenId >= int(Interface::ChoiceIcon1))
		return chosenId - int(Interface::ChoiceIcon1);

	return -1;
}

bool QuestRewards::canNeverUseItemEntry(const int entry) const
{
	auto& it = sContentMgr->db("item_template");

	if (const int requiredClass = atoi(it.data(entry, "required_class").c_str()))
	{
		if (auto parent = dynamic_cast<WorldChild*>(getOwner()))
		{
			if (parent->world().myself() != nullptr)
				return parent->world().myself()->getClass() != requiredClass;
		}
	}

	return false;
}