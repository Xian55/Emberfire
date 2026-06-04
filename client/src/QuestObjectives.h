#pragma once

#include "WorldChild.h"

class Sprite;
class TextBoxRo;

class QuestObjectives : public WorldChild
{
	enum Interface
	{
		BackgroundRo,
		BannerRo,

		QuestTitleStart = 1000,
		QuestObjectivesStart = 2000
	};

	public:
		QuestObjectives(World& owner, const int id);
		virtual ~QuestObjectives();

		void recalc();

	protected:
		void input() final;
		void render() final;

		int m_cachInvPanelBottom{0};

		map<int, int> m_legend;
		shared_ptr<Sprite> m_bgSpr;
};

