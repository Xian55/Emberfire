#pragma once

#include "DialogPanel.h"

class DraggableNode;
class QuestRewards;
class ScrollBar;
class TextBoxRo;
class SpriteRo;

class QuestComplete : public DialogPanel
{
	public:
		enum Interface
		{
			QuestTitle = 1,
			QuestDescription,
			QuestObjectivesBanner,
			QuestObjectives,
			QuestRewardsObject,
			CompleteButton,
			DescriptionScroll,
			DescriptionScrollBg
		};

	public:
		QuestComplete(World& owner, const int id);
		virtual ~QuestComplete();

		void setQuestGiverGuid(const int guid) { m_questGiverGuid = guid; }
		void setForQuest(const int questId);

		// ---- Lua view: dialog content + the complete action. choiceIdx = the reward CHOICE SLOT (0-based,
		//      the quest_template rew_choiceN slot, NOT the displayed index), -1 = no choice. Validates the
		//      "must pick a reward" rule exactly like the C++ button. ----
		int  questId() const { return m_questId; }
		const string& titleStr() const { return m_titleStr; }
		const string& descriptionStr() const { return m_descriptionStr; }
		bool needsChoice() const;
		void completeWithChoice(const int choiceIdx);

	protected:
		virtual void notifyClose() {}
		virtual void input() override;
		virtual void render() override;

	private:
		int m_questId{0};
		int m_questGiverGuid{0};

		string m_titleStr;        // formatted content captured for the Lua view
		string m_descriptionStr;

		shared_ptr<SpriteRo> m_descScrBarBg;
		shared_ptr<ScrollBar> m_descScrollBar;
		shared_ptr<TextBoxRo> m_description;
		shared_ptr<QuestRewards> m_rewards;
};

