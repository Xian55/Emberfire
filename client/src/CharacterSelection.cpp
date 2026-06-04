#include "stdafx.h"
#include "CharacterSelection.h"
#include "ContentMgr.h"
#include "Application.h"
#include "Sprite.h"
#include "World.h"   // World::s_pendingSelf (bridge to spawn controlled player)
#include "..\Shared\Config.h"  // sConfig (AutoLogin)
#include "Button.h"
#include "Game.h"
#include "TextBoxRo.h"
#include "ConfirmMessageBox.h"
#include "Connector.h"
#include "SpriteRo.h"
#include "Text.h"
#include "ContextMenu.h"

#include "..\StringHelpers.h"
#include "..\Shared\PlayerDefines.h"
#include "..\Shared\AccountDefines.h"
#include "..\Shared\GamePacketClient.h"
#include "..\Shared\MutualObject.h"

#include <assert.h>

CharacterSelection::CharacterSelection(RenderObjectHolder& owner, const int id) :
	RenderObjectHolder(&owner, id),
	m_scrollOffset(0)
{
	setMultiInput(true);
	sContentMgr->loopMusic("aion_creation.ogg", false);

	ASSERT(m_background = sContentMgr->spawnSprite("main_bg.jpg"));
	ASSERT(m_backgroundGui = sContentMgr->spawnSprite("main_selection.png"));
	
	// Character list
	initButton({ Interface::CharacterSlot1,Interface::CharacterSlot1_Name,Interface::CharacterSlot1_Subtext,Interface::CharacterSlot1_Portrait }, sf::Vector2i(27, 125));
	initButton({ Interface::CharacterSlot2,Interface::CharacterSlot2_Name,Interface::CharacterSlot2_Subtext,Interface::CharacterSlot2_Portrait }, sf::Vector2i(27, 125 + 90));
	initButton({ Interface::CharacterSlot3,Interface::CharacterSlot3_Name,Interface::CharacterSlot3_Subtext,Interface::CharacterSlot3_Portrait }, sf::Vector2i(27, 125 + 180));

	// < Page 1 >
	m_pageTxt = make_shared<Text>(sContentMgr->getFont("Palatino Linotype Regular.ttf"));
	m_pageTxt->setOriginalColor(sf::Color(136, 117, 102, 255));
	m_pageTxt->setOutlineColor(sf::Color(0, 0, 0, 60));
	m_pageTxt->setOutlineThickness(0.5f);
	m_pageTxt->setCharacterSize(18);
	updatePageTxt();
		 
	addRenderObject(attachObj(m_leftBtn = make_shared<Button>(*this, "left_arrow", Interface::LeftBtn), sf::Vector2i(50, 415)));
	addRenderObject(attachObj(m_rightBtn = make_shared<Button>(*this, "right_arrow", Interface::RightBtn), sf::Vector2i(360, 415)));

	// Create
	addRenderObject(attachObj(make_shared<Button>(*this, "create_character", Interface::CreateCharacter), sf::Vector2i(137, 480)));
	updateButtons();
}

CharacterSelection::~CharacterSelection()
{

}

shared_ptr<Sprite> CharacterSelection::preparePortrait(shared_ptr<Sprite> ptr)
{
	int offset = sContentMgr->getPortraitOffset(*ptr);
	ptr->setTextureRect({0, offset, int(ptr->getTexture()->getSize().x), int(ptr->getTexture()->getSize().x)});
	ptr->setScale(55.f / ptr->getGlobalBounds().width, 55.f / ptr->getGlobalBounds().height);
	return ptr;
}

void CharacterSelection::initButton(const ButtonData& buttondata, const sf::Vector2i& offset)
{
	auto textName = make_shared<TextBoxRo>(*this, buttondata.name, "Ringbearer Medium.ttf", 500, 22, TextBox::AlignLeft, false, 2.3f);
	attachObj(textName, offset + sf::Vector2i(88, 17));
	textName->setMouseable(false);

	auto textSubtext = make_shared<TextBoxRo>(*this, buttondata.subtext, "Cambria Regular.ttf", 500, 15, TextBox::AlignLeft, false, 3.f);
	attachObj(textSubtext, offset + sf::Vector2i(88, 45));
	textSubtext->setMouseable(false);
		
	auto portrait = make_shared<SpriteRo>(*this, preparePortrait(sContentMgr->spawnPlayerPortrait(1, PlayerDefines::Gender::Male)), buttondata.portrait);
	attachObj(portrait, offset + sf::Vector2i(17, 15));
	portrait->setMouseable(false);

	auto button = make_shared<Button>(*this, "char_slot", buttondata.button);
	button->setAllowRightClick(true);
	
	addRenderObject(attachObj(button, offset));
	addRenderObject(textName);
	addRenderObject(textSubtext);
	addRenderObject(portrait);
}

void CharacterSelection::setButton(const ButtonData& buttondata, const Character& chardata)
{
	auto buttonPtr = dynamic_pointer_cast<Button>(getRenderObject(buttondata.button));
	auto namePtr = dynamic_pointer_cast<TextBoxRo>(getRenderObject(buttondata.name));
	auto subtextPtr = dynamic_pointer_cast<TextBoxRo>(getRenderObject(buttondata.subtext));
	auto portrait = dynamic_pointer_cast<SpriteRo>(getRenderObject(buttondata.portrait));

	buttonPtr->setHidden(false);
	namePtr->setHidden(false);
	subtextPtr->setHidden(false);
	portrait->setHidden(false);

	portrait->changeSprite(preparePortrait(sContentMgr->spawnPlayerPortrait(chardata.portrait, chardata.gender)));

	string name = chardata.name;
	string subtext = "Level " + to_string(chardata.level) + " " + PlayerFunctions::className(chardata.classId);

	transform(name.begin(), name.end(), name.begin(), ::tolower);
	transform(subtext.begin(), subtext.end(), subtext.begin(), ::toupper);

	namePtr->setString(name, sf::Color(221, 197, 185, 255), sf::Color(0, 0, 0, 43));
	subtextPtr->setString(subtext, sf::Color(98, 81, 68, 255), sf::Color(0, 0, 0, 43));
}

void CharacterSelection::notifyCtxMenuClicked(const int id, const string& lineClicked) /*final*/
{
	if (id != Game::Ro::CtxMenu_DeleteCharacter)
		return;

	if (lineClicked.find("Delete") != string::npos)
		sApplication->spawnPopup("Are you sure you want to DELETE " + m_chosenCharacter.name + "?", ConfirmMessageBox::ConfirmBox_YesNo, ConfirmMessageBox::ConfirmCode_DeleteCharacter);
}

void CharacterSelection::spawnCtxMenuForCharacter(const int slot)
{
	if (size_t(slot) >= m_characters.size())
		return;

	m_chosenCharacter = m_characters[slot];

	if (auto owner = dynamic_cast<Game*>(getOwner()))
	{
		sContentMgr->playSound("button_click_a.ogg");
		owner->registerContextMenu(Game::Ro::CtxMenu_DeleteCharacter, getId(), sApplication->mousePos(),
			{
				{ "Delete " + m_chosenCharacter.name },
				{ "Cancel" }
			});
	}
}

void CharacterSelection::pickCharacter(const int slot)
{
	if (size_t(slot) >= m_characters.size())
		return;

	m_chosenCharacter = m_characters[slot];
	sApplication->spawnPopup("Connect with " + m_chosenCharacter.name + "?", ConfirmMessageBox::ConfirmBox_YesNo, ConfirmMessageBox::ConfirmCode_SelectCharacter);
}

void CharacterSelection::input()
{
	__super::input();

	switch (popFirstRightClickButtonId())
	{
		case CharacterSlot1:
		{
			spawnCtxMenuForCharacter(m_scrollOffset);
			break;
		}
		case CharacterSlot2:
		{
			spawnCtxMenuForCharacter(m_scrollOffset + 1);
			break;
		}
		case CharacterSlot3:
		{
			spawnCtxMenuForCharacter(m_scrollOffset + 2);
			break;
		}
	}

	switch (popFirstButtonId())
	{
		case Interface::CreateCharacter:
		{
			if (auto game = dynamic_pointer_cast<Game>(sApplication->getRenderObject(Application::Ro::RoGame)))
				game->setStage(Game::Ro::RoCharacterCreation);
			break;
		}
		case CharacterSlot1:
		{
			pickCharacter(m_scrollOffset);
			break;
		}
		case CharacterSlot2:
		{
			pickCharacter(m_scrollOffset + 1);
			break;
		}
		case CharacterSlot3:
		{
			pickCharacter(m_scrollOffset + 2);
			break;
		}
		case LeftBtn:
		{
			m_scrollOffset -= 3;
			
			if (m_scrollOffset < 0)
				m_scrollOffset = 0;
			
			updatePageTxt();
			updateButtons();
			break;
		}
		case RightBtn:
		{
			if (m_scrollOffset + 3 < int(m_characters.size()))
				m_scrollOffset += 3;

			updatePageTxt();
			updateButtons();
			break;
		}
	}

	if (auto confirmBox = sApplication->popConfirmBox({ ConfirmMessageBox::ConfirmCode_SelectCharacter, ConfirmMessageBox::ConfirmCode_DeleteCharacter }))
	{
		switch (confirmBox->getCode())
		{
			case ConfirmMessageBox::ConfirmCode_SelectCharacter:
			{
				if (confirmBox->getResult() == ConfirmMessageBox::ConfirmBox_Yes)
				{
					GP_Client_EnterWorld packet;
					packet.m_playerGuid = m_chosenCharacter.guid;
					sConnector->sendPacket(packet.build(StlBuffer{}));

					// Stash the entering character so World::setController can spawn the
					// ClientPlayer (NewWorld carries pos+vars but not class/gender/portrait).
					World::s_pendingSelf = {};
					World::s_pendingSelf.guid     = m_chosenCharacter.guid;
					World::s_pendingSelf.classId  = static_cast<int>(m_chosenCharacter.classId);
					World::s_pendingSelf.gender   = m_chosenCharacter.gender;
					World::s_pendingSelf.portrait = m_chosenCharacter.portrait;
					World::s_pendingSelf.name     = m_chosenCharacter.name;

					sApplication->spawnTimedPopup("Loading", 300.0f);
				}

				break;
			}
			case ConfirmMessageBox::ConfirmCode_DeleteCharacter:
			{
				if (confirmBox->getResult() == ConfirmMessageBox::ConfirmBox_Yes)
				{
					GP_Client_DeleteCharacter packet;
					packet.m_playerGuid = m_chosenCharacter.guid;
					sConnector->sendPacket(packet.build(StlBuffer{}));
					sApplication->spawnTimedPopup("Loading", 3.0f);
				}

				break;
			}
		}
	}
}

void CharacterSelection::updatePageTxt()
{
	int page = 1 + (m_scrollOffset / 3);
	m_pageTxt->setOriginalString("PAGE " + to_string(page));
}

void CharacterSelection::render()
{
	// Dev auto-login: [Debug] AutoLogin enters the first character (or AutoLoginChar by name) once.
	if (!m_autoEntered && !m_characters.empty() && sConfig->getInt("Debug", "AutoLogin", 0))
	{
		m_autoEntered = true;
		const std::string want = sConfig->getString("Debug", "AutoLoginChar", "");
		m_chosenCharacter = m_characters[0];
		for (auto& c : m_characters) if (!want.empty() && c.name == want) { m_chosenCharacter = c; break; }

		GP_Client_EnterWorld packet;
		packet.m_playerGuid = m_chosenCharacter.guid;
		sConnector->sendPacket(packet.build(StlBuffer{}));
		World::s_pendingSelf = {};
		World::s_pendingSelf.guid     = m_chosenCharacter.guid;
		World::s_pendingSelf.classId  = static_cast<int>(m_chosenCharacter.classId);
		World::s_pendingSelf.gender   = m_chosenCharacter.gender;
		World::s_pendingSelf.portrait = m_chosenCharacter.portrait;
		World::s_pendingSelf.name     = m_chosenCharacter.name;
		sApplication->spawnTimedPopup("Loading", 300.0f);
	}

	m_topLeftCorner = { static_cast<int>(sApplication->sWf() - m_backgroundGui->getGlobalBounds().width) / 2, static_cast<int>(sApplication->sHf() - m_backgroundGui->getGlobalBounds().height) / 2 };
	m_bottomRightCorner = m_topLeftCorner + sf::Vector2i(m_backgroundGui->vbounds());

	m_leftBtn->setHidden(m_scrollOffset < 3);
	m_rightBtn->setHidden(m_scrollOffset + 3 >= int(m_characters.size()));

	m_background->renderStretch({ 0, 0 }, { sApplication->sWf(), sApplication->sHf() });
	m_backgroundGui->render(sf::Vector2f(m_topLeftCorner));
	m_pageTxt->draw(190 + m_topLeftCorner.x, 415 + m_topLeftCorner.y);
	__super::render();
}

void CharacterSelection::registerCharacter(const string& name, const PlayerDefines::Classes classId, const int level, const int guid, const int portrait, const int gender)
{
	Character c;
	c.name = name;
	c.classId = classId;
	c.level = level;
	c.guid = guid;
	c.portrait = portrait;
	c.gender = gender;
	m_characters.push_back(c);
	updateButtons();
}

void CharacterSelection::clearCharacters()
{
	m_characters.clear();
	updateButtons();
}

void CharacterSelection::updateButtons()
{
	if (m_scrollOffset < 0)
		m_scrollOffset = 0;

	static vector<ButtonData> buttondatas =
	{
		{ CharacterSlot1, CharacterSlot1_Name, CharacterSlot1_Subtext, CharacterSlot1_Portrait },
		{ CharacterSlot2, CharacterSlot2_Name, CharacterSlot2_Subtext, CharacterSlot2_Portrait },
		{ CharacterSlot3, CharacterSlot3_Name, CharacterSlot3_Subtext, CharacterSlot3_Portrait },
	};

	// Just hide everything and then we'll reveal what should be shown
	for (auto& itr : buttondatas)
	{
		getRenderObject(itr.button)->setHidden(true);
		getRenderObject(itr.name)->setHidden(true);
		getRenderObject(itr.subtext)->setHidden(true);
		getRenderObject(itr.portrait)->setHidden(true);
	}

	for (int i = 0; size_t(i) < buttondatas.size(); ++i)
	{
		auto charslot = m_scrollOffset + i;

		if (size_t(charslot) < m_characters.size())
			setButton(buttondatas[i], m_characters[charslot]);
	}
}