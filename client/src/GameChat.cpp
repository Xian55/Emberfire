#include "stdafx.h"
#include "GameChat.h"
#include "SpriteRo.h"
#include "ContentMgr.h"
#include "Application.h"
#include "Sprite.h"
#include "Button.h"
#include "PromptBox.h"
#include "ScrollBar.h"
#include "Connector.h"
#include "Text.h"
#include "World.h"
#include "lua/LuaEngine.h"
#include "ContextMenu.h"
#include "GuildRoster.h"
#include "ClientPlayer.h"
#include "ItemIcon.h"
#include "Tooltip.h"
#include "ConsoleWindow.h"

#include "..\Files.h"
#include "..\StringHelpers.h"

#include "..\Shared\GamePacketClient.h"
#include "..\Shared\CharacterDefines.h"
#include "..\Shared\SpellDefines.h"
#include "..\SqlConnector\QueryResult.h"

GameChat::GameChat(RenderObjectHolder& owner, const int id, const sf::Vector2i& pos) :
	RenderObjectHolder(&owner, id),
	m_channel(ChatDefines::Channels::Say),
	m_channelPrevious(ChatDefines::Channels::Say)
{
	setMultiInput(true);
	updateTopLeftCorner(pos);

	// Background
	auto bgSpr = sContentMgr->spawnSprite("game_chat_backdrop.png");
	m_bgSprite = make_shared<SpriteRo>(*this, bgSpr, 0);
	m_bgSprite->setPos(pos);
	addRenderObject(attachObj(m_bgSprite, sf::Vector2i{}));

	// Given to the prompt box
	shared_ptr<ScrollBar> scrollBar = make_shared<ScrollBar>(*this, "game_chat_up", "game_chat_down", ScrollBar::ScrollBottomUp, "");
	attachObj(scrollBar->getScrollUpButton(), sf::Vector2i(5, 101));
	attachObj(scrollBar->getScrollDownButton(), sf::Vector2i(5, 125));

	// Given to the prompt box
	m_promptEnterButton = make_shared<Button>(*this, "game_chat_enter", 0, sf::Vector2i{}, SfKeyEvent(sf::Keyboard::Return));
	m_promptEnterButton->setKeyDown(true);
	attachObj(m_promptEnterButton, sf::Vector2i(425, 145));

	// PromptBox class will destruct the ScrollBar for us, which destroys the two buttons
	m_promptBox = make_shared<PromptBox>(*this, Interface::RoPromptBox, FontId::Helvetica, m_promptEnterButton, sf::Vector2i{}, 390, sf::Color(0xFFFFFFFF));
	m_promptBox->setDialogCharacterSize(Defines::CharacterSize);
	m_promptBox->setPromptCharacterSize(Defines::CharacterSize);
	m_promptBox->setScrollObject(scrollBar);
	m_promptBox->setLeading(2);
	m_promptBox->setColor(getChatColor(ChatDefines::Say));
	m_promptBox->setAllowStringBeyondRender(false);
	m_promptBox->setBounds(Util::GeoBox2d(0, -150, 460, 120));
	m_promptBox->setClickableLines(true);
	m_promptBox->setClickableMouseButton(sf::Mouse::Right);
	m_promptBox->setShadowOffset(1);
	addRenderObject(attachObj(m_promptBox, sf::Vector2i(30, 157)));

	// Buttons 
	addRenderObject(attachObj(m_fullUpButton = make_shared<Button>(*this, "game_chat_fullup", Interface::RoFullUp), sf::Vector2i(5, 71)));
	addRenderObject(attachObj(m_fullDownButton = make_shared<Button>(*this, "game_chat_fulldown", Interface::RoFullDown), sf::Vector2i(5, 149)));
	m_closeLinkTooltipBtn = make_shared<Button>(*this, "panel_close", Interface::RoCloseTooltipBtn), sf::Vector2i{};
}

GameChat::~GameChat()
{

}

void GameChat::input()
{
	if (m_promptBox->popEmptyEnter())
	{
		setInUse(false);
		return;
	}

	if (m_inUse)
		sApplication->setCurrentPrompt(m_promptBox.get());

	// Messes with mouse over logic for the scroll object so that if we mouse over any part of the chat box we can scroll wheel it
	m_promptBox->getScrollObject()->setAllowMousewheel(isMousedOver());

	__super::input();

	switch (popFirstButtonId())
	{
		case Interface::RoFullDown:
			m_promptBox->getScrollObject()->setScrollOffset(0);
			break;
		case Interface::RoFullUp:
			m_promptBox->getScrollObject()->setScrollOffset(m_promptBox->getScrollObject()->getMaxOffset());
			break;
	}
	
	m_fullUpButton->setFlashing(m_promptBox->getScrollObject()->getScrollOffset() != 0 && m_promptBox->getScrollObject()->getScrollOffset() < m_promptBox->getScrollObject()->getMaxOffset());
	m_fullDownButton->setFlashing(m_promptBox->getScrollObject()->getScrollOffset() != 0);
	
	// Limitation/flaw of design of chat box, we can only put links at front of the chat
	if (m_linkedItem.m_itemId != 0 && m_promptBox->isCurrentPrompt())
	{
		if (m_linkedItemNameCache.empty())
			m_linkedItemNameCache = "[" + ItemIcon::formItemTitle(m_linkedItem) + "]";

		m_promptBox->setPrefix(getChannelPrefix(m_channel, m_whisperTarget) + m_linkedItemNameCache);
	}
	else 
	{
		if (m_linkedItem.m_itemId != 0)
		{
			m_linkedItemNameCache.clear();
			m_linkedItem = {};
		}

		m_promptBox->setPrefix(getChannelPrefix(m_channel, m_whisperTarget));
	}
		
	// If the prompt box isn't in use then the enter button won't receive input, but we want it to because for GameChat it's an entry and exit point into using the chat box
	if (!m_promptBox->isCurrentPrompt())
	{
		m_promptEnterButton->attemptInput();

		if (m_promptEnterButton->popActivated())
		{
			setInUse(true);
		}
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Slash))
		{ 
			setInUse(true);
			m_promptBox->setTextPrompt("/");
		}
	}
	else
	{
		string enteredTxt = m_promptBox->popQueuedStatement();
		bool emptyEnter = false;
		
		// The enter key is also for exiting the chat box when there's nothing there, only pop if we have m_linkedItemNameCache to fill the voidf
		if (!m_linkedItemNameCache.empty())
			emptyEnter = m_promptBox->popEmptyEnter();

		if (!enteredTxt.empty() || emptyEnter)
		{
			enteredTxt = m_linkedItemNameCache + enteredTxt;

			if (parseChannelSwap(enteredTxt, false, string{}))
			{

			}
			else if (enteredTxt[0] == '/')
			{
				if (!processServerCommand(enteredTxt))
					printHelp();
			}
			else
			{
				GP_Client_ChatMsg packet;
				packet.m_channelId = m_channel;
				packet.m_text = enteredTxt;
				packet.m_targetName = m_whisperTarget;
				
				if (m_linkedItem.m_itemId != 0)
					packet.m_itemId = m_linkedItem;

				sConnector->sendPacket(packet.build(StlBuffer{}));
			}

			// After you yell, go back to what you were doing before, etc
			switch (m_channel)
			{
				case ChatDefines::Channels::Say:
				case ChatDefines::Channels::Party:
				case ChatDefines::Channels::Guild:
				{
					m_channelPrevious = m_channel;
					break;
				}
				case ChatDefines::Channels::AllChat:
				case ChatDefines::Channels::Yell:
				{
					if (m_channelPrevious == ChatDefines::Channels::Whisper)
						setChannel(ChatDefines::Channels::Say);
					else
						setChannel(m_channelPrevious);
					break;
				}
				case ChatDefines::Channels::Whisper:
				{
					if (!m_whisperTarget.empty())
					{
						const string toWhisperStr = "To [" + m_whisperTarget + "]: " + enteredTxt;
						registerFromSlot(m_whisperTarget, toWhisperStr);
						addLine(toWhisperStr, ChatDefines::Channels::Whisper);
					}

					break;
				}
			}

			if (m_channel != ChatDefines::Channels::Whisper)
				m_whisperTarget.clear();

			setInUse(false);
		}
		else if (sApplication->popKeyUp(sf::Keyboard::Escape))
		{
			setInUse(false);
		}
		else
		{
			string extractedStr;
			parseChannelSwap(m_promptBox->getContent(), true, extractedStr);
		}
	}
	
	const string rightClickedLine = m_promptBox->popPendingClickedLine();

	if (!rightClickedLine.empty())
	{
		const string msgSenderName = getMsgSenderName(rightClickedLine);

		if (!msgSenderName.empty())
		{
			if (auto owner = dynamic_cast<World*>(getOwner()))
			{
				m_rightClickedName = msgSenderName;
				sContentMgr->playSound(SfxId::ButtonClick);
				setInUse(false);

				owner->registerContextMenu(World::Interface::CtxMenuCurrent, getId(), sApplication->mousePos(),
					{
						ctxMenuOptionStr(CtxMenuOptions::DdoWhisper),
						ctxMenuOptionStr(CtxMenuOptions::DdoInvite),
						ctxMenuOptionStr(CtxMenuOptions::DdoCancel),
					});
			}
		}
	}

	if (m_promptBox->getHoveringLineId() != -1 && sApplication->mouseUp(sf::Mouse::Left))
	{
		auto ptr = m_promptBox->getLine(m_promptBox->getHoveringLineId());
		auto linkedItem = m_itemLinks.find(reinterpret_cast<uintptr_t>(ptr));

		if (linkedItem != m_itemLinks.end())
		{
			sContentMgr->playSound(SfxId::ButtonClick);
			m_clickedLinkedItem = linkedItem->second;
		}
	}

	if (m_clickedLinkedItem != nullptr)
	{
		m_closeLinkTooltipBtn->attemptInput();

		if (m_closeLinkTooltipBtn->popActivated())
		{
			sContentMgr->playSound(SfxId::WindowTargetClose);
			m_clickedLinkedItem = nullptr;
		}
	}
}

void GameChat::render()
{
	if (m_promptBox->isCurrentPrompt())
		m_bgSprite->getRaw()->setColor(sf::Color(255, 255, 255, 255));
	else
		m_bgSprite->getRaw()->setColor(sf::Color(255, 255, 255, 225));

	m_promptEnterButton->setExclaimNotice(sApplication->getCurrentPromptInput() == reinterpret_cast<uintptr_t>(m_promptBox.get()));

	__super::render();

	auto drawLinkedItem = [&](ItemIcon& icon)
	{
		icon.popTooltipRefresh();
		icon.getTopLeftCornerRef() = getTopLeftCornerRef() + sf::Vector2i{ 515, -(40 + icon.getTooltip()->getHeight()) };
		icon.updateBottomRightCorner(icon.getTopLeftCornerRef() + sf::Vector2i{ 1, 1 });
		icon.updateTooltipPosition();
		icon.getTooltip()->draw();	
	};

	if (m_clickedLinkedItem == nullptr)
	{
		if (m_promptBox->getHoveringLineId() != -1)
		{
			auto ptr = m_promptBox->getLine(m_promptBox->getHoveringLineId());
			auto linkedItem = m_itemLinks.find(reinterpret_cast<uintptr_t>(ptr));

			if (linkedItem != m_itemLinks.end())
				drawLinkedItem(*linkedItem->second);
		}
	}
	else
	{
		drawLinkedItem(*m_clickedLinkedItem);
		m_closeLinkTooltipBtn->getTopLeftCornerRef() = m_clickedLinkedItem->getTooltip()->getTopLeftCornerRef() + sf::Vector2i{ m_clickedLinkedItem->getTooltip()->mouseableWidth() - m_closeLinkTooltipBtn->mouseableWidth(), 0 } + sf::Vector2i{ (m_closeLinkTooltipBtn->mouseableWidth() / 2) - 5, (-m_closeLinkTooltipBtn->mouseableHeight() / 2) + 5 };
		m_closeLinkTooltipBtn->attemptRender();
	}
}
		
bool GameChat::parseChannelSwap(const string& inputstr, const bool extraSpace, string& out_extractedStr)
{
	string buffer = extraSpace ? " " : "";

	// Set to guild chat
	if (inputstr == "/g" + buffer || inputstr == "/guild" + buffer)
	{
		m_promptBox->setContent("");
		setChannel(ChatDefines::Channels::Guild);
		return true;
	}

	// Set to party chat
	else if (inputstr == "/p" + buffer || inputstr == "/party" + buffer)
	{
		m_promptBox->setContent("");
		setChannel(ChatDefines::Channels::Party);
		return true;
	}

	// Set to say chat
	else if (inputstr == "/s" + buffer || inputstr == "/say" + buffer)
	{
		m_promptBox->setContent("");
		setChannel(ChatDefines::Channels::Say);
		return true;
	}

	// Set to all chat
	else if (inputstr == "/a" + buffer || inputstr == "/all" + buffer)
	{
		m_promptBox->setContent("");
		setChannel(ChatDefines::Channels::AllChat);
		return true;
	}

	// Set to yell chat
	else if (inputstr == "/y" + buffer || inputstr == "/yell" + buffer)
	{
		m_promptBox->setContent("");
		setChannel(ChatDefines::Channels::Yell);
		return true;
	}

	// Set to whisper chat
	else if (hasAndExtract("/w" + buffer, out_extractedStr) || hasAndExtract("/whisper" + buffer, out_extractedStr))
	{
		m_whisperTarget = CharacterFunctions::formatName(out_extractedStr);
		setChannel(ChatDefines::Channels::Whisper);
		m_promptBox->setContent("");
		return true;
	}

	return false;
}

void GameChat::recvMsg(const string& msg, const string& from, const ChatDefines::Channels channel, const ItemDefines::ItemDefinition* linkedItem /*= nullptr*/)
{
	string formattedMsg;

	switch (channel)
	{
		case ChatDefines::Channels::Say: formattedMsg = "[" + from + "]" + " says: " + msg; break;
		case ChatDefines::Channels::Yell: formattedMsg = "[" + from + "]" + " yells: " + msg; break;
		case ChatDefines::Channels::Guild: from.empty() ? formattedMsg = msg :  formattedMsg = "[G] [" + from + "]" + ": " + msg; break;
		case ChatDefines::Channels::Whisper: formattedMsg = "[" + from + "] whispers: " + msg; break;
		case ChatDefines::Channels::Party: formattedMsg = "[P] [" + from + "]" + ": " + msg; break;
		case ChatDefines::Channels::AllChat: formattedMsg = "[A] [" + from + "]" + ": " + msg; break;
		case ChatDefines::Channels::System: formattedMsg = msg; break;
		case ChatDefines::Channels::SystemCenter: formattedMsg = msg; break;
		case ChatDefines::Channels::ExpPurple: formattedMsg = msg; break;
		case ChatDefines::Channels::RedWarning: formattedMsg = msg; break;
		default: return;
	}

	registerFromSlot(from, formattedMsg);
	addLine(formattedMsg, channel, linkedItem);
}

void GameChat::addLineColor(const string& msg, const sf::Color color, const ItemDefines::ItemDefinition* linkedItem /*= nullptr*/)
{
	uintptr_t prunedPtr = 0;
	m_promptBox->addLine(msg, color, &prunedPtr);

	if (prunedPtr != 0)
		m_itemLinks.erase(prunedPtr);

	if (linkedItem != nullptr)
	{
		int index = m_promptBox->getNumLines() - 1;
		uintptr_t ptrval = reinterpret_cast<uintptr_t>(m_promptBox->getLine(index));

		auto fakeIcon = make_shared<ItemIcon>(*this, 0, "gameicon19");		
		fakeIcon->setItemDef(*linkedItem);
		fakeIcon->refreshTooltip();
		fakeIcon->setTooltipHorizontalAdju(false);

		m_itemLinks[ptrval] = fakeIcon;
	}

	if (sApplication->isDevMode())
	{
		if (auto consoleWindow = dynamic_pointer_cast<ConsoleWindow>(sApplication->getRenderObject(Application::RoConsole)))
			consoleWindow->warning(msg);
	}
}

void GameChat::addLine(const string& msg, const ChatDefines::Channels channel, const ItemDefines::ItemDefinition* linkedItem /*= nullptr*/)
{
	addLineColor(msg, getChatColor(channel), linkedItem);
}

void GameChat::promptLinkAnItem(const ItemDefines::ItemDefinition& itemDef)
{
	setInUse(true);
	m_linkedItem = itemDef;
	m_linkedItemNameCache.clear();
	m_promptBox->setContent("");
}

void GameChat::registerFromSlot(const string& from, const string& msg)
{
	if (m_promptBox->getNumLines() + 1 > TextLines::Defines::DialogWindowMaxLines)
	{
		auto itr = m_msgNames.find(m_promptBox->getLine(0)->getTextStr());

		if (itr != m_msgNames.end())
			m_msgNames.erase(itr);
	}

	if (!from.empty())
		m_msgNames[msg] = from;
}

string GameChat::getChannelPrefix(const ChatDefines::Channels channel, const string& whisperName /*= ""*/)
{
	switch (channel)
	{
		case ChatDefines::Channels::Say: return "Say: ";
		case ChatDefines::Channels::Yell: return "Yell: ";
		case ChatDefines::Channels::Guild: return "Guild: ";
		case ChatDefines::Channels::Whisper: return "Tell " + whisperName + ": ";
		case ChatDefines::Channels::Party: return "Party: ";
		case ChatDefines::Channels::AllChat: return "All: ";
	}

	return "";
}

void GameChat::setInUse(const bool v)
{
	m_inUse = v;

	if (RenderObjectHolder* owner = dynamic_cast<RenderObjectHolder*>(getOwner()))
	{
		//owner->setMultiInput(!v);

		if (v)
			owner->requestMoveToTop(getId());
		else
			m_promptBox->setContent("");
	}
}

void GameChat::setChannel(const ChatDefines::Channels c)
{
	m_channel = c;
	m_promptBox->setColor(getChatColor(c));
}

void GameChat::printHelp()
{
	vector<string> msgs;
	Util::readLinesFromFile("scripts\\text\\help.txt", msgs);

	for (auto& itr : msgs)
		recvMsg(itr, "", ChatDefines::Channels::System);

	m_promptBox->getScrollObject()->setScrollOffset(0);
}

bool GameChat::processServerCommand(string enteredTxt)
{
	if (enteredTxt.find("/reloadui") == 0 || enteredTxt.find("/reload") == 0)
	{
		sLua->requestReload();
		recvMsg("Reloading addons...", "", ChatDefines::Channels::System);
		return true;
	}

	if (enteredTxt.find("/mall") == 0)
	{
		m_muteAllChate = true;
		recvMsg("All chat muted.", "", ChatDefines::Channels::System);
		return true;
	}

	if (enteredTxt.find("/umall") == 0)
	{
		m_muteAllChate = false;
		recvMsg("All chat un-muted.", "", ChatDefines::Channels::System);
		return true;
	}

	if (enteredTxt.find("/invite ") == 0)
	{
		Util::strReplaceAll(enteredTxt, "/invite ", "");
		
		GP_Client_PartyInviteMember pk;
		pk.m_playerName = CharacterFunctions::formatName(enteredTxt);
		sConnector->sendPacket(pk.build(StlBuffer{}));
		return true;
	}

	if (enteredTxt.find("/roll") == 0)
	{
		GP_Client_RollDice pk;
		sConnector->sendPacket(pk.build(StlBuffer{}));
		return true;
	}

	if (enteredTxt.find("/pvp") == 0)
	{
		GP_Client_TogglePvP pk;
		sConnector->sendPacket(pk.build(StlBuffer{}));
		return true;
	}

	if (enteredTxt.find("/gcreate ") == 0)
	{
		Util::strReplaceAll(enteredTxt, "/gcreate ", "");
		const auto nameErrorId = GuildFunctions::invalidNameError(enteredTxt);

		if (nameErrorId == GuildDefines::NameError::Success)
		{
			GP_Client_GuildCreate pk;
			pk.m_guildName = enteredTxt;
			sConnector->sendPacket(pk.build(StlBuffer{}));
		}
		else
		{
			recvMsg(GuildFunctions::nameErrorStr(nameErrorId), "", ChatDefines::Channels::System);
		}

		return true;
	}

	if (enteredTxt.find("/ginvite ") == 0)
	{
		Util::strReplaceAll(enteredTxt, "/ginvite ", "");
		enteredTxt = CharacterFunctions::formatName(enteredTxt);

		GP_Client_GuildInviteMember pk;
		pk.m_playerName = enteredTxt;
		sConnector->sendPacket(pk.build(StlBuffer{}));
		return true;
	}

	if (enteredTxt.find("/gkick ") == 0)
	{
		Util::strReplaceAll(enteredTxt, "/gkick ", "");
		enteredTxt = CharacterFunctions::formatName(enteredTxt);

		if (auto owner = dynamic_cast<World*>(getOwner()))
		{
			if (auto guildRoster = dynamic_pointer_cast<GuildRoster>(owner->getRenderObject(World::Interface::GuildPanel)))
			{
				if (auto memberData = guildRoster->getMember(enteredTxt))
				{
					GP_Client_GuildKickMember pk;
					pk.m_targetGuid = memberData->guid;
					sConnector->sendPacket(pk.build(StlBuffer{}));
				}
				else
				{
					recvMsg("Player not found.", "", ChatDefines::Channels::System);
				}
			}
		}

		return true;
	}

	if (enteredTxt.find("/gquit") == 0)
	{
		GP_Client_GuildQuit pk;
		sConnector->sendPacket(pk.build(StlBuffer{}));
		return true;
	}

	if (enteredTxt.find("/gdisband") == 0)
	{
		GP_Client_GuildDisband pk;
		sConnector->sendPacket(pk.build(StlBuffer{}));
		return true;
	}

	if (enteredTxt.find("/yield") == 0)
	{
		GP_Client_YieldDuel pk;
		sConnector->sendPacket(pk.build(StlBuffer{}));
		return true;
	}

	if (enteredTxt.find("/duel") == 0)
	{
		if (auto owner = dynamic_cast<World*>(getOwner()))
		{
				GP_Client_CastSpell packet;
				packet.m_targetGuid = owner->getSelectedGuid();
				packet.m_spellId = SpellDefines::StaticSpells::OfferDuel;
				sConnector->sendPacket(packet.build(StlBuffer{}));
		}

		return true;
	}

	if (enteredTxt.find("/ignore") == 0)
	{
		Util::strReplaceAll(enteredTxt, "/ignore ", "");
		enteredTxt = CharacterFunctions::formatName(enteredTxt);

		GP_Client_SetIgnorePlayer pk;
		pk.m_ignore = true;
		pk.m_name = enteredTxt;
		sConnector->sendPacket(pk.build(StlBuffer{}));
		return true;
	}

	if (enteredTxt.find("/unignore") == 0)
	{
		Util::strReplaceAll(enteredTxt, "/unignore ", "");
		enteredTxt = CharacterFunctions::formatName(enteredTxt);

		GP_Client_SetIgnorePlayer pk;
		pk.m_ignore = false;
		pk.m_name = enteredTxt;
		sConnector->sendPacket(pk.build(StlBuffer{}));
		return true;
	}

	if (enteredTxt.find("/report") == 0)
	{
		Util::strReplaceAll(enteredTxt, "/report ", "");
		enteredTxt = CharacterFunctions::formatName(enteredTxt);

		if (enteredTxt.empty() || enteredTxt == "/report")
		{
			if (auto world = dynamic_cast<World*>(getOwner()))
			{
				if (auto selectedUnit = world->getClientObject(world->getSelectedGuid()))
					enteredTxt = selectedUnit->getName();
			}
		}

		GP_Client_ReportPlayer pk;
		pk.m_name = enteredTxt;
		sConnector->sendPacket(pk.build(StlBuffer{}));
		return true;
	}

	if (auto owner = dynamic_cast<World*>(getOwner()))
	{
		if (owner->myself() != nullptr && owner->myself()->getVariable(ObjDefines::Variable::Moderator) != 0)
		{
			if (enteredTxt.find("/warn") == 0)
			{
				Util::strReplaceAll(enteredTxt, "/warn ", "");
				enteredTxt = CharacterFunctions::formatName(enteredTxt);
				GP_Client_MOD_WarnPlayer pk;
				pk.m_name = enteredTxt;
				sConnector->sendPacket(pk.build(StlBuffer{}));
				return true;
			}

			if (enteredTxt.find("/mute") == 0)
			{
				Util::strReplaceAll(enteredTxt, "/mute ", "");
				enteredTxt = CharacterFunctions::formatName(enteredTxt);
				GP_Client_MOD_MutePlayer pk;
				pk.m_name = enteredTxt;
				sConnector->sendPacket(pk.build(StlBuffer{}));
				return true;
			}

			if (enteredTxt.find("/kick") == 0)
			{
				Util::strReplaceAll(enteredTxt, "/kick ", "");
				enteredTxt = CharacterFunctions::formatName(enteredTxt);
				GP_Client_MOD_KickPlayer pk;
				pk.m_name = enteredTxt;
				sConnector->sendPacket(pk.build(StlBuffer{}));
				return true;
			}

			if (enteredTxt.find("/ban") == 0)
			{
				Util::strReplaceAll(enteredTxt, "/ban ", "");
				enteredTxt = CharacterFunctions::formatName(enteredTxt);
				GP_Client_MOD_BanPlayer pk;
				pk.m_name = enteredTxt;
				sConnector->sendPacket(pk.build(StlBuffer{}));
				return true;
			}
		}
	}
	
	return false;
}

bool GameChat::hasAndExtract(const string& prefx, string& output) const
{
	if (m_promptBox->getContent().find(prefx) != 0)
		return false;

	output.clear();

	if (m_promptBox->getContent().size() >= prefx.size() + 2 && m_promptBox->getContent()[m_promptBox->getContent().size() - 1] == ' ')
	{
		for (size_t i = prefx.size(); i < m_promptBox->getContent().size() - 1; ++i)
			output.push_back(m_promptBox->getContent()[i]);
	}

	return !output.empty();
}

string GameChat::getMsgSenderName(const string& msg) const
{
	auto itr = m_msgNames.find(msg);

	if (itr != m_msgNames.end())
		return itr->second;

	return "";
}

void GameChat::notifyCtxMenuClicked(const int id, const string& lineClicked) /*final*/
{
	if (id != World::Interface::CtxMenuCurrent)
		return;

	if (lineClicked == ctxMenuOptionStr(CtxMenuOptions::DdoWhisper))
	{
		setWhispering(m_rightClickedName);
	}
	else if (lineClicked == ctxMenuOptionStr(CtxMenuOptions::DdoInvite))
	{
		GP_Client_PartyInviteMember pk;
		pk.m_playerName = m_rightClickedName;
		sConnector->sendPacket(pk.build(StlBuffer{}));
	}
	
	m_rightClickedName.clear();
}

void GameChat::setWhispering(const string& name)
{
	m_whisperTarget = name;
	m_promptBox->setContent("");
	setChannel(ChatDefines::Channels::Whisper);
	setInUse(true);
}

/*static*/
string GameChat::ctxMenuOptionStr(const CtxMenuOptions id)
{
	switch (id)
	{
		case CtxMenuOptions::DdoWhisper: return "Whisper";
		case CtxMenuOptions::DdoInvite: return "Invite";
		case CtxMenuOptions::DdoCancel: return "Cancel";
	}

	return "";
}

sf::Color GameChat::getChatColor(const ChatDefines::Channels c)
{
	switch (c)
	{
		case ChatDefines::Channels::Say: return sf::Color(0xE7EAEFFF);
		case ChatDefines::Channels::Yell: return sf::Color(0xDB5D20FF);
		case ChatDefines::Channels::Whisper: return sf::Color(0xD853D1FF);
		case ChatDefines::Channels::Guild: return sf::Color(0x8AD910FF);
		case ChatDefines::Channels::Party: return sf::Color(0x569cd6FF);
		case ChatDefines::Channels::System: return sf::Color(0xffd700FF);
		case ChatDefines::Channels::SystemCenter: return getChatColor(ChatDefines::Channels::System);
		case ChatDefines::Channels::AllChat: return sf::Color(0x9fefe7FF);
		case ChatDefines::Channels::ExpPurple: return sf::Color(88, 88, 185, 255);
		case ChatDefines::Channels::RedWarning: return sf::Color::Red;
	}

	return sf::Color::White;
}