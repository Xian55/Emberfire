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

	protected:
		virtual void notifyClose() {}
		virtual void input() override;
		virtual void render() override;

	private:
		int m_questId{0};
		int m_questGiverGuid{0};
		
		shared_ptr<SpriteRo> m_descScrBarBg;
		shared_ptr<ScrollBar> m_descScrollBar;
		shared_ptr<TextBoxRo> m_description;
		shared_ptr<QuestRewards> m_rewards;
};

