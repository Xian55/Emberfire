#include "stdafx.h"
#include "PopupPrompt.h"
#include "SpriteRo.h"
#include "ContentMgr.h"
#include "Sprite.h"
#include "Application.h"
#include "Button.h"
#include "TextBoxRo.h"
#include "PromptBox.h"
#include "ConfirmMessageBox.h"
#include "Sprite.h"

#include <assert.h>

PopupPrompt::PopupPrompt(RenderObject& owner, const int id, const string& titleStr, const Codes code, const bool numbersOnly /*= false*/, const bool allowZero /*= false*/) :
	RenderObjectHolder(&owner, id),
	m_code(code)
{
	setMultiInput(true);
	sContentMgr->playSound(SfxId::BoxMessage);

	// Background
	ASSERT(m_bgSprite = sContentMgr->spawnSprite("error_popup.png"));

	addRenderObject(attachObj(m_bg = make_shared<SpriteRo>(*this, m_bgSprite, Interface::Background), sf::Vector2i{}));

	// Title
	auto title = make_shared<TextBoxRo>(*this, Interface::Title, FontId::Palatino, 425, 15);
	title->setString(titleStr);
	addRenderObject(attachObj(title, sf::Vector2i(67, 35)));
	
	// Accept
	auto acceptButton = make_shared<Button>(*this, "yes_button", Interface::AcceptButton, sf::Vector2i{}, SfKeyEvent(sf::Keyboard::Return));
	acceptButton->setPlayButtonOnKeyboardPress(true);
	addRenderObject(attachObj(acceptButton, sf::Vector2i(165, 125)));

	// Decline
	auto declineButton = make_shared<Button>(*this, "no_button", Interface::DeclineButton, sf::Vector2i{}, SfKeyEvent(sf::Keyboard::Escape));
	declineButton->setPlayButtonOnKeyboardPress(true);
	addRenderObject(attachObj(declineButton, sf::Vector2i(285, 125)));

	// Prompt
	m_promptBox = make_shared<PromptBox>(*this, Interface::Prompt, FontId::Helvetica, nullptr, sf::Vector2i{}, 350, sf::Color::White);
	m_promptBox->setPromptCharacterSize(15);
	m_promptBox->setPromptCharacterSize(12);
	m_promptBox->setNumbersOnly(numbersOnly);
	m_promptBox->setAllowZero(allowZero);
	addRenderObject(attachObj(m_promptBox, sf::Vector2i(70, 72)));
}

PopupPrompt::~PopupPrompt()
{

}

void PopupPrompt::input()
{
	sApplication->setCurrentPrompt(m_promptBox.get());

	__super::input();
	
	switch (popFirstButtonId())
	{
		case Interface::AcceptButton:
		{
			if (getContent().empty())
			{
				sApplication->spawnPopup("Please enter something first.", ConfirmMessageBox::ConfirmBox_Ok, ConfirmMessageBox::ConfirmCode_Null);
			}
			else
			{
				m_accepted = true;

				if (auto roh = dynamic_cast<RenderObjectHolder*>(getOwner()))
					roh->setPopupPromptDone(getId());
			}

			break;
		}
		case Interface::DeclineButton:
		{
			if (auto roh = dynamic_cast<RenderObjectHolder*>(getOwner()))
				roh->destroyObjectById(getId());
			break;
		}
	}
}

void PopupPrompt::render()
{
	m_topLeftCorner.x = static_cast<int>((sApplication->sWf() / 2.0f) - (m_bgSprite->getGlobalBounds().width / 2.0f));
	m_topLeftCorner.y = static_cast<int>((sApplication->sHf() / 2.0f) - (m_bgSprite->getGlobalBounds().height / 2.0f));

	__super::render();
}

const string& PopupPrompt::getContent() const
{
	auto promptBox = dynamic_pointer_cast<PromptBox>(getRenderObject(Interface::Prompt));
	ASSERT(promptBox != nullptr);
	return promptBox->getContent();
}
