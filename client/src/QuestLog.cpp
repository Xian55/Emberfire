#include "stdafx.h"
#include "QuestLog.h"
#include "ScrollBar.h"
#include "TextLines.h"
#include "Button.h"
#include "Application.h"
#include "ContentMgr.h"
#include "TextBoxRo.h"
#include "ConfirmMessageBox.h"
#include "GameIcon.h"
#include "World.h"
#include "QuestRewards.h"
#include "SelectionButtons.h"
#include "Sprite.h"
#include "Connector.h"
#include "QuestObjectives.h"
#include "MapQuester.h"
#include "ClientPlayer.h"

#include "..\Shared\NpcDefines.h"
#include "..\Shared\PlayerDefines.h"
#include "..\Shared\GamePacketClient.h"
#include "..\SqlConnector\QueryResult.h"
#include "..\StringHelpers.h"
#include "..\Math.h"

#include <filesystem>

QuestLog::QuestLog(World& owner, const int id) :
	WorldPanel(owner, id, "questlog_empty.png", sf::Vector2i(572, 23)),
	m_selectedQuestIdx(-1),
	m_refreshCurrent(false),
	m_lastScrollOffset(-1)
{
	m_doneCheck = sContentMgr->spawnSprite("quest_done_map.png");
	m_trackedCheck = sContentMgr->spawnSprite("questlog_green_check.png");
}

QuestLog::~QuestLog()
{

}

void QuestLog::input()
{
	__super::input();

	switch (popFirstButtonId())
	{
		case Interface::AbandonQuestButton:
		{
			sApplication->spawnPopup("Abandon quest?", ConfirmMessageBox::ConfirmBox_YesNo, ConfirmMessageBox::ConfirmCode_AbandonQuest);
			break;
		}
		case Interface::TrackQuestButton:
		{
			if (isQuestTracked(getSelectedEntry()))
			{
				// Untrack
				untrackQuest(getSelectedEntry());
			}
			else
			{
				// Track
				if (!trackQuest(getSelectedEntry()))
					world().error(PlayerDefines::WorldError::MaximumQuestsTracked);
				else
					saveTrackedQuests();
			}

			break;
		}
	}

	if (auto confirmBox = sApplication->popConfirmBox({ ConfirmMessageBox::ConfirmCode_AbandonQuest }))
	{
		if (confirmBox->getResult() == ConfirmMessageBox::ConfirmBox_Yes)
		{
			GP_Client_AbandonQuest packet;
			packet.m_questId = getSelectedEntry();
			sConnector->sendPacket(packet.build(StlBuffer{}));
		}
	}

	if (m_questScroll != nullptr && m_questList != nullptr)
	{
		// Maintain the chosen square with the scroll offset 
		if (m_lastScrollOffset != m_questScroll->getScrollOffset())
			refreshChosen();

		m_lastScrollOffset = m_questScroll->getScrollOffset();

		if (m_questList->popChange())
		{
			int clickedLine = m_questList->getChosen() + m_questScroll->getScrollOffset();

			if (clickedLine >= 0 && static_cast<size_t>(clickedLine) < m_questEntries.size())
			{
				selectQuestIndex(clickedLine);
				sContentMgr->playSound("window_target_open_a.ogg");
			}
		}
	}
}

void QuestLog::render()
{
	// It's possible that we'll be told to refresh objective text count more than once during the same frame.
	// We only want to recreate the text objects once once, and they do need to be recreated when we want them to change.
	if (m_refreshCurrent)
	{
		m_refreshCurrent = false;
		selectQuestIndex(m_selectedQuestIdx);
	}

	__super::render();
		
	// Quest entries will always be positive
	if (const int selectedQuest = getSelectedEntry())
	{

	}

	if (m_questScroll != nullptr && m_questList != nullptr)
	{
		// Quest done green check mark
		for (int id = QuestTitle_1, counter = 0; id <= QuestTitle_10; ++id, ++counter)
		{
			int idx = m_questScroll->getScrollOffset() + counter;

			if (idx < int(m_questEntries.size()))
			{
				if (m_questEntries[idx].done)
					m_doneCheck->render(sf::Vector2f(getTitleTopLeft(counter) + getTopLeftCornerRef() + sf::Vector2i(140, -2)));
				else if (m_questEntries[idx].tracked)
					m_trackedCheck->render(sf::Vector2f(getTitleTopLeft(counter) + getTopLeftCornerRef() + sf::Vector2i(140, -2)));
			}
		}
	}
}

void QuestLog::refreshQuestListObj()
{	
	if (m_questList == nullptr)
		return;

	// Hide not needed selection buttons
	for (int i = 0; i < NumQuestListVisible; ++i)
	{
		if (auto obj = m_questList->getRenderObject(i))
			obj->setHidden(i >= int(m_questEntries.size()));
	}
}

void QuestLog::refreshChosen()
{
	int newChosen = m_selectedQuestIdx - m_questScroll->getScrollOffset();
			
	// It's off screen
	if (newChosen < 0 || newChosen >= NumQuestListVisible)
		m_questList->clearChosen();
	else
		m_questList->setChosen(newChosen);
		
	refreshTitles();
	m_questList->popChange();
}

void QuestLog::refreshTitles()
{
	for (int id = QuestTitle_1, counter = 0; id <= QuestTitle_10; ++id, ++counter)
	{
		int idx = m_questScroll->getScrollOffset() + counter;

		if (auto obj = dynamic_pointer_cast<TextBoxRo>(getRenderObject(id)))
		{
			if (idx >= int(m_questEntries.size()))
			{
				obj->setHidden(true);
			}
			else 
			{
				obj->setHidden(false);
				obj->setString(m_questEntries[idx].title, sf::Color(164, 144, 126, 255), sf::Color(0, 0, 0, 50), 1);
			}
		}
	}
}

void QuestLog::questHelper_RevealDoneQuests()
{
	vector<uint16_t> doneQuests;

	for (auto& itr : m_questEntries)
	{
		if (itr.done)
			doneQuests.push_back(itr.id);
	}

	dynamic_pointer_cast<MapQuester>(world().getRenderObject(World::MapQuesterPanel))->setDoneQuests(doneQuests);
}

void QuestLog::questHelper_QueueReveal()
{
	set<int> npcsToKill;
	set<int> gosToUse;
	set<int> itemsToFind;

	auto consider = [&](const map<int, map<int, Objective>>& mapobj, set<int>& output)
	{
		for (auto& itr : mapobj)
		{
			if (!isQuestTracked(itr.first))
				continue;

			for (auto& subItr : itr.second)
			{
				if (subItr.second.numDone < subItr.second.maxCount)
					output.insert(subItr.second.id);
			}
		}
	};
	
	consider(m_objectivesItem, itemsToFind);
	consider(m_objectivesNpc, npcsToKill);
	consider(m_objectivesGameObject, gosToUse);
	//consider(spells?)
	
	dynamic_pointer_cast<MapQuester>(world().getRenderObject(World::MapQuesterPanel))->queueRevealKillNpcs(npcsToKill);
	dynamic_pointer_cast<MapQuester>(world().getRenderObject(World::MapQuesterPanel))->queueRevealGameObjs(gosToUse);
	dynamic_pointer_cast<MapQuester>(world().getRenderObject(World::MapQuesterPanel))->queueRevealItems_AppendNpcGameObj(itemsToFind);
}

void QuestLog::init()
{
	alterBackground("questlog.png");

	// Given to the quest list
	m_questScroll = make_shared<ScrollBar>(*this, "scrollbar_thin_blank", "scrollbar_thin_blank", ScrollBar::ScrollTopDown, "scrollbar_thin", Interface::QuestScroll);
	m_questScroll->getScrollUpButton()->setOffset({ 236, 72 });
	m_questScroll->getScrollUpButton()->setAnchor(&getTopLeftCornerRef());
	m_questScroll->getScrollDownButton()->setOffset({ 236, 510 });
	m_questScroll->getScrollDownButton()->setAnchor(&getTopLeftCornerRef());
	m_questScroll->getScrollSquare()->setOffset({ 236, 510 });
	attachObj(m_questScroll, { 236, 72 });
	addRenderObject(m_questScroll);

	// Quest list
	addRenderObject(m_questList = make_shared<SelectionButtons>(*this, Interface::QuestList));

	for (int i = 0; i < NumQuestListVisible; ++i)
	{
		auto button = make_shared<Button>(*m_questList, "questlist", i);
		attachObj(button, sf::Vector2i(20, 77 + (i * 49)));
		m_questList->addRenderObject(button);
	}

	m_questList->setChosen(0);
	m_questList->popChange();
		
	for (int i = QuestTitle_1, counter = 0; i <= QuestTitle_10; ++i, ++counter)
	{
		auto label = make_shared<TextBoxRo>(*this, i, "Palatino Linotype Regular.ttf", 155, 15, TextBox::AlignLeft, false, 2.f);
		attachObj(label, getTitleTopLeft(counter));
		addRenderObject(label);
	}
}

sf::Vector2i QuestLog::getTitleTopLeft(const int counter)
{
	return sf::Vector2i(50, 90 + (counter * 49));
}

bool QuestLog::isQuestTracked(const int questId) const
{
	return m_trackedQuests.find(questId) != m_trackedQuests.end();
}

bool QuestLog::untrackQuest(const int questId)
{
	int idx = getQuestIndex(questId);

	if (idx != -1)
		m_questEntries[m_selectedQuestIdx].tracked = false;

	m_trackedQuests.erase(questId);
	world().queueObjectivesRecalc();
	questHelper_QueueReveal();
	return true;
}

bool QuestLog::trackQuest(const int questId)
{
	vector<QuestData> trackedQuests;
	getTrackedQuests(trackedQuests);

	if (trackedQuests.size() >= MaxTrackedQuests)
		return false;

	int idx = getQuestIndex(questId);

	if (idx != -1)
		m_questEntries[idx].tracked = true;

	m_trackedQuests.insert(questId);	
	world().queueObjectivesRecalc();
	questHelper_QueueReveal();
	return true;
}
		
void QuestLog::saveTrackedQuests()
{
	if (world().myself() == nullptr)
		return;

	StlBuffer data;
		
	data << uint16_t(m_trackedQuests.size());

	for (auto& questId : m_trackedQuests)
		data << uint16_t(questId);

	if (!filesystem::is_directory("cache") || !filesystem::exists("cache")) 
		filesystem::create_directory("cache");

	data.writeFile(sApplication->getCacheFolderPath() + "trackedquests_" + world().myself()->getName());
}
		
void QuestLog::loadTrackedQuests()
{
	if (world().myself() == nullptr)
		return;
	
	StlBuffer data;
	
	if (data.readFile(sApplication->getCacheFolderPath() + "trackedquests_" + world().myself()->getName()))
	{
		uint16_t num = 0;
		data >> num;

		for (uint16_t i = 0; i < num; ++i)
		{
			uint16_t questId = 0;
			data >> questId;
			trackQuest(questId);
		}
	}
}

void QuestLog::getTrackedQuests(vector<QuestData>& output) const
{
	output.clear();

	for (auto& itr : m_questEntries)
	{
		if (itr.tracked)
			output.push_back(itr);
	}
}

void QuestLog::setQuestDone(const int entry, const bool value)
{
	for (auto& itr : m_questEntries)
	{
		if (itr.id == entry)
			itr.done = value;
	}
}

void QuestLog::addQuest(const int entry)
{
	// No duplicates
	if (getQuestIndex(entry) != -1)
		return;

	if (m_questList == nullptr)
		init();

	auto& qt = sContentMgr->db("quest_template");
	string titleStr = qt.data(entry, "name");

	if (titleStr.empty())
		titleStr = "Unknown";
		
	QuestData data;
	data.id = entry;
	data.title = titleStr;
	data.tracked = m_trackedQuests.find(entry) != m_trackedQuests.end();
	m_questEntries.push_back(data);

	// Populate quest objectives for it	
	for (int i = 0; i < 4; ++i)
	{
		// Only up to 4 objectives max. Not big but not a big game, either.
		if (const int reqCount = atoi(qt.data(entry, "req_count" + to_string(i + 1)).c_str()))
		{
			if (const int id = atoi(qt.data(entry, "req_item" + to_string(i + 1)).c_str()))
				m_objectivesItem[entry][i] = { id, 0, reqCount };
			else if (const int id = atoi(qt.data(entry, "req_go" + to_string(i + 1)).c_str()))
				m_objectivesGameObject[entry][i] = { id, 0, reqCount };
			else if (const int id = atoi(qt.data(entry, "req_spell" + to_string(i + 1)).c_str()))
				m_objectivesSpell[entry][i] = { id, 0, reqCount };
			else if (const int id = considerNewReqNpc(entry, atoi(qt.data(entry, "req_npc" + to_string(i + 1)).c_str())))
				m_objectivesNpc[entry][i] = { id, 0, reqCount };
		}
	}
	
	if (m_selectedQuestIdx == -1)
		selectTopQuest();
	
	// No point letting them scroll if they haven't exceeded max
	if (m_questEntries.size() > NumQuestListVisible)
	{
		m_questScroll->setHidden(false);
		m_questScroll->setMaxOffset(m_questEntries.size() - NumQuestListVisible);
	}
	else
	{
		m_questScroll->setHidden(true);
	}

	refreshTitles();
	refreshQuestListObj();
}

int QuestLog::considerNewReqNpc(const int questId, const int npcEntry)
{
	auto& db = sContentMgr->db("npc_template");
	int npcflags = atoi(db.data(npcEntry, "npc_flags").c_str());

	for (auto& itr : m_questEntries)
	{
		if (itr.id == questId && itr.done)
			return npcEntry;
	}

	if (Util::maskHas(npcflags, NpcDefines::Flags::SpendExpCredit))
		world().exclaimHint(World::Hint::SpendExp);

	return npcEntry;
}

void QuestLog::selectTopQuest()
{
	if (m_questList == nullptr)
		return;

	// These two will sync up next update loop
	m_questList->setChosen(0);
	selectQuestIndex(0);
}

void QuestLog::removeQuest(const int entry)
{
	for (size_t i = 0; i < m_questEntries.size(); ++i)
	{
		if (entry == m_questEntries[i].id)
		{
			m_questEntries.erase(m_questEntries.begin() + i);

			if (m_questEntries.empty())
				m_selectedQuestIdx = -1;
			else if (m_selectedQuestIdx == i)
				selectQuestIndex(0);

			if (i == m_questList->getChosen())
				selectTopQuest();
		
			break;
		}
	}

	auto eraseContainer = [&](map<int, map<int, Objective>>& c)
	{
		auto objectivesItr = c.find(entry);

		if (objectivesItr != c.end())
			c.erase(objectivesItr);
	};

	// Erase objective data
	eraseContainer(m_objectivesItem);
	eraseContainer(m_objectivesNpc);
	eraseContainer(m_objectivesSpell);
	eraseContainer(m_objectivesGameObject);	

	// Reset if this made us empty
	if (m_questEntries.empty())
		clear();

	refreshQuestListObj();
	refreshTitles();
}

void QuestLog::clear()
{
	if (m_questList != nullptr)
		m_questList->clear();

	m_questEntries.clear();
		
	if (m_questScroll != nullptr)
	{
		refreshTitles();
		refreshQuestListObj();
	}

	m_objectivesItem.clear();
	m_objectivesNpc.clear();
	m_objectivesSpell.clear();

	m_selectedQuestIdx = -1;
	m_questList = nullptr;

	destroyObjectById(Interface::QuestList);
	destroyObjectById(Interface::QuestTitle);
	destroyObjectById(Interface::QuestObjective);
	destroyObjectById(Interface::AbandonQuestButton);
	destroyObjectById(Interface::QuestRewardsObject);
	destroyObjectById(Interface::QuestDescription);
	destroyObjectById(Interface::QuestObjectivesBanner);

	alterBackground("questlog_empty.png");
}

string QuestLog::formObjectiveString_Npc(const Objective& objective, const bool completeTxt)
{
	string monsterName;
	string result;

	auto& db = sContentMgr->db("npc_template");

	if (objective.id == -1)
		monsterName = "Monster";
	else
		monsterName = db.data(objective.id, "name");

	int npcflags = atoi(db.data(objective.id, "npc_flags").c_str());

	if (Util::maskHas(npcflags, NpcDefines::Flags::SpendExpCredit))
		result = monsterName;
	else if (Util::maskHas(npcflags, NpcDefines::Flags::LevelToCredit))
		result = "Reach " + monsterName;
	else if (Util::maskHas(npcflags, NpcDefines::Flags::TalkCredit))
		result = to_string(objective.numDone) + "/" + to_string(objective.maxCount) + " " + monsterName + " spoken to";
	else
		result = to_string(objective.numDone) + "/" + to_string(objective.maxCount) + " " + monsterName + " slain";

	if (objective.numDone >= objective.maxCount && completeTxt)
		result += " (Complete)";

	return result;
}

string QuestLog::formObjectiveString_Item(const Objective& objective, const bool completeTxt)
{
	string result = sContentMgr->db("item_template").data(objective.id, "name") + ": " +
		to_string(objective.numDone) + "/" + to_string(objective.maxCount);

	if (objective.numDone >= objective.maxCount && completeTxt)
		result += " (Complete)";

	return result;
}

string QuestLog::formObjectiveString_Spell(const Objective& objective, const bool completeTxt)
{
	string result = sContentMgr->db("spell_template").data(objective.id, "name") +
		to_string(objective.numDone) + "/" + to_string(objective.maxCount);

	if (objective.numDone >= objective.maxCount && completeTxt)
		result += " (Complete)";

	return result;
}

string QuestLog::formObjectiveString_GameObject(const Objective& objective, const bool completeTxt)
{
	string result = "Open " + sContentMgr->db("gameobject_template").data(objective.id, "name") +
		to_string(objective.numDone) + "/" + to_string(objective.maxCount);

	if (objective.numDone >= objective.maxCount && completeTxt)
		result += " (Complete)";

	return result;
}

bool QuestLog::hasTallyObjectives(const int questId)
{
	return m_objectivesItem[questId].size() > 0 || m_objectivesNpc[questId].size() > 0 || m_objectivesGameObject[questId].size() > 0 || m_objectivesSpell[questId].size() > 0;
}

string QuestLog::getQuestObjectiveString(const int questId)
{
	string result;

	// Consider individual objectives, such as "0/10 Monsters"
	for (auto& itr : m_objectivesItem[questId])
		result += "\n" + formObjectiveString_Item(itr.second, false);

	for (auto& itr : m_objectivesNpc[questId])
		result += "\n" + formObjectiveString_Npc(itr.second, false);

	for (auto& itr : m_objectivesGameObject[questId])
		result += "\n" + formObjectiveString_GameObject(itr.second, false);

	for (auto& itr : m_objectivesSpell[questId])
		result += "\n" + formObjectiveString_Spell(itr.second, false);

	if (result.empty())
	{
		auto& qt = sContentMgr->db("quest_template");
		result = qt.data(questId, "objective");

		if (result.empty())
			result = "There was an error finding objective text.";
	}
	else
	{
		result.erase(result.begin());
	}

	return result;
}

string QuestLog::getRequiredItemsString(const int questId)
{
	auto& qt = sContentMgr->db("quest_template");

	string str;

	// Consider individual objectives, such as "0/10 Monsters"
	for (auto& itr : m_objectivesItem[questId])
		str += "\n" + formObjectiveString_Item(itr.second);

	if (!str.empty())
		str.erase(str.begin());

	return str;
}

void QuestLog::selectQuestIndex(int idx)
{
	if (m_questEntries.empty())
	{
		clear();
		return;
	}

	if (idx >= static_cast<int>(m_questEntries.size()))
		idx = static_cast<int>(m_questEntries.size()) - 1;

	ASSERT(static_cast<size_t>(idx) < m_questEntries.size());
	const int entry = m_questEntries[idx].id;
	m_selectedQuestIdx = idx;
	
	auto& qt = sContentMgr->db("quest_template");
	
	string descriptionStr = qt.data(entry, "description");
	string titleStr = qt.data(entry, "name");

	if (titleStr.empty())
		titleStr = "Unknown";

	if (descriptionStr.empty())
		descriptionStr = "Error, description not found.";

	string objectiveStr = qt.data(entry, "objective");

	Util::trimStr(objectiveStr);
	Util::trimStr(titleStr);
	Util::trimStr(descriptionStr);

	if (objectiveStr.empty())
		objectiveStr = "There was an error finding objective text.";

	// Consider individual objectives, such as "0/10 Monsters"
	for (auto& itr : m_objectivesItem[entry])
		objectiveStr += "\n\t\t" + formObjectiveString_Item(itr.second);

	for (auto& itr : m_objectivesNpc[entry])
		objectiveStr += "\n\t\t" + formObjectiveString_Npc(itr.second);

	for (auto& itr : m_objectivesGameObject[entry])
		objectiveStr += "\n\t\t" + formObjectiveString_GameObject(itr.second);

	for (auto& itr : m_objectivesSpell[entry])
		objectiveStr += "\n\t\t" + formObjectiveString_Spell(itr.second);
		
	m_world.formatQuestText(descriptionStr, entry);
	m_world.formatQuestText(objectiveStr, entry);
	
	// Title
	auto title = make_shared<TextBoxRo>(*this, Interface::QuestTitle, "Palatino Linotype Regular.ttf", 300, 16, TextBox::AlignLeft, false, 2.f);
	title->setAnchor(&getTopLeftCornerRef());
	title->setOffset({ 300, 95 });
	title->setString(titleStr, sf::Color(189, 166, 145, 255), sf::Color(0, 0, 0, 50));
	title->getTextRef().getTextRef().setShadowOffset(1);
	addRenderObject(title);

	// Description
	auto description = make_shared<TextBoxRo>(*this, Interface::QuestDescription, "Palatino Linotype Regular.ttf", 330, 12, TextBox::AlignLeft, false, 2.f);
	description->setAnchor(&getTopLeftCornerRef());
	description->setOffset({ 277, 135 });
	description->setString(descriptionStr, sf::Color(139, 116, 95, 255), sf::Color(0, 0, 0, 50), 6);
	description->getTextRef().getTextRef().setShadowOffset(1);
	addRenderObject(description);

	// Objectives banner
	auto objectiveBanner = make_shared<TextBoxRo>(*this, Interface::QuestObjectivesBanner, "Palatino Linotype Regular.ttf", 330, 16, TextBox::AlignLeft, false, 2.f);
	objectiveBanner->setAnchor(&getTopLeftCornerRef());
	objectiveBanner->setOffset({ 277, 135 + description->getTextRef().getHeight() + 20 });
	objectiveBanner->setString("Objectives", sf::Color(189, 166, 145, 255), sf::Color(0, 0, 0, 50));
	objectiveBanner->getTextRef().getTextRef().setShadowOffset(1);
	addRenderObject(objectiveBanner);

	// Objectives
	auto objective = make_shared<TextBoxRo>(*this, Interface::QuestObjective, "Palatino Linotype Regular.ttf", 330, 14, TextBox::AlignLeft, false, 2.f);
	objective->setAnchor(&getTopLeftCornerRef());
	objective->setOffset({ 277, 135 + description->getTextRef().getHeight() + 45 });
	objective->setString(objectiveStr, sf::Color(139, 116, 95, 255), sf::Color(0, 0, 0, 50));
	objective->getTextRef().getTextRef().setShadowOffset(1);
	addRenderObject(objective);

	// Rewards
	auto rewards = make_shared<QuestRewards>(*this, Interface::QuestRewardsObject, entry);
	rewards->setAnchor(&getTopLeftCornerRef());
	rewards->setOffset({ 317, 440 });
	addRenderObject(rewards);

	// Track quest button
	auto trackButton = make_shared<Button>(*this, "generic_highlight_medium", Interface::TrackQuestButton);
	trackButton->setOffset({ 266, 532 });
	trackButton->setAnchor(&getTopLeftCornerRef());
	addRenderObject(trackButton);

	// Share quest button
	auto shareButton = make_shared<Button>(*this, "generic_highlight_medium", Interface::ShareQuestButton);
	shareButton->setOffset({ 357, 532 });
	shareButton->setAnchor(&getTopLeftCornerRef());
	addRenderObject(shareButton);

	// Abandon quest button
	auto abandonButton = make_shared<Button>(*this, "generic_highlight_medium", Interface::AbandonQuestButton);
	abandonButton->setOffset({ 508, 532 });
	abandonButton->setAnchor(&getTopLeftCornerRef());
	addRenderObject(abandonButton);
}

void QuestLog::selectQuestEntry(const int entry)
{
	for (size_t i = 0; i < m_questEntries.size(); ++i)
	{
		if (m_questEntries[i].id == entry)
		{
			selectQuestIndex(i);
			sContentMgr->playSound("window_target_open_a.ogg");
			refreshChosen();
			return;
		}
	}
}

void QuestLog::notifyGainedItem(const int questId, const int itemId, const int newValue, const bool silent)
{
	vector<Objective const*> changes;
	notifyObjectiveValue(questId, m_objectivesItem, itemId, newValue, changes, silent);
	
	bool finishedAtleastOne = false;

	for (auto& itr : changes)
	{
		if (itr->numDone >= itr->maxCount)
		{
			finishedAtleastOne = true;
			break;
		}
	}

	if (finishedAtleastOne)
		questHelper_QueueReveal();

	if (silent)
		return;

	for (auto& itr : changes)
		m_world.pushScrollingMessage(formObjectiveString_Item(*itr), sf::Color(Colors::ObjectiveTextColor));
}

void QuestLog::notifyKilledNpc(const int questId, const int npc, const int newValue, const bool silent)
{
	vector<Objective const*> changes;
	notifyObjectiveValue(questId, m_objectivesNpc, npc, newValue, changes, silent);
	
	bool finishedAtleastOne = false;

	for (auto& itr : changes)
	{
		if (itr->numDone >= itr->maxCount)
		{
			finishedAtleastOne = true;
			break;
		}
	}

	if (finishedAtleastOne)
		questHelper_QueueReveal();

	if (silent)
		return;

	for (auto& itr : changes)
		m_world.pushScrollingMessage(formObjectiveString_Npc(*itr), sf::Color(Colors::ObjectiveTextColor));
}

void QuestLog::notifyCastedSpell(const int questId, const int spell, const int newValue, const bool silent)
{
	vector<Objective const*> changes;
	notifyObjectiveValue(questId, m_objectivesSpell, spell, newValue, changes, silent);
	
	if (silent)
		return;

	for (auto& itr : changes)
		m_world.pushScrollingMessage(formObjectiveString_Spell(*itr), sf::Color(Colors::ObjectiveTextColor));
}

void QuestLog::notifyUsedObject(const int questId, const int objId, const int newValue, const bool silent)
{
	vector<Objective const*> changes;
	notifyObjectiveValue(questId, m_objectivesGameObject, objId, newValue, changes, silent);
	
	if (silent)
		return;

	for (auto& itr : changes)
		m_world.pushScrollingMessage(formObjectiveString_GameObject(*itr), sf::Color(Colors::ObjectiveTextColor));
}

void QuestLog::notifyObjectiveValue(const int questId, map<int, map<int, Objective>>& objective, const int id, const int newValue, vector<Objective const*>& changes, const bool silent)
{
	// questId, objectiveId, count
	auto itr = objective.find(questId);

	if (itr == objective.end())
		return;

	for (auto& subItr : itr->second)
	{
		auto& data = subItr.second;

		if (data.id == id)
		{
			if (newValue > data.numDone)
			{
				if (!silent)
					sContentMgr->playSound("alert_increased_a.ogg");

				changes.push_back(&data);
			}

			data.numDone = newValue;
			data.numDone = min(data.numDone, data.maxCount);
		}
	}

	if (!changes.empty())
	{	
		if (getSelectedEntry() == questId)
			m_refreshCurrent = true;
	}
}

int QuestLog::getSelectedEntry() const
{
	if (m_selectedQuestIdx == -1)
		return 0;

	ASSERT(static_cast<size_t>(m_selectedQuestIdx) < m_questEntries.size());
	return m_questEntries[m_selectedQuestIdx].id;
}

int QuestLog::getQuestIndex(const int entry) const
{
	for (size_t i = 0; i < m_questEntries.size(); ++i)
	{
		if (m_questEntries[i].id == entry)
			return i;
	}

	return -1;
}
