#pragma once

#include "DialogPanel.h"

class TextBoxRo;
class ScrollBar;
class SpriteRo;

class QuestOffer : public DialogPanel
{
	public:
		enum Interface
		{
			QuestTitle = 1,
			QuestDescription,
			QuestObjectivesBanner,
			QuestObjectives,
			QuestRewardsObject,
			AcceptButton,
			DescriptionScroll,
			DescriptionScrollBg
		};

	public:
		QuestOffer(World& owner, const int id);
		virtual ~QuestOffer();

		void setQuestGiverGuid(const int guid) { m_questGiverGuid = guid; }
		void setForQuest(const int questId);

		// ---- Lua view: the formatted dialog content + the accept action (shared with the C++ button) ----
		int  questId() const { return m_questId; }
		const string& titleStr() const { return m_titleStr; }
		const string& descriptionStr() const { return m_descriptionStr; }
		const string& objectivesStr() const { return m_objectivesStr; }
		void acceptQuest();

	protected:
		virtual void notifyClose() {}

	private:
		void input() final;
		void render() final;

		int m_questId{0};
		int m_questGiverGuid{0};

		string m_titleStr;        // formatted content captured for the Lua view
		string m_descriptionStr;
		string m_objectivesStr;

		shared_ptr<SpriteRo> m_descScrBarBg;
		shared_ptr<ScrollBar> m_descScrollBar;
		shared_ptr<TextBoxRo> m_description;
};

