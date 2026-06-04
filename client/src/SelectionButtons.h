#pragma once

#include "RenderObjectHolder.h"

class SelectionButtons : public RenderObjectHolder
{
	public:
		SelectionButtons(RenderObjectHolder& owner, const int id);
		virtual ~SelectionButtons();

		void clearChosen();
		void setChosen(const int id);

		int getChosen() const;

		bool popChange();

	private:
		void input() final;
		void render() final;

		void processChoice(shared_ptr<Button> b);

		bool m_change;
		shared_ptr<Button> m_choice;
};

