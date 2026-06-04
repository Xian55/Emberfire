#pragma once

#include "WorldPanel.h"

class TextBox;
class SelectionButtons;
class ScrollBar;
class Sprite;

class QuestLog : public WorldPanel
{
	struct Objective
	{
		int id;
		int numDone;
		int maxCount;
	};

	public:
		enum Interface
		{
			QuestTitle,
			QuestObjective,
			TrackQuestButton,
			ShareQuestButton,
			AbandonQuestButton,
			QuestRewardsObject,
			QuestDescription,
			QuestObjectivesBanner,			
			QuestList,
			QuestScroll,
			
			QuestTitle_1,
			QuestTitle_2,
			QuestTitle_3,
			QuestTitle_4,
			QuestTitle_5,
			QuestTitle_6,
			QuestTitle_7,
			QuestTitle_8,
			QuestTitle_9,
			QuestTitle_10,
		};

		enum QuestListDef
		{
			NumQuestListVisible = 10,
			MaxTrackedQuests = 5
		};

		enum Colors
		{
			ObjectiveTextColor = 0xe8c716ff
		};

		struct QuestData
		{
			int id{0};
			string title;
			bool done{false};
			bool tracked{true};
		};

	public:
		QuestLog(World& owner, const int id);
		virtual ~QuestLog();

		void addQuest(const int entry);
		void removeQuest(const int entry);
		void clear();
		void selectQuestIndex(int idx);
		void selectQuestEntry(const int questId);
		void setQuestDone(const int entry, const bool value);
		void questHelper_RevealDoneQuests();
		void questHelper_QueueReveal();
		void loadTrackedQuests();
		void saveTrackedQuests();

		// QuestId, Done
		void getTrackedQuests(vector<QuestData>& output) const;

		void notifyGainedItem(const int questId, const int itemId, const int newValue, const bool silent);
		void notifyKilledNpc(const int questId, const int npc, const int newValue, const bool silent);
		void notifyCastedSpell(const int questId, const int spell, const int newValue, const bool silent);
		void notifyUsedObject(const int questId, const int objId, const int newValue, const bool silent);
		
		bool trackQuest(const int questId);
		bool untrackQuest(const int questId);
		bool isQuestTracked(const int questId) const;
		bool hasTallyObjectives(const int questId);

		int getSelectedEntry() const;
		int getQuestIndex(const int entry) const;
		
		string getQuestObjectiveString(const int questId);
		string getRequiredItemsString(const int questId);

	private:
		void input() final;
		void render() final;

		void init();
		void refreshTitles();
		void refreshChosen();
		void refreshQuestListObj();
		void selectTopQuest();
		void notifyObjectiveValue(const int questId, map<int, map<int, Objective>>& objective, const int id, const int num, vector<Objective const*>& changes, const bool silent);
		
		// Returns npcEntry
		int considerNewReqNpc(const int questId, const int npcEntry);

		string formObjectiveString_Npc(const Objective& objective, const bool completeTxt = true);
		string formObjectiveString_Item(const Objective& objective, const bool completeTxt = true);
		string formObjectiveString_Spell(const Objective& objective, const bool completeTxt = true);
		string formObjectiveString_GameObject(const Objective& objective, const bool completeTxt = true);

		sf::Vector2i getTitleTopLeft(const int counter);

		int m_lastScrollOffset;
		int m_selectedQuestIdx;

		bool m_refreshCurrent;
		
		shared_ptr<Sprite> m_trackedCheck;
		shared_ptr<Sprite> m_doneCheck;
		shared_ptr<ScrollBar> m_questScroll;
		shared_ptr<SelectionButtons> m_questList;
		unique_ptr<TextBox> m_objectivesDialog;

		// Needs to be saved seperately for clear() after new quest list from server
		set<int> m_trackedQuests;

		// index, entry
		vector<QuestData> m_questEntries;

		// questId, objectiveId, count
		map<int, map<int, Objective>> m_objectivesItem;
		map<int, map<int, Objective>> m_objectivesNpc;
		map<int, map<int, Objective>> m_objectivesSpell;
		map<int, map<int, Objective>> m_objectivesGameObject;
};

