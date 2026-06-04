#pragma once

#include "WorldChild.h"

#include "..\Geo2d.h"

class Sprite;
class TextBox;

class LevelupNotify : public WorldChild
{
	public:
		LevelupNotify(World& owner, const int id);
		virtual ~LevelupNotify();
		
		void onLevelTo(const int level);

	private:
		void input() final;
		void render() final;

		shared_ptr<Sprite> m_frame;
		shared_ptr<TextBox> m_title;

		float m_alpha;
		float m_solidTimerSeconds;
		float m_introAlpha;

		string m_txt;
};