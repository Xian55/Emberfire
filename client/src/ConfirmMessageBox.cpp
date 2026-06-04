#include "stdafx.h"
#include "ConfirmMessageBox.h"
#include "ContentMgr.h"
#include "Application.h"
#include "TextBox.h"
#include "Button.h"
#include "Sprite.h"

#include <assert.h>

ConfirmMessageBox::ConfirmMessageBox(RenderObject& owner, const int id, const int code, const string& msg, const ConfirmBoxType type) :
	RenderObject(&owner, id),
	m_spriteBackdrop(sContentMgr->spawnSprite("error_popup.png")),
	m_result(ConfirmBox_Waiting),
	m_code((ConfirmBoxCodes)code),
	m_msg(msg),
	m_type(type)
{
	m_text = make_unique<TextBox>(sContentMgr->getFont(FontId::Palatino), 16);
	m_text->setColor(sf::Color(139, 116, 95, 255));

	m_acceptButton = make_unique<Button>(*this, "yes_button", ConfirmBox_Yes);

	if (m_type == ConfirmBox_YesNo)
		m_declineButton = make_unique<Button>(*this, "no_button", ConfirmBox_No);
	
	sContentMgr->playSound(SfxId::LoginPopupOpen);

	ASSERT(m_text != nullptr);
	ASSERT(m_spriteBackdrop != nullptr);
	ASSERT(dynamic_cast<RenderObjectHolder*>(getOwner()) != nullptr);
}

ConfirmMessageBox::~ConfirmMessageBox()
{

}

void ConfirmMessageBox::input()
{
	m_acceptButton->attemptInput();

	if (m_acceptButton->popActivated())
		m_result = ConfirmBox_Yes;

	if (m_declineButton != nullptr)
	{
		m_declineButton->attemptInput();

		if (m_declineButton->popActivated())
			m_result = ConfirmBox_No;
	}

	if (m_result != ConfirmBox_Waiting)
	{
		if (RenderObjectHolder* obj = dynamic_cast<RenderObjectHolder*>(getOwner()))
			obj->setConfirmBoxDone(getId());
		else
			ASSERT(0);
	}
}

void ConfirmMessageBox::render()
{
	// Update positions
	m_topLeftCorner.x = static_cast<int>((sApplication->sWf() / 2.0f) - (m_spriteBackdrop->getGlobalBounds().width / 2.0f));
	m_topLeftCorner.y = static_cast<int>((sApplication->sHf() / 2.0f) - (m_spriteBackdrop->getGlobalBounds().height / 2.0f));
	
	m_text->setData(m_topLeftCorner.x + 25, m_topLeftCorner.y + 60, m_msg, 510, TextBox::AlignCenterBounds, true);

	if (m_type == ConfirmBox_YesNo)
	{
		m_acceptButton->setPos({ m_topLeftCorner.x + 165, m_topLeftCorner.y + 125 });
		m_declineButton->setPos({ m_topLeftCorner.x + 285, m_topLeftCorner.y + 125 });
	}
	else
	{
		m_acceptButton->setPos({ m_topLeftCorner.x + 220, m_topLeftCorner.y + 125 });
	}

	// Start rendering
	m_spriteBackdrop->render({ static_cast<float>(m_topLeftCorner.x), static_cast<float>(m_topLeftCorner.y) });
	m_acceptButton->attemptRender();

	if (m_declineButton != nullptr)
		m_declineButton->attemptRender();

	m_text->draw();
}