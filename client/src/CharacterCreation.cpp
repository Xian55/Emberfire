#include "stdafx.h"
#include "CharacterCreation.h"
#include "ContentMgr.h"
#include "Application.h"
#include "Sprite.h"
#include "Button.h"
#include "PromptBox.h"
#include "SelectionButtons.h"
#include "Game.h"
#include "ConfirmMessageBox.h"
#include "Connector.h"
#include "Tooltip.h"
#include "SpriteRo.h"

#include "..\Rand.h"
#include "..\Shared\GamePacketClient.h"
#include "..\Shared\CharacterDefines.h"

#include <assert.h>

CharacterCreation::CharacterCreation(RenderObjectHolder& owner, const int id) :
	RenderObjectHolder(&owner, id)
{
	setMultiInput(true);
	sContentMgr->loopMusic(MusicId::AionCreation, false);

	ASSERT(m_background = sContentMgr->spawnSprite("main_bg.jpg"));
	ASSERT(m_backgroundGui = sContentMgr->spawnSprite("main_creation.png"));

	addRenderObject(attachObj(make_shared<Button>(*this, "generic_highlight", Interface::CreateCharacter, sf::Vector2i{}, SfKeyEvent(sf::Keyboard::Return)), sf::Vector2i(447, 686)));
	addRenderObject(attachObj(make_shared<Button>(*this, "left_arrow", Interface::PortraitsLeft), sf::Vector2i(350, 350)));
	addRenderObject(attachObj(make_shared<Button>(*this, "right_arrow", Interface::PortraitsRight), sf::Vector2i(670, 350)));
	addRenderObject(attachObj(make_shared<Button>(*this, "cancel_back", Interface::CancelBack), sf::Vector2i(int(m_backgroundGui->vbounds().x), int(sApplication->sHf() - m_backgroundGui->getGlobalBounds().height) / 2)));
	
	// Character name prompt
	m_namePrompt = make_shared<PromptBox>(*this, Interface::NamePrompt, FontId::Palatino, nullptr, sf::Vector2i{}, 200, sf::Color(157, 137, 119, 255), false);
	m_namePrompt->setPromptCharacterSize(18);
	m_namePrompt->setPromptMaxStrLen(CharacterDefines::Names::MaxLength);
	m_namePrompt->setAsciiOnly(true);
	addRenderObject(attachObj(m_namePrompt, sf::Vector2i(380, 610)));

	// Genders
	auto femaleIcon = make_shared<Button>(*this, "create_class", Interface::FemaleButton);
	auto maleIcon = make_shared<Button>(*this, "create_class", Interface::MaleButton);
	addRenderObject(attachObj(femaleIcon, sf::Vector2i(805, 465)));
	addRenderObject(attachObj(maleIcon, sf::Vector2i(879, 465)));
	m_selectedGender = make_shared<SpriteRo>(*this, sContentMgr->spawnSprite("create_class_press.png"), 0);

	// Class icons
	m_classChoices = make_shared<SelectionButtons>(*this, Interface::ClassSelection);
	auto classIconPaladin = attachObj(make_shared<Button>(*m_classChoices, "create_class", PlayerDefines::Classes::Paladin), sf::Vector2i(44 + (60 * 0), 465));
	auto classIconMage = attachObj(make_shared<Button>(*m_classChoices, "create_class", PlayerDefines::Classes::Mage), sf::Vector2i(44 + (60 * 1), 465));
	auto classIconRanger = attachObj(make_shared<Button>(*m_classChoices, "create_class", PlayerDefines::Classes::Ranger), sf::Vector2i(44 + (60 * 2), 465));
	auto classIconBishop = attachObj(make_shared<Button>(*m_classChoices, "create_class", PlayerDefines::Classes::Bishop), sf::Vector2i(44 + (60 * 3), 465));
	m_classChoices->addRenderObject(classIconPaladin);
	m_classChoices->addRenderObject(classIconMage);
	m_classChoices->addRenderObject(classIconRanger);
	m_classChoices->addRenderObject(classIconBishop);

	// Default selected as Paladin
	m_classChoices->setChosen(PlayerDefines::Paladin);
	m_classChoices->popChange();
	addRenderObject(m_classChoices);

	// Paladin tooltip
	auto tooltipPaladin = make_shared<Tooltip>(*classIconPaladin, sf::Vector2i{});
	tooltipPaladin->addLine(FontId::Helvetica, 15, PlayerFunctions::className(PlayerDefines::Classes::Paladin), sf::Color(PlayerFunctions::classColor(PlayerDefines::Classes::Paladin)));
	tooltipPaladin->addLine(FontId::Helvetica, 15, "Divine warriors devoted to honor and piety. They live to defend, protect and serve all believers. Can use Shields and Melee weapons.");
	tooltipPaladin->addLine(FontId::Trebuchet, 15, "Difficulty: ", sf::Color(240, 197, 2, 255), false);
	tooltipPaladin->addLine(FontId::Trebuchet, 15, "Beginner", sf::Color(145, 233, 1, 255));
	attachObj(tooltipPaladin, classIconPaladin->getOffset() + sf::Vector2i(-20, 90));
	static_pointer_cast<Button>(classIconPaladin)->setTooltip(tooltipPaladin);	

	// Mage tooltip
	auto tooltipMage = make_shared<Tooltip>(*classIconMage, sf::Vector2i{});
	tooltipMage->addLine(FontId::Helvetica, 15, PlayerFunctions::className(PlayerDefines::Classes::Mage), sf::Color(PlayerFunctions::classColor(PlayerDefines::Classes::Mage)));
	tooltipMage->addLine(FontId::Helvetica, 15, "Powerful arcane spellcasters born with magic in their blood who gradually learn to harness their innate abilities over time. Can use Staves and Wands.");
	tooltipMage->addLine(FontId::Trebuchet, 15, "Difficulty: ", sf::Color(240, 197, 2, 255), false);
	tooltipMage->addLine(FontId::Trebuchet, 15, "Advanced", sf::Color(215, 63, 19, 255));
	attachObj(tooltipMage, classIconPaladin->getOffset() + sf::Vector2i(-20, 90));
	static_pointer_cast<Button>(classIconMage)->setTooltip(tooltipMage);

	// Ranger tooltip
	auto tooltipRanger = make_shared<Tooltip>(*classIconRanger, sf::Vector2i{});
	tooltipRanger->addLine(FontId::Helvetica, 15, PlayerFunctions::className(PlayerDefines::Classes::Ranger), sf::Color(PlayerFunctions::classColor(PlayerDefines::Classes::Ranger)));
	tooltipRanger->addLine(FontId::Helvetica, 15, "A wanderer in the wilds, their fierce independence makes them well suited to adventuring and mischief. Can use Bows and Daggers.");
	tooltipRanger->addLine(FontId::Trebuchet, 15, "Difficulty: ", sf::Color(240, 197, 2, 255), false);
	tooltipRanger->addLine(FontId::Trebuchet, 15, "Moderate", sf::Color(226, 218, 0, 255));
	attachObj(tooltipRanger, classIconPaladin->getOffset() + sf::Vector2i(-20, 90));
	static_pointer_cast<Button>(classIconRanger)->setTooltip(tooltipRanger);

	// Bishop tooltip
	auto tooltipBishop = make_shared<Tooltip>(*classIconBishop, classIconBishop->getTopLeftCornerRef() + sf::Vector2i(-60, 90));
	tooltipBishop->addLine(FontId::Helvetica, 15, PlayerFunctions::className(PlayerDefines::Classes::Bishop), sf::Color(PlayerFunctions::classColor(PlayerDefines::Classes::Bishop)));
	tooltipBishop->addLine(FontId::Helvetica, 15, "Best known as healers, they are members of the divine faith who have the ability to cast priest spells from the gods they worship. Can use Shields, Staves and Maces.");
	tooltipBishop->addLine(FontId::Trebuchet, 15, "Difficulty: ", sf::Color(240, 197, 2, 255), false);
	tooltipBishop->addLine(FontId::Trebuchet, 15, "Moderate", sf::Color(226, 218, 0, 255));
	attachObj(tooltipBishop, classIconPaladin->getOffset() + sf::Vector2i(-20, 90));
	static_pointer_cast<Button>(classIconBishop)->setTooltip(tooltipBishop);

	// Flag to initialize m_gender and m_portrait variables after first render
	m_portairtId = -1;
}

CharacterCreation::~CharacterCreation()
{

}

void CharacterCreation::input()
{
	sApplication->setCurrentPrompt(m_namePrompt.get());

	__super::input();

	if (m_classChoices->popChange())
		sContentMgr->playSound(SfxId::WindowTargetOpen);

	switch (popFirstButtonId())
	{
		case Interface::PortraitsLeft:
		{
			--m_portairtId;
			updatePortrait();
			break;
		}
		case Interface::PortraitsRight:
		{
			++m_portairtId;
			updatePortrait();
			break;
		}
		case Interface::MaleButton:
		{
			if (m_gender != PlayerDefines::Male)
			{
				m_gender = PlayerDefines::Male;
				updatePortrait();
				sContentMgr->playSound(SfxId::WindowTargetOpen);				
			}

			break;
		}
		case Interface::FemaleButton:
		{
			if (m_gender != PlayerDefines::Female)
			{
				m_gender = PlayerDefines::Female;
				updatePortrait();
				sContentMgr->playSound(SfxId::WindowTargetOpen);
			}

			break;
		}
		case Interface::CancelBack:
		{
			if (auto game = dynamic_pointer_cast<Game>(sApplication->getRenderObject(Application::Ro::RoGame)))
			{
				sConnector->sendPacket(GP_Client_CharacterList{}.build(StlBuffer{}));
				game->setStage(Game::Ro::RoCharacterSelection);
				sApplication->spawnTimedPopup("Loading", 2.0f);
			}

			break;
		}
		case Interface::CreateCharacter:
		{
			const string desiredName = m_namePrompt->getContent();

			// We'll also check server side but this will save some traffic to check here as well
			if (auto error = CharacterFunctions::invalidNameError(desiredName))
			{
				sApplication->spawnPopup(CharacterFunctions::nameErrorStr(error), ConfirmMessageBox::ConfirmBox_Ok, ConfirmMessageBox::ConfirmCode_Null);
			}
			else
			{
				GP_Client_CharCreate packet;
				packet.m_gender = m_gender;
				packet.m_portrait = m_portairtId;
				packet.m_name = CharacterFunctions::formatName(m_namePrompt->getContent());
				packet.m_classId = m_classChoices->getChosen();
				sConnector->sendPacket(packet.build(StlBuffer{}));
				sApplication->spawnTimedPopup("Loading", 300.0f);
			}

			break;
		}
	}
}

void CharacterCreation::render()
{
	m_topLeftCorner = { static_cast<int>(sApplication->sWf() - m_backgroundGui->getGlobalBounds().width) / 2, static_cast<int>(sApplication->sHf() - m_backgroundGui->getGlobalBounds().height) / 2 };
	m_bottomRightCorner = m_topLeftCorner + sf::Vector2i(m_backgroundGui->vbounds());

	// Format our name to be like Name instead of nAmE etc.
	const string formatedName = CharacterFunctions::formatName(m_namePrompt->getContent());

	if (m_namePrompt->getContent() != formatedName)
		m_namePrompt->setContent(formatedName);

	m_background->renderStretch({ 0, 0 }, { sApplication->sWf(), sApplication->sHf() });
	m_backgroundGui->render(sf::Vector2f(m_topLeftCorner));

	__super::render();

	if (m_portraitSprite != nullptr)
		m_portraitSprite->renderAsCircle(sf::Vector2f(sApplication->centerOfScreen()), 105, { 0, m_portraitOffset });

	if (auto ro = getRenderObject(m_gender == PlayerDefines::Male ? Interface::MaleButton : Interface::FemaleButton))
		m_selectedGender->getTopLeftCornerRef() = ro->getTopLeftCornerRef();

	m_selectedGender->attemptRender();

	if (m_portairtId == -1)
	{
		m_gender = PlayerDefines::Male;
		m_portairtId = Util::irand(1, CharacterDefines::Portraits::NumMale);
		updatePortrait();
	}
}

void CharacterCreation::setSelectedClass(const PlayerDefines::Classes classId)
{
	m_classChoices->setChosen(classId);
}

void CharacterCreation::updatePortrait()
{
	int maxVal = 0;

	if (m_gender == PlayerDefines::Male)
		maxVal = CharacterDefines::Portraits::NumMale;
	else
		maxVal = CharacterDefines::Portraits::NumFemale;

	if (m_portairtId <= 0)
		m_portairtId = maxVal;

	if (m_portairtId > maxVal)
		m_portairtId = 1;

	m_portraitSprite = sContentMgr->spawnPlayerPortrait(m_portairtId, m_gender);
	m_portraitSprite->setHotspotEasy(true, true);
	m_portraitOffset = sContentMgr->getPortraitOffset(*m_portraitSprite);
}