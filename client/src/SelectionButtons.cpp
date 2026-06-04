#include "stdafx.h"
#include "SelectionButtons.h"
#include "Button.h"

SelectionButtons::SelectionButtons(RenderObjectHolder& owner, const int id) :
	RenderObjectHolder(&owner, id),
	m_change(false)
{
	setMultiInput(true);
}

SelectionButtons::~SelectionButtons()
{

}

void SelectionButtons::input()
{
	__super::input();

	if (auto result = dynamic_pointer_cast<Button>(popFirstButton()))
		processChoice(result);
}

void SelectionButtons::render()
{
	__super::render();
}

void SelectionButtons::clearChosen()
{
	if (m_choice != nullptr)
		m_choice->setHighlight(false);

	m_choice = nullptr;
}

void SelectionButtons::processChoice(shared_ptr<Button> b)
{
	if (m_choice == b)
		return;

	if (m_choice != nullptr)
		m_choice->setHighlight(false);

	m_choice = b;
	m_choice->setHighlight(true);
	m_change = true;
}

int SelectionButtons::getChosen() const
{
	if (m_choice == nullptr)
		return -1;

	return m_choice->getId();
}

bool SelectionButtons::popChange()
{
	if (m_change)
	{
		m_change = false;
		return true;
	}
	
	return false;
}

void SelectionButtons::setChosen(const int id)
{
	if (auto result = dynamic_pointer_cast<Button>(getRenderObject(id)))
		processChoice(result);
}
