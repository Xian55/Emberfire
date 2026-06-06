#include "stdafx.h"
#include "Options.h"
#include "SpriteRo.h"
#include "ContentMgr.h"
#include "Sprite.h"
#include "Application.h"
#include "Game.h"
#include "Button.h"
#include "ConfirmMessageBox.h"
#include "TickBox.h"
#include "ScrollBar.h"
#include "TextLines.h"
#include "Keybinds.h"
#include "SelectionButtons.h"

#include "..\Files.h"
#include "..\StringHelpers.h"
#include "..\Shared\Config.h"

#include <assert.h>
#include <set>
#include <vector>
#include <string>

Options::Options(RenderObjectHolder& owner, const int id) :
	RenderObjectHolder(&owner, id),
	m_stage(Null)
{
	setStage(Stage::Main);
	setMultiInput(true);

	m_keybind_clicked_1_A = -1;
	m_keybind_clicked_1_B = -1;
	m_keybind_clicked_2_A = -1;
	m_keybind_clicked_2_B = -1;

	m_windowModeTxt = make_unique<Text>(sContentMgr->getFont(FontId::Palatino));
	m_windowModeTxt->setCharacterSize(14);
	m_windowModeTxt->setColorIfNot(sf::Color(78, 63, 52, 255));

	string currentTxt;

	if (sConfig->getBool("Window", "Fullscreen"))
		m_windowModeTxt->setOriginalString("Borderless Fullscreen");
	else
		m_windowModeTxt->setOriginalString("Windowed Mode");

	m_resolutionTxt = make_unique<Text>(sContentMgr->getFont(FontId::Palatino));
	m_resolutionTxt->setCharacterSize(14);
	m_resolutionTxt->setColorIfNot(sf::Color(78, 63, 52, 255));
	m_resolutionTxt->setOriginalString(to_string(sConfig->getInt("Window", "Width", 1280)) + "x" + to_string(sConfig->getInt("Window", "Height", 720)));
}

Options::~Options()
{

}

void Options::input()
{
	__super::input();

	auto confirmBox = sApplication->popConfirmBox({ ConfirmMessageBox::ConfirmCode_ExitGame, ConfirmMessageBox::ConfirmCode_Disconnect });

	if (confirmBox != nullptr && confirmBox->getResult() == ConfirmMessageBox::ConfirmBox_Yes)
	{
		if (confirmBox->getCode() == ConfirmMessageBox::ConfirmCode_ExitGame)
		{
			sApplication->cancel();
			return;
		}
		else if (confirmBox->getCode() == ConfirmMessageBox::ConfirmCode_Disconnect)
		{
			sApplication->logout();
			return;
		}
	}
	
	if (m_viewChoices != nullptr && m_viewChoices->getChosen() != m_stage)
		setStage(Stage(m_viewChoices->getChosen()));

	switch (popFirstButtonId())
	{
		case Main_ButtonSystem:
		{
			setStage(Stage::System);
			break;
		}
		case Main_ButtonLogout:
		{
			sApplication->spawnPopup("Disconnect?", ConfirmMessageBox::ConfirmBox_YesNo, ConfirmMessageBox::ConfirmCode_Disconnect);
			break;
		}
		case Main_ButtonExitGame:
		{
			sApplication->spawnPopup("Exit game?", ConfirmMessageBox::ConfirmBox_YesNo, ConfirmMessageBox::ConfirmCode_ExitGame);
			break;
		}
		case Main_ButtonReturnGame:
		{
			sApplication->game()->toggleOptions(false);
			break;
		}
		case ButtonGoBack:
		{
			setStage(Stage::Main);
			break;
		}
		case System_WindowMode:
		{
			if (auto owner = dynamic_cast<RenderObjectHolder*>(getOwner()))
			{
				owner->registerContextMenu(Interface::System_WindowModeCtx, getId(), sApplication->mousePos(),
					{
						"Windowed Mode",
						"Borderless Fullscreen",
					});
			}
			break;
		}
		case System_Resolution:
		{
			if (auto owner = dynamic_cast<RenderObjectHolder*>(getOwner()))
			{
				// the machine's supported resolutions (deduped by w x h, <= desktop, descending)
				vector<string> items;
				const sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
				set<pair<unsigned int, unsigned int>> seen;
				for (const auto& m : sf::VideoMode::getFullscreenModes())
				{
					if (m.width > desktop.width || m.height > desktop.height) continue;
					if (m.width < 1280 || m.height < 720) continue;
					if (!seen.insert({ m.width, m.height }).second) continue;
					items.push_back(to_string(m.width) + "x" + to_string(m.height));
					if (seen.size() >= 8) break;
				}
				owner->registerContextMenu(Interface::System_ResolutionCtx, getId(), sApplication->mousePos(), items);
			}
			break;
		}
	}

	if (m_stage == Stage::Keybindings)
	{
		auto lines_1_A = dynamic_pointer_cast<TextLines>(getRenderObject(Interface::Keybinds_List_1_A));
		auto lines_1_B = dynamic_pointer_cast<TextLines>(getRenderObject(Interface::Keybinds_List_1_B));
		auto lines_2_A = dynamic_pointer_cast<TextLines>(getRenderObject(Interface::Keybinds_List_2_A));
		auto lines_2_B = dynamic_pointer_cast<TextLines>(getRenderObject(Interface::Keybinds_List_2_B));

		if (m_keybind_clicked_1_A != -1 ||
			m_keybind_clicked_1_B != -1 ||
			m_keybind_clicked_2_A != -1 ||
			m_keybind_clicked_2_B != -1)
		{
			auto keyDown = sApplication->popKeyDown();

			if (keyDown != nullptr &&
				keyDown->code != sf::Keyboard::LControl &&
				keyDown->code != sf::Keyboard::LAlt &&
				keyDown->code != sf::Keyboard::LShift)
			{
				string keybindName;

				if (keyDown->code == sf::Keyboard::Escape)
					sContentMgr->playSound(SfxId::WindowTargetClose);

				// Fetch the label
				if (m_keybind_clicked_1_A != -1)
					keybindName = lines_1_A->getLine(m_keybind_clicked_1_A)->getTextStr();

				if (m_keybind_clicked_1_B != -1)
					keybindName = lines_1_A->getLine(m_keybind_clicked_1_B)->getTextStr();

				if (m_keybind_clicked_2_A != -1)
					keybindName = lines_2_A->getLine(m_keybind_clicked_2_A)->getTextStr();

				if (m_keybind_clicked_2_B != -1)
					keybindName = lines_2_A->getLine(m_keybind_clicked_2_B)->getTextStr();

				if (keyDown->code != sf::Keyboard::Escape)
				{
					// Assign the value to config
					const int key = keyDown->code;

					int modifier = -1;

					if (keyDown->control)
						modifier = sf::Keyboard::LControl;
					else if (keyDown->alt)
						modifier = sf::Keyboard::LAlt;
					else if (keyDown->shift)
						modifier = sf::Keyboard::LShift;

					sConfig->setInt("KeyBind", keybindName.c_str(), keyDown->code);
					sConfig->setInt("KeyBindModifier", keybindName.c_str(), modifier);

					sContentMgr->playSound(SfxId::BoxMessage);
				}

				sKeybinds->loadKeybind(keybindName);

				// Load the value to line list
				if (m_keybind_clicked_1_A != -1)
					lines_1_B->getLine(m_keybind_clicked_1_A)->setTextStr(sKeybinds->deduceKeybindStr(keybindName));

				if (m_keybind_clicked_1_B != -1)
					lines_1_B->getLine(m_keybind_clicked_1_B)->setTextStr(sKeybinds->deduceKeybindStr(keybindName));

				if (m_keybind_clicked_2_A != -1)
					lines_2_B->getLine(m_keybind_clicked_2_A)->setTextStr(sKeybinds->deduceKeybindStr(keybindName));

				if (m_keybind_clicked_2_B != -1)
					lines_2_B->getLine(m_keybind_clicked_2_B)->setTextStr(sKeybinds->deduceKeybindStr(keybindName));

				// Done
				m_keybind_clicked_1_A = -1;
				m_keybind_clicked_1_B = -1;
				m_keybind_clicked_2_A = -1;
				m_keybind_clicked_2_B = -1;
			}
		}
		else
		{
			m_keybind_clicked_1_A = lines_1_A->popPendingClickedLineId();
			m_keybind_clicked_1_B = lines_1_B->popPendingClickedLineId();
			m_keybind_clicked_2_A = lines_2_A->popPendingClickedLineId();
			m_keybind_clicked_2_B = lines_2_B->popPendingClickedLineId();

			if (m_keybind_clicked_1_A != -1)
			{
				lines_1_B->getLine(m_keybind_clicked_1_A)->setTextStr("____");
				sContentMgr->playSound(SfxId::WindowTargetOpen);
			}

			if (m_keybind_clicked_1_B != -1)
			{
				lines_1_B->getLine(m_keybind_clicked_1_B)->setTextStr("____");
				sContentMgr->playSound(SfxId::WindowTargetOpen);
			}

			if (m_keybind_clicked_2_A != -1)
			{
				lines_2_B->getLine(m_keybind_clicked_2_A)->setTextStr("____");
				sContentMgr->playSound(SfxId::WindowTargetOpen);
			}

			if (m_keybind_clicked_2_B != -1)
			{
				lines_2_B->getLine(m_keybind_clicked_2_B)->setTextStr("____");
				sContentMgr->playSound(SfxId::WindowTargetOpen);
			}

			if (sApplication->popKeyDown(sf::Keyboard::Escape))
				setStage(Stage::Main);
		}
	}
	else
	{
		if (sApplication->popKeyUp(sf::Keyboard::Escape))
			setStage(Stage::Main);
	}

	save();
}

void Options::render()
{
	sf::RectangleShape rectangle;
	rectangle.setSize(sf::Vector2f(sf::Vector2i(sApplication->sW(), sApplication->sH())));
	rectangle.setFillColor(sf::Color(0, 0, 0, 75));
	sApplication->canvas().draw(rectangle);

	recalcOptionsBounds();

	__super::render();

	if (m_stage == Stage::System)
	{
		// cover the two baked, now-redundant subtitle texts (they're part of the panel art) so the new
		// Resolution dropdown can take their place. Semi-transparent black darkens the faint grey into the panel.
		auto cover = [&](int x, int y, int w, int h) {
			sf::RectangleShape r;
			r.setPosition((float)(m_topLeftCorner.x + x), (float)(m_topLeftCorner.y + y));
			r.setSize({ (float)w, (float)h });
			r.setFillColor(sf::Color(0, 0, 0, 200));
			sApplication->canvas().draw(r);
		};
		cover(92, 213, 260, 18);   // "Additional options available in your config.ini"
		cover(92, 300, 270, 18);   // "The playback volume for combat abilities etc."

		m_windowModeTxt->draw(m_topLeftCorner.x + 120, m_topLeftCorner.y + 195);
		m_resolutionTxt->draw(m_topLeftCorner.x + 120, m_topLeftCorner.y + 243);
	}
}

void Options::save()
{
	if (auto tick = dynamic_pointer_cast<TickBox>(getRenderObject(Interface::System_CtmTick)))
		sConfig->setInt("System", "CtmTick", tick->ticked());

	if (auto tick = dynamic_pointer_cast<TickBox>(getRenderObject(Interface::System_StickyTick)))
		sConfig->setInt("System", "StickyTick", tick->ticked());

	if (auto tick = dynamic_pointer_cast<TickBox>(getRenderObject(Interface::System_EnemyNameplateTick)))
		sConfig->setInt("System", "EnemyNameplateTick", tick->ticked());

	if (auto tick = dynamic_pointer_cast<TickBox>(getRenderObject(Interface::System_FriendlyNameplateTick)))
		sConfig->setInt("System", "FriendlyNameplateTick", tick->ticked());

	if (auto tick = dynamic_pointer_cast<TickBox>(getRenderObject(Interface::System_YourNameTick)))
		sConfig->setInt("System", "YourNameTick", tick->ticked());

	if (auto tick = dynamic_pointer_cast<TickBox>(getRenderObject(Interface::System_PlayerRanksTick)))
		sConfig->setInt("System", "PlayerRanksTick", tick->ticked());

	if (auto tick = dynamic_pointer_cast<TickBox>(getRenderObject(Interface::System_ShowPlayerNameTick)))
		sConfig->setInt("System", "ShowPlayerNameTick", tick->ticked());

	if (auto tick = dynamic_pointer_cast<TickBox>(getRenderObject(Interface::System_ShowNpcNameTick)))
		sConfig->setInt("System", "ShowNpcNameTick", tick->ticked());

	if (auto bar = dynamic_pointer_cast<ScrollBar>(getRenderObject(Interface::System_MusicVolumeBar)))
		sConfig->setInt("System", "MusicVolumeBar", bar->getScrollOffset());

	if (auto bar = dynamic_pointer_cast<ScrollBar>(getRenderObject(Interface::System_SfxVolumeBar)))
		sConfig->setInt("System", "SfxVolumeBar", bar->getScrollOffset());

	sApplication->loadCfg();
}

void Options::recalcOptionsBounds()
{
	m_topLeftCorner = { (sApplication->sW() / 2) - static_cast<int>(m_bgSprite->getGlobalBounds().width / 2.0f), (sApplication->sH() / 2) - static_cast<int>(m_bgSprite->getGlobalBounds().height / 2.0f) };
	m_bottomRightCorner = m_topLeftCorner + sf::Vector2i(sf::Vector2f(m_bgSprite->getGlobalBounds().width, m_bgSprite->getGlobalBounds().height));
}

void Options::setBackground(const string& filename)
{
	m_bgSprite = sContentMgr->spawnSprite(filename);
	ASSERT(m_bgSprite != nullptr);

	recalcOptionsBounds();
	attachAddObj(make_shared<SpriteRo>(*this, m_bgSprite, BackgroundImg), {});
}

void Options::setStage(const Stage stage)
{
	if (m_stage == stage)
		return;

	clear();
	m_stage = stage;
	m_viewChoices = nullptr;

	if (m_stage == Stage::Main)
	{
		setBackground("options_background.png");

		attachAddObj(make_shared<Button>(*this, "generic_highlight", Interface::Main_ButtonSystem), sf::Vector2i(70, 50));
		attachAddObj(make_shared<Button>(*this, "generic_highlight", Interface::Main_ButtonLogout), sf::Vector2i(70, 120));
		attachAddObj(make_shared<Button>(*this, "generic_highlight", Interface::Main_ButtonExitGame), sf::Vector2i(70, 188));
		attachAddObj(make_shared<Button>(*this, "generic_highlight", Interface::Main_ButtonReturnGame, sf::Vector2i{}, SfKeyEvent(sf::Keyboard::Escape)), sf::Vector2i(70, 255));
	}
	else
	{
		if (m_stage == Stage::System)
		{
			setBackground("options_background_system.png");

			attachAddObj(make_shared<Button>(*this, "options_wmode", Interface::System_WindowMode), sf::Vector2i(91, 184));
			attachAddObj(make_shared<Button>(*this, "options_wmode", Interface::System_Resolution), sf::Vector2i(91, 232));

			auto sfxVolumeBar = make_shared<ScrollBar>(*this, "side_scroll_up", "side_scroll_down", ScrollBar::ScrollLeftRight, "side_scroll_box", Interface::System_SfxVolumeBar);
			sfxVolumeBar->getScrollDownButton()->setPos(sf::Vector2i(100, 328) + m_topLeftCorner);
			sfxVolumeBar->getScrollUpButton()->setPos(sf::Vector2i(395, 328) + m_topLeftCorner);
			sfxVolumeBar->setMaxOffset(100);
			sfxVolumeBar->setScrollOffset(sConfig->getInt("System", "SfxVolumeBar"));
			sfxVolumeBar->setAllowMousewheelOnlyMousedOver(true);
			addRenderObject(sfxVolumeBar);
			attachObj(sfxVolumeBar->getScrollDownButton(), sf::Vector2i(100, 328));
			attachObj(sfxVolumeBar->getScrollUpButton(), sf::Vector2i(395, 328));

			auto musicVolumeBar = make_shared<ScrollBar>(*this, "side_scroll_up", "side_scroll_down", ScrollBar::ScrollLeftRight, "side_scroll_box", Interface::System_MusicVolumeBar);
			musicVolumeBar->getScrollDownButton()->setPos(sf::Vector2i(100, 472) + m_topLeftCorner);
			musicVolumeBar->getScrollUpButton()->setPos(sf::Vector2i(395, 472) + m_topLeftCorner);
			musicVolumeBar->setMaxOffset(100);
			musicVolumeBar->setScrollOffset(sConfig->getInt("System", "MusicVolumeBar"));
			musicVolumeBar->setAllowMousewheelOnlyMousedOver(true);
			addRenderObject(musicVolumeBar);
			attachObj(musicVolumeBar->getScrollDownButton(), sf::Vector2i(100, 472));
			attachObj(musicVolumeBar->getScrollUpButton(), sf::Vector2i(395, 472));

			int x1 = 575;
			int x2 = 775;
			int y1 = 383;
			int y2 = 436;
			int y3 = 490;

			attachAddObj(make_shared<TickBox>(*this, Interface::System_CtmTick, "slider_off", "slider_on", sf::Vector2i{}, sConfig->getInt("System", "CtmTick")), sf::Vector2i(575, 186));
			attachAddObj(make_shared<TickBox>(*this, Interface::System_StickyTick, "slider_off", "slider_on", sf::Vector2i{}, sConfig->getInt("System", "StickyTick")), sf::Vector2i(575, 301));
			attachAddObj(make_shared<TickBox>(*this, Interface::System_EnemyNameplateTick, "tick_box_empty", "tick_box_full", sf::Vector2i{}, sConfig->getInt("System", "EnemyNameplateTick")), sf::Vector2i(x1, y1));
			attachAddObj(make_shared<TickBox>(*this, Interface::System_FriendlyNameplateTick, "tick_box_empty", "tick_box_full", sf::Vector2i{}, sConfig->getInt("System", "FriendlyNameplateTick")), sf::Vector2i(x2, y1));
			attachAddObj(make_shared<TickBox>(*this, Interface::System_YourNameTick, "tick_box_empty", "tick_box_full", sf::Vector2i{}, sConfig->getInt("System", "YourNameTick")), sf::Vector2i(x1, y2));
			attachAddObj(make_shared<TickBox>(*this, Interface::System_PlayerRanksTick, "tick_box_empty", "tick_box_full", sf::Vector2i{}, sConfig->getInt("System", "PlayerRanksTick")), sf::Vector2i(x2, y2));
			attachAddObj(make_shared<TickBox>(*this, Interface::System_ShowPlayerNameTick, "tick_box_empty", "tick_box_full", sf::Vector2i{}, sConfig->getInt("System", "ShowPlayerNameTick")), sf::Vector2i(x1, y3));
			attachAddObj(make_shared<TickBox>(*this, Interface::System_ShowNpcNameTick, "tick_box_empty", "tick_box_full", sf::Vector2i{}, sConfig->getInt("System", "ShowNpcNameTick")), sf::Vector2i(x2, y3));
		}
		else if (m_stage == Stage::Keybindings)
		{
			setBackground("options_background_keybind.png");

			// Left
			{
				// Binds List
				auto bindList = make_shared<TextLines>(*this, Interface::Keybinds_List_1_A, FontId::Palatino, Util::GeoBox2d(0, 0, 500, 370));
				bindList->setClickableLines(true);
				bindList->setDialogCharacterSize(20);
				attachAddObj(bindList, sf::Vector2i(80, 155));

				auto bindValueList = make_shared<TextLines>(*this, Interface::Keybinds_List_1_B, FontId::Palatino, Util::GeoBox2d(0, 0, 150, 370));
				bindValueList->setClickableLines(true);
				bindValueList->setDialogCharacterSize(20);
				attachAddObj(bindValueList, sf::Vector2i(350, 155));

				vector<string> binds;
				Util::readLinesFromFile("scripts\\interface\\keybinds_1.txt", binds);

				for (auto& str : binds)
					bindList->addLine(str, sf::Color(157, 135, 120, 255));

				for (auto& str : binds)
					bindValueList->addLine(sKeybinds->deduceKeybindStr(str), sf::Color(157, 135, 120, 255));

				// Given to the binds list
				auto scrollBar = make_shared<ScrollBar>(*bindList, "scrollbar_thin_blank", "scrollbar_thin_blank", ScrollBar::ScrollTopDown, "scrollbar_thin", Interface::Keybinds_Scrollbar_1);
				attachObj(scrollBar->getScrollUpButton(), sf::Vector2i(437, 151));
				//attachObj(scrollBar->getScrollSquare(), sf::Vector2i(437, 151));
				attachObj(scrollBar->getScrollDownButton(), sf::Vector2i(437, 475));
				scrollBar->setAllowMousewheel(false);
				bindList->setScrollObject(scrollBar);
				bindValueList->setScrollObject(scrollBar);
				addRenderObject(scrollBar);
			}

			// Right
			{
				// Binds List
				auto bindList = make_shared<TextLines>(*this, Interface::Keybinds_List_2_A, FontId::Palatino, Util::GeoBox2d(0, 0, 500, 370));
				bindList->setClickableLines(true);
				bindList->setDialogCharacterSize(20);
				attachAddObj(bindList, sf::Vector2i(555, 155));

				auto bindValueList = make_shared<TextLines>(*this, Interface::Keybinds_List_2_B, FontId::Palatino, Util::GeoBox2d(0, 0, 150, 370));
				bindValueList->setClickableLines(true);
				bindValueList->setDialogCharacterSize(20);
				attachAddObj(bindValueList, sf::Vector2i(835, 155));

				vector<string> binds;
				Util::readLinesFromFile("scripts\\interface\\keybinds_2.txt", binds);

				for (auto& str : binds)
					bindList->addLine(str, sf::Color(157, 135, 120, 255));

				for (auto& str : binds)
					bindValueList->addLine(sKeybinds->deduceKeybindStr(str), sf::Color(157, 135, 120, 255));

				// Given to the binds list
				auto scrollBar = make_shared<ScrollBar>(*bindList, "scrollbar_thin_blank", "scrollbar_thin_blank", ScrollBar::ScrollTopDown, "scrollbar_thin", Interface::Keybinds_Scrollbar_2);
				attachObj(scrollBar->getScrollUpButton(), sf::Vector2i(949, 151));
				//attachObj(scrollBar->getScrollSquare(), sf::Vector2i(949, 151));
				attachObj(scrollBar->getScrollDownButton(), sf::Vector2i(949, 475));
				scrollBar->setAllowMousewheel(false);
				bindList->setScrollObject(scrollBar);
				bindValueList->setScrollObject(scrollBar);
				addRenderObject(scrollBar);
			}
		}

		attachAddObj(make_shared<Button>(*this, "generic_highlight", Interface::ButtonGoBack, sf::Vector2i{}), sf::Vector2i(462, 602));

		addRenderObject(m_viewChoices = make_shared<SelectionButtons>(*this, Interface::System_Tabs));
		m_viewChoices->addRenderObject(attachObj(make_shared<Button>(*m_viewChoices, "equipment_view", System, sf::Vector2i{}), sf::Vector2i(402, 70)));
		m_viewChoices->addRenderObject(attachObj(make_shared<Button>(*m_viewChoices, "equipment_view", Keybindings, sf::Vector2i{}), sf::Vector2i(535, 70)));
		m_viewChoices->setChosen(m_stage);
		m_viewChoices->popChange();
	}
}

void Options::notifyCtxMenuClicked(const int id, const string& lineClicked) /*final*/
{
	if (id == Interface::System_WindowModeCtx)
	{
		if (lineClicked == "Windowed Mode")
		{
			sApplication->setFullscreenBorderless(false);
			sApplication->setNativeRendering(true);   // native (crisp) windowed
			sApplication->resetCanvas();
			m_windowModeTxt->setOriginalString(lineClicked);
		}
		else if (lineClicked == "Borderless Fullscreen")
		{
			sApplication->setNativeRendering(true);   // render natively (crisp), no 1080 upscale
			sApplication->resetCanvas();
			sApplication->setFullscreenBorderless(true);
			m_windowModeTxt->setOriginalString(lineClicked);
		}
	}
	else if (id == Interface::System_ResolutionCtx)
	{
		const auto xpos = lineClicked.find('x');
		if (xpos != string::npos)
		{
			const int w = atoi(lineClicked.substr(0, xpos).c_str());
			const int h = atoi(lineClicked.substr(xpos + 1).c_str());
			if (w >= 1280 && h >= 720)
			{
				sApplication->setResolution(w, h);   // switches to windowed at this size
				m_windowModeTxt->setOriginalString("Windowed Mode");
				m_resolutionTxt->setOriginalString(lineClicked);
			}
		}
	}
}
