#pragma once

#include "RenderObject.h"

class TextBox;
class Sprite;

class TimedMessageBox : public RenderObject
{
	public:
		TimedMessageBox(RenderObject& owner, const int id, const float numSeconds, const string& msg);
		virtual ~TimedMessageBox();

	private:
		void input() final;
		void render() final;

		float m_timer;

		string m_msg;

		shared_ptr<TextBox> m_text;
		shared_ptr<Sprite> m_spriteBackdrop;
};

