#pragma once

#include "RenderObjectHolder.h"

class WorldPanel;
class SelectionButtons;

class QuestRewards : public RenderObjectHolder
{
	public:
		enum Interface
		{
			QuestRewardsText,
			ChoicesObj,

			RewardIcon1 = 1000,
			RewardIcon2,
			RewardIcon3,
			RewardIcon4,
			RewardIcon5,

			ChoiceIcon1 = 2000,
			ChoiceIcon2,
			ChoiceIcon3,
			ChoiceIcon4,
			ChoiceIcon5,
		};

	public:
		QuestRewards(WorldPanel& owner, const int id, const int questId);
		virtual ~QuestRewards();

		void enableChoosing(const bool v) { m_chosing = v; }

		bool isChoosing() const { return m_chosing; }
		bool isChoiceMade() const { return getChoiceIdx() != -1; }
		bool hasChoices() const { return m_hasChoices; }
		bool hasChoicesAndExtra() const { return m_hasChoicesAndExtra; }

		int getChoiceIdx() const;

	private:
		void input() final;
		void render() final;
		
		bool canNeverUseItemEntry(const int entry) const;

		bool m_chosing{false};
		bool m_hasChoices{false};
		bool m_hasChoicesAndExtra{false};

		shared_ptr<SelectionButtons> m_choices;
};

