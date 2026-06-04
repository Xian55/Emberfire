#include "stdafx.h"
#include "GuildRoster.h"
#include "ContentMgr.h"
#include "TextBoxRo.h"
#include "Button.h"
#include "TextLines.h"
#include "ScrollBar.h"
#include "TickBox.h"
#include "TextBoxRo.h"
#include "Application.h"
#include "PopupPrompt.h"
#include "Connector.h"
#include "World.h"
#include "GameChat.h"
#include "ConfirmMessageBox.h"

#include "..\Rand.h"
#include "..\Shared\GamePacketClient.h"

#include <assert.h>

GuildRoster::GuildRoster(World& owner, const int id) :
	WorldPanel(owner, id, "guildroster_empty.png", sf::Vector2i(392, 28)),
	m_showOffline(true),
	m_numOnline(0)
{
	setGuild("debug");

	for (int i = 1; i < 100; ++i)
	{
		addMember(i, i == 1 ? "Gum" : "Example" + to_string(i), Util::randomChoice(PlayerDefines::Mage, PlayerDefines::Bishop, PlayerDefines::Paladin, PlayerDefines::Ranger),
			i == 1 ? GuildDefines::Leader : GuildDefines::Officer, i, Util::randomChoice(true, false));
	}
}

GuildRoster::~GuildRoster()
{

}

void GuildRoster::input()
{
	__super::input();

	switch (popFirstButtonId())
	{
		// Start editing motd
		case Interface::EditMotdButton:
		{
			if (isLeader())
				sApplication->spawnPopupPrompt("Set the Message of the Day", PopupPrompt::Codes::SetGuildMotd);

			break;
		}
	}

	// Finish editing motd
	if (auto promptPopup = sApplication->popPopupPrompt({ PopupPrompt::Codes::SetGuildMotd }))
	{
		if (promptPopup->isAccepted())
		{
			GP_Client_GuildMotd packet;
			packet.m_motd = promptPopup->getContent();
			sConnector->sendPacket(packet.build(StlBuffer{}));
		}
	}

	// Toggling to show offline players
	if (auto showOfflineTick = dynamic_pointer_cast<TickBox>(getRenderObject(Interface::ShowOfflineTick)))
	{
		if (showOfflineTick->ticked() != m_showOffline)
			setShowOffline(showOfflineTick->ticked());
	}

	// When they click on a name, try to whisper
	if (auto listNames = dynamic_pointer_cast<TextLines>(getRenderObject(Interface::ListNames)))
	{
		// Messes with mouse over logic for the scroll object so that if we mouse over any part of the window we can scroll wheel it
		listNames->getScrollObject()->getTopLeftCornerRef() = getTopLeftCornerRef();

		const string clickedName = listNames->popPendingClickedLine();

		// todo:
		if (!clickedName.empty())
		{
			auto owner = dynamic_cast<World*>(getOwner());
			auto guildMember = getMember(clickedName);

			if (guildMember != nullptr && owner != nullptr)
			{
				vector<string> lines;

				if (guildMember->online)
				{
					lines.push_back(ctxMenuOptionStr(CtxMenuOptions::DdoWhisper));
					lines.push_back(ctxMenuOptionStr(CtxMenuOptions::DdoInvite));
				}

				if (GuildFunctions::hasOfficerPowerOver(getLocalRank(), guildMember->rank))
				{
					lines.push_back(ctxMenuOptionStr(CtxMenuOptions::DdoPromote));

					// Can't go lower than Initiate
					if (guildMember->rank > GuildDefines::Rank::Initiate)
						lines.push_back(ctxMenuOptionStr(CtxMenuOptions::DdoDemote));

					lines.push_back(ctxMenuOptionStr(CtxMenuOptions::DdoKick));
				}

				if (!lines.empty())
				{
					lines.push_back(ctxMenuOptionStr(CtxMenuOptions::DdoCancel));
					owner->registerContextMenu(World::Interface::CtxMenuCurrent, getId(), sApplication->mousePos(), lines);
					sContentMgr->playSound(SfxId::ButtonClick);
					m_rightClickedName = clickedName;
				}
			}
		}
	}
	
	if (auto confirmBox = sApplication->popConfirmBox({ ConfirmMessageBox::ConfirmCode_GkickSomeone, 
		ConfirmMessageBox::ConfirmCode_PromoteSomeone, ConfirmMessageBox::ConfirmCode_DemoteSomeone }))
	{
		if (confirmBox->getResult() == ConfirmMessageBox::ConfirmBox_Yes)
		{
			if (confirmBox->getCode() == ConfirmMessageBox::ConfirmCode_GkickSomeone)
			{
				if (auto memberData = getMember(m_rightClickedName))
				{
					GP_Client_GuildKickMember packet;
					packet.m_targetGuid = memberData->guid;
					sConnector->sendPacket(packet.build(StlBuffer{}));
				}
			}
			else if (confirmBox->getCode() == ConfirmMessageBox::ConfirmCode_PromoteSomeone)
			{
				if (auto memberData = getMember(m_rightClickedName))
				{
					GP_Client_GuildPromoteMember packet;
					packet.m_targetGuid = memberData->guid;
					sConnector->sendPacket(packet.build(StlBuffer{}));
				}
			}
			else if (confirmBox->getCode() == ConfirmMessageBox::ConfirmCode_DemoteSomeone)
			{
				if (auto memberData = getMember(m_rightClickedName))
				{
					GP_Client_GuildDemoteMember packet;
					packet.m_targetGuid = memberData->guid;
					sConnector->sendPacket(packet.build(StlBuffer{}));
				}
			}
		}

		m_rightClickedName.clear();
	}
}

void GuildRoster::render()
{
	// Sync the list of levels/classes with the list of names, before rendering of course
	if (auto listNames = dynamic_pointer_cast<TextLines>(getRenderObject(Interface::ListNames)))
	{
		auto listLevels = dynamic_pointer_cast<TextLines>(getRenderObject(Interface::ListLevels));
		ASSERT(listLevels != nullptr);

		auto listClasses = dynamic_pointer_cast<TextLines>(getRenderObject(Interface::ListClasses));
		ASSERT(listClasses != nullptr);

		listLevels->setExtraScrollOfset(listNames->getScrollOffset());
		listClasses->setExtraScrollOfset(listNames->getScrollOffset());
	}

	__super::render();
}

void GuildRoster::setNumOnline(const int v)
{
	m_numOnline = v;

	if (auto text = dynamic_pointer_cast<TextBoxRo>(getRenderObject(Interface::OnlineCount)))
		text->setString(to_string(m_numOnline));
}

void GuildRoster::unsetGuild()
{
	alterBackground("guildroster_empty.png");
	destroyObjectById(Interface::GuildTitle);
}

void GuildRoster::setGuild(const string& guildname)
{
	if (m_guildName == guildname)
		return;

	m_guildName = guildname;

	alterBackground("guildroster.png");

	// todo: where to put this?
	auto title = make_shared<TextBoxRo>(*this, Interface::GuildTitle, FontId::Fontin, 100, 12, TextBox::Align::AlignCenterBounds, m_showOffline, 1.0f);
	title->setHidden(true);
	addRenderObject(title);

	// Message of the day
	auto motd = make_shared<TextBoxRo>(*this, Interface::GuildMotd, FontId::Helvetica, 240, 16);
	motd->setString("No message currently set.");
	motd->setAnchor(&getTopLeftCornerRef());
	motd->setOffset({ 40, 475 });
	addRenderObject(motd);

	// Number of people online
	auto onlineCount = make_shared<TextBoxRo>(*this, Interface::OnlineCount, FontId::Ringbearer, 100, 20);
	onlineCount->setString(to_string(m_numOnline));
	onlineCount->setAnchor(&getTopLeftCornerRef());
	onlineCount->setOffset({ 311, 427 });
	addRenderObject(onlineCount);

	// Roster
	const int rosterY = 130;
	auto listNames = make_shared<TextLines>(*this, Interface::ListNames, FontId::Helvetica, Util::GeoBox2d(0, 0, 125, 285), 18);
	listNames->setAnchor(&getTopLeftCornerRef());
	listNames->setOffset({ 35, rosterY });
	listNames->setClickableLines(true);
	listNames->setClickableMouseButton(sf::Mouse::Right);
	addRenderObject(listNames);

	auto listLevels = make_shared<TextLines>(*this, Interface::ListLevels, FontId::Helvetica, Util::GeoBox2d(0, 0, 125, 285), 18);
	listLevels->setAnchor(&getTopLeftCornerRef());
	listLevels->setOffset({ 183, rosterY });
	addRenderObject(listLevels);

	auto listClasses = make_shared<TextLines>(*this, Interface::ListClasses, FontId::Helvetica, Util::GeoBox2d(0, 0, 125, 285), 18);
	listClasses->setAnchor(&getTopLeftCornerRef());
	listClasses->setOffset({ 301, rosterY });
	addRenderObject(listClasses);

	// Given to the roster list
	auto scrollBar = make_shared<ScrollBar>(*listNames, "scrollbar_thin_blank", "scrollbar_thin_blank", ScrollBar::ScrollTopDown, "scrollbar_thin");
	scrollBar->getScrollUpButton()->setOffset({ 395, 120 });
	scrollBar->getScrollUpButton()->setAnchor(&getTopLeftCornerRef());
	scrollBar->getScrollDownButton()->setOffset({ 395, 365 });
	scrollBar->getScrollDownButton()->setAnchor(&getTopLeftCornerRef());
	scrollBar->getScrollSquare()->setOffset({ 395, 365 });
	scrollBar->setAllowMousewheelOnlyMousedOver(true);
	listNames->setScrollObject(scrollBar);

	// Toggle showing offline members or not
	auto offlineTick = make_shared<TickBox>(*this, Interface::ShowOfflineTick, "tick_box_empty", "tick_box_full", sf::Vector2i(0, 0), true);
	offlineTick->setAnchor(&getTopLeftCornerRef());
	offlineTick->setOffset({ 155, 425 });
	addRenderObject(offlineTick);

	// Edit MOTD
	auto editMotdButton = make_shared<Button>(*this, "guild_motd_emptybutton", Interface::EditMotdButton);
	editMotdButton->setAnchor(&getTopLeftCornerRef());
	editMotdButton->setOffset({ 41, 479 });
	addRenderObject(editMotdButton);
}

void GuildRoster::setMotd(const string& str)
{
	if (auto motd = dynamic_pointer_cast<TextBoxRo>(getRenderObject(Interface::GuildMotd)))
		motd->setString(str);
	else
		blog(Logger::LOG_ERROR, "Tried to set guild motd but did not find object to do so");
}

void GuildRoster::addMember(const int guid, const string& name, const PlayerDefines::Classes classId, const GuildDefines::Rank rank, 
	const int level, const bool online /*= true*/)
{
	// No duplicates
	if (hasMember(name))
		return;

	if (online)
		setNumOnline(getNumOnline() + 1);

	GuildMember member;
	member.guid = guid;
	member.classId = classId;
	member.rank = rank;
	member.level = level;
	member.online = online;

	// Cache
	m_members[name] = member;

	// Don't add to render objects if we're not displaying offline members
	if (m_showOffline || online)
		addMemberToDrawLists(m_members.find(name));
}

void GuildRoster::onOpen() /*final*/
{
	GP_Client_GuildRosterRequest pk;
	sConnector->sendPacket(pk.build(StlBuffer{}));
}

void GuildRoster::notifyCtxMenuClicked(const int id, const string& lineClicked) /*final*/
{
	if (lineClicked == ctxMenuOptionStr(CtxMenuOptions::DdoWhisper))
	{
		if (auto owner = dynamic_cast<World*>(getOwner()))
		{
			if (auto gameChat = dynamic_pointer_cast<GameChat>(owner->getRenderObject(World::Interface::GameChatBox)))
				gameChat->setWhispering(m_rightClickedName);
		}
	}
	else if (lineClicked == ctxMenuOptionStr(CtxMenuOptions::DdoInvite))
	{
		GP_Client_PartyInviteMember pk;
		pk.m_playerName = m_rightClickedName;
		sConnector->sendPacket(pk.build(StlBuffer{}));
	}
	else if (lineClicked == ctxMenuOptionStr(CtxMenuOptions::DdoDemote))
	{
		if (auto member = getMember(m_rightClickedName))
		{
			sApplication->spawnPopup("Demote " + m_rightClickedName + " to " + GuildFunctions::rankName(static_cast<GuildDefines::Rank>(member->rank - 1)) +
				"?", ConfirmMessageBox::ConfirmBox_Yes, ConfirmMessageBox::ConfirmCode_DemoteSomeone);
			return;
		}
	}
	else if (lineClicked == ctxMenuOptionStr(CtxMenuOptions::DdoKick))
	{
		sApplication->spawnPopup("Kick " + m_rightClickedName + "?", ConfirmMessageBox::ConfirmBox_Yes, ConfirmMessageBox::ConfirmCode_GkickSomeone);
		return;
	}
	else if (lineClicked == ctxMenuOptionStr(CtxMenuOptions::DdoPromote))
	{
		if (auto member = getMember(m_rightClickedName))
		{
			if (getLocalRank() == GuildDefines::Rank::Leader && member->rank == GuildDefines::Rank::Officer)
			{
				sApplication->spawnPopup("WARNING: Are you sure you want to transfer leadership to " + m_rightClickedName + "?",
					ConfirmMessageBox::ConfirmBox_Yes, ConfirmMessageBox::ConfirmCode_PromoteSomeone);
			}
			else
			{
				sApplication->spawnPopup("Promote " + m_rightClickedName + " to " + GuildFunctions::rankName(static_cast<GuildDefines::Rank>(member->rank + 1)) +
					"?", ConfirmMessageBox::ConfirmBox_Yes, ConfirmMessageBox::ConfirmCode_PromoteSomeone);
			}
			return;
		}
	}

	m_rightClickedName.clear();
}

GuildRoster::GuildMember const* GuildRoster::getMember(const string& name) const
{
	auto itr = m_members.find(name);

	if (itr == m_members.end())
		return nullptr;

	return &itr->second;
}

GuildDefines::Rank GuildRoster::getLocalRank() const
{
	if (auto owner = dynamic_cast<World*>(getOwner()))
	{
		if (auto myself = getMember(owner->getMyselfName()))
			return myself->rank;
	}

	return GuildDefines::Rank::Leader;
}

void GuildRoster::addMemberToDrawLists(const unordered_map<string, GuildMember>::iterator& itr)
{
	ASSERT(itr != m_members.end());

	auto listNames = dynamic_pointer_cast<TextLines>(getRenderObject(Interface::ListNames));
	ASSERT(listNames != nullptr);

	auto listLevels = dynamic_pointer_cast<TextLines>(getRenderObject(Interface::ListLevels));
	ASSERT(listLevels != nullptr);

	auto listClasses = dynamic_pointer_cast<TextLines>(getRenderObject(Interface::ListClasses));
	ASSERT(listClasses != nullptr);

	sf::Color nameColor(0xdfa802ff);
	sf::Color levelColor(sf::Color::White);
	sf::Color classColor(PlayerFunctions::classColor(itr->second.classId));

	if (!itr->second.online)
	{
		nameColor.a /= 2;
		levelColor.a /= 2;
		classColor.a /= 2;
	}

	// Name
	listNames->addLine(itr->first, nameColor);
	listNames->getLine(listNames->getNumLines() - 1)->setTooltipStr(GuildFunctions::rankName(itr->second.rank));

	// Level
	listLevels->addLine("lv. " + to_string(itr->second.level), levelColor);

	// Class
	listClasses->addLine(PlayerFunctions::className(itr->second.classId), classColor);
}

/*static*/
string GuildRoster::ctxMenuOptionStr(const CtxMenuOptions id)
{
	switch (id)
	{
		case CtxMenuOptions::DdoWhisper: return "Whisper";
		case CtxMenuOptions::DdoInvite: return "Invite";
		case CtxMenuOptions::DdoCancel: return "Cancel";
		case CtxMenuOptions::DdoPromote: return "Promote";
		case CtxMenuOptions::DdoDemote: return "Demote";
		case CtxMenuOptions::DdoKick: return "Kick";
	}

	return "";
}

void GuildRoster::setShowOffline(const bool v)
{
	if (m_showOffline == v)
		return;

	m_showOffline = v;

	// Possible we don't have a guild but something from the outside is telling us to set this variable for future use
	if (isGuildSet())
		redrawRoster();
}

// I think we can get away with being lazy as long as we don't call this very often, which we surely won't be
void GuildRoster::redrawRoster()
{
	auto listNames = dynamic_pointer_cast<TextLines>(getRenderObject(Interface::ListNames));
	ASSERT(listNames != nullptr);

	auto listLevels = dynamic_pointer_cast<TextLines>(getRenderObject(Interface::ListLevels));
	ASSERT(listLevels != nullptr);

	auto listClasses = dynamic_pointer_cast<TextLines>(getRenderObject(Interface::ListClasses));
	ASSERT(listClasses != nullptr);

	listNames->clear();
	listLevels->clear();
	listClasses->clear();

	auto itr = m_members.begin();

	while (itr != m_members.end())
	{
		if (m_showOffline || itr->second.online)
			addMemberToDrawLists(itr);

		++itr;
	}
}

void GuildRoster::clearMemberList()
{
	m_members.clear();
	setNumOnline(0);
	redrawRoster();
}