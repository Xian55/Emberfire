#pragma once

#include "Button.h"

class TickBox : public Button
{
	public:
		TickBox(RenderObject& owner, const int id, string bscriptUntick, string bscriptTick, 
			const sf::Vector2i& position = sf::Vector2i(), const bool ticked = false, SfKeyEvent activateKey = SfKeyEvent{});
		virtual ~TickBox();

		void input() override;
		void setTicked(const bool ticked);

		bool ticked() const { return m_ticked; }

	private:
		bool m_ticked;
		string m_scriptUntickName;
		string m_scriptTickName;
};

