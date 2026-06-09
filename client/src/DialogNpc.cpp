#include "stdafx.h"
#include "DialogNpc.h"
#include "TextBoxRo.h"
#include "ContentMgr.h"
#include "World.h"
#include "TextLines.h"
#include "Button.h"
#include "Connector.h"
#include "Sprite.h"
#include "QuestOffer.h"
#include "QuestComplete.h"
#include "VendorNpc.h"
#include "ScrollBar.h"
#include "SpriteRo.h"

#include "..\SqlConnector\QueryResult.h"
#include "..\Shared\GamePacketClient.h"
#include "..\Shared\GamePacketServer.h"
#include "..\StringHelpers.h"

#include <assert.h>

DialogNpc::DialogNpc(World& owner, const int id) :
	DialogPanel(owner, id, "dialog.png", sf::Vector2i(365, 20))
{
	clearGossip();

	m_questDone = sContentMgr->spawnSprite("quest_done_gossip.png");
	m_questDone->setHotspotEasy(true, true);

	m_questAvail = sContentMgr->spawnSprite("quest_ready_gossip.png");
	m_questAvail->setHotspotEasy(true, true);

	m_gossipOptionSpr = sContentMgr->spawnSprite("gossip_option.png");
	m_gossipOptionSpr->setHotspotEasy(true, true);
	
	m_vendorGold = sContentMgr->spawnSprite("gossip_gold_pouch.png");
	m_vendorGold->setHotspotEasy(true, true);

	//applyText("1", "John Doe");
	//addGossipOption("How are you?", 1);
}

DialogNpc::~DialogNpc()
{

}

void DialogNpc::input()
{
	__super::input();

	switch (popFirstButtonId())
	{
		case Interface::GoodbyeButton:
		{
			// The server won't need to know that we closed the window
			m_world.closePanel(World::Interface(getId()));
			break;
		}
	}

	if (m_readyForInput && m_gossipOptions != nullptr)
	{
		const int clickedLine = m_gossipOptions->popPendingClickedLineId();

		if (clickedLine != -1)
		{
			sContentMgr->playSound(SfxId::ButtonClick);
			selectGossip(clickedLine);
		}
	}
	
	if (m_descScrollBar != nullptr)
	{
		if (isMousedOver())
			m_descScrollBar->attemptInput();

		m_description->pumpScrollOffset(m_descScrollBar->getScrollOffset());
	}
}

void DialogNpc::selectGossip(const int lineIdx)
{
	auto itr = m_serverGossipMenu.find(lineIdx);

	if (itr == m_serverGossipMenu.end())
		return;

	switch (itr->second.type)
	{
		//todo: copy of anchor data from this to the one we open up
		case GossipType::GossipQuestAccept:
		{
			// Close this and open up quest accept
			auto questOffer = dynamic_pointer_cast<QuestOffer>(world().getRenderObject(World::Interface::QuestOfferPanel));
			// Server identifies the giver from the open GossipMenu session, NOT from the wire:
			// AcceptQuest (op8, FUN_0048a030) sends only questId. Giver guid kept for UI, not transmitted.
			questOffer->setQuestGiverGuid(m_world.getGossipGuid());
			questOffer->setForQuest(itr->second.entry);
			m_world.openPanel(World::Interface::QuestOfferPanel, false);
			questOffer->getTopLeftCornerRef() = getTopLeftCornerRef();
			m_world.closePanel(World::Interface(getId()), false);
			break;
		}
		case GossipType::GossipQuestComplete:
		{
			// Close this and open up quest complete
			auto questComplete = dynamic_pointer_cast<QuestComplete>(world().getRenderObject(World::Interface::QuestCompletePanel));
			// CompleteQuest (op33, FUN_0048a310) wire = [questId, ignored, itemChoiceIdx]; giver guid
			// is NOT used by the server (parked in the ignored slot). Kept here only for the UI panel.
			questComplete->setQuestGiverGuid(m_world.getGossipGuid());
			questComplete->setForQuest(itr->second.entry);
			m_world.openPanel(World::Interface::QuestCompletePanel, false);
			questComplete->getTopLeftCornerRef() = getTopLeftCornerRef();
			m_world.closePanel(World::Interface(getId()), false);
			break;
		}
		case GossipType::GossipVendor:
		{
			// Close this and open up vendor (it should already have data)
			auto vendor = dynamic_pointer_cast<VendorNpc>(world().getRenderObject(World::Interface::VendorNpcPanel));
			m_world.openPanel(World::Interface::VendorNpcPanel, false);
			m_world.openPanel(World::Interface::InventoryPanel);
			vendor->getTopLeftCornerRef() = getTopLeftCornerRef();
			m_world.closePanel(World::Interface(getId()), false);
			break;
		}
		case GossipType::GossipDialog:
		{
			int nextGossipId = atoi(sContentMgr->db("gossip_option").data(itr->second.entry, "click_new_gossip").c_str());

			if (nextGossipId == -1)
			{
				m_world.closePanel(World::Interface(getId()));
				break;
			}

			GP_Client_ClickedGossipOption packet;
			packet.m_entry = itr->second.entry;
			sConnector->sendPacket(packet.build(StlBuffer{}));

			if (nextGossipId == 0)
				m_world.closePanel(World::Interface(getId()));

			break;
		}
	}
}

bool DialogNpc::gossipOptionAt(const int idx, int& type, int& entry, string& label) const
{
	auto itr = m_serverGossipMenu.find(idx);

	if (itr == m_serverGossipMenu.end() || m_gossipOptions == nullptr)
		return false;

	type = itr->second.type;
	entry = itr->second.entry;

	// The display label lives in the TextLines (added in the same order); trim the leading icon padding.
	if (auto* line = m_gossipOptions->getLine(idx))
	{
		label = line->getTextStr();
		Util::trimStr(label);
	}

	return true;
}

void DialogNpc::render()
{
	__super::render();

	if (m_gossipOptions != nullptr)
	{
		for (auto& itr : m_serverGossipMenu)
		{
			int gossipIdx = itr.first;

			if (gossipIdx >= 0 && gossipIdx < m_gossipOptions->getNumLines())
			{
				auto renderPos = m_gossipOptions->getLine(gossipIdx)->getRenderPos();
				renderPos.y += 7;
				renderPos.x += 2;

				switch (itr.second.type)
				{
				case GossipType::GossipQuestAccept:
					m_questAvail->render(sf::Vector2f(renderPos));
					break;
				case GossipType::GossipQuestComplete:
					m_questDone->render(sf::Vector2f(renderPos));
					break;
				case GossipType::GossipVendor:
					m_vendorGold->render(sf::Vector2f(renderPos));
					break;
				default:
					m_gossipOptionSpr->render(sf::Vector2f(renderPos));
					break;
				}
			}
		}
	}

	if (m_descScrBarBg != nullptr)
		m_descScrBarBg->attemptRender();

	if (m_descScrollBar != nullptr)
		m_descScrollBar->attemptRender();
}

void DialogNpc::onClose() /*override*/
{
	__super::onClose();
	m_serverGossipMenu.clear();
}

void DialogNpc::applyText(const int entry, const string& npcName)
{
	string text = world().getGossipString(entry);

	if (text.empty())
		text = "Greetings.";

	m_world.formatWorldText(text, npcName);
	Util::trimStr(text);
	text = "\"" + text + "\"";

	// Capture for the Lua view (read via gossipText()/npcName()).
	m_npcName = npcName;
	m_gossipText = text;

	int maxLines = 13 - (int)m_serverGossipMenu.size();

	// Title
	auto title = make_shared<TextBoxRo>(*this, Interface::NpcName, FontId::Palatino, 300, 16, TextBox::AlignLeft, false, 2.f);
	title->setAnchor(&getTopLeftCornerRef());
	title->setOffset({ 90, 95 });
	title->setString(npcName + " says:", sf::Color(189, 166, 145, 255), sf::Color(0, 0, 0, 50));
	title->getTextRef().getTextRef().setShadowOffset(1);
	addRenderObject(title);

	// Description
	m_description = make_shared<TextBoxRo>(*this, Interface::GossipText, FontId::Palatino, 330, 14, TextBox::AlignLeft, false, 2.f);
	m_description->setAnchor(&getTopLeftCornerRef());
	m_description->setOffset({ 55, 135 });
	m_description->setString(text, sf::Color(142, 119, 98, 255), sf::Color(0, 0, 0, 50), maxLines, true);
	m_description->getTextRef().getTextRef().setShadowOffset(1);
	addRenderObject(m_description);

	// Goodbye
	auto goodbyeButton = make_shared<Button>(*this, "generic_highlight", Interface::GoodbyeButton);
	goodbyeButton->setOffset({ 145, 500 });
	goodbyeButton->setAnchor(&getTopLeftCornerRef());
	addRenderObject(goodbyeButton);
		
	if (m_description->getScrollMax() > 0)
	{
		m_descScrollBar = make_shared<ScrollBar>(*this, "scrollbar_thin_blank", "scrollbar_thin_blank", ScrollBar::ScrollTopDown, "scrollbar_thin", Interface::DescriptionScroll);
		attachObj(m_descScrollBar->getScrollUpButton(), sf::Vector2i(30, 135));
		attachObj(m_descScrollBar->getScrollDownButton(), sf::Vector2i(30, 230));
		m_descScrollBar->getScrollUpButton()->setHidden(true);
		m_descScrollBar->getScrollDownButton()->setHidden(true);
		m_descScrollBar->setMaxOffset(m_description->getScrollMax());
		attachObj(m_descScrBarBg = make_shared<SpriteRo>(*this, sContentMgr->spawnSprite("questoffer_scroll_bg.png"), Interface::DescriptionScrollBg), { 34, 147 });
		attachObj(m_descScrollBar, {});
	}
	else
	{
		m_descScrollBar = nullptr;
		m_descScrBarBg = nullptr;
	}

	adjustGossipY();
}

void DialogNpc::clearGossip()
{
	destroyObjectById(Interface::GossipOptions);
	destroyObjectById(Interface::GossipOptionsTitle);
	m_gossipOptions = nullptr;
	m_readyForInput = true;
}

void DialogNpc::adjustGossipY()
{
	if (m_gossipOptions == nullptr)
		return;

	int yPos = 250;

	if (auto description = dynamic_pointer_cast<TextBoxRo>(getRenderObject(Interface::GossipText)))
		yPos = description->getOffset().y + description->getDrawnHeight() + 20;
	
	if (auto ptr = getRenderObject(Interface::GossipOptionsTitle))
		ptr->setOffset({ 55, yPos });

	m_gossipOptions->setOffset({ 55, yPos + 30 });
}

void DialogNpc::createGossipOptions()
{
	int yPos = 250;

	if (auto description = dynamic_pointer_cast<TextBoxRo>(getRenderObject(Interface::GossipText)))
		yPos = description->getOffset().y + description->getDrawnHeight() + 20;

	// Gossip options title
	auto gossipOptionsTitle = make_shared<TextBoxRo>(*this, Interface::GossipOptionsTitle, FontId::Palatino, 330, 15, TextBox::AlignLeft, false, 2.f);
	gossipOptionsTitle->setAnchor(&getTopLeftCornerRef());
	gossipOptionsTitle->setOffset({ 55, yPos });
	gossipOptionsTitle->setString("Options:", sf::Color(189, 166, 145, 255), sf::Color(0, 0, 0, 50));
	gossipOptionsTitle->getTextRef().getTextRef().setShadowOffset(1);
	addRenderObject(gossipOptionsTitle);

	// Gossip options
	m_gossipOptions = make_shared<TextLines>(*this, Interface::GossipOptions, FontId::Palatino, Util::GeoBox2d(0, 0, 250, 250));
	m_gossipOptions->setAnchor(&getTopLeftCornerRef());
	m_gossipOptions->setOffset({ 55, yPos + 30 });
	m_gossipOptions->setDialogCharacterSize(13);
	m_gossipOptions->setClickableLines(true);
	m_gossipOptions->setLeading(5);
	m_gossipOptions->setShadowOffset(1);
	addRenderObject(m_gossipOptions);
}

void DialogNpc::addVendorButton()
{
	if (m_gossipOptions == nullptr)
		createGossipOptions();

	GossipField dat;
	dat.type = GossipType::GossipVendor;
	m_serverGossipMenu[m_gossipOptions->getNumLines()] = dat;
	m_gossipOptions->addLine("     I'd like to browse your goods.", getGossipOptionColor());
}

void DialogNpc::addGossip_Dialog(const int entry)
{
	if (m_gossipOptions == nullptr)
		createGossipOptions();

	GossipField dat;
	dat.type = GossipType::GossipDialog;
	dat.entry = entry;
	m_serverGossipMenu[m_gossipOptions->getNumLines()] = dat;
	m_gossipOptions->addLine("     \"" + world().getGossipOptionsString(entry) + "\"", getGossipOptionColor());
}

void DialogNpc::addGossip_QuestAccept(const int questId)
{
	if (m_gossipOptions == nullptr)
		createGossipOptions();

	GossipField dat;
	dat.type = GossipType::GossipQuestAccept;
	dat.entry = questId;
	m_serverGossipMenu[m_gossipOptions->getNumLines()] = dat;
	m_gossipOptions->addLine("     " + sContentMgr->db("quest_template").data(questId, "name"), getGossipOptionColor());
}

void DialogNpc::addGossip_QuestComplete(const int questId)
{
	if (m_gossipOptions == nullptr)
		createGossipOptions();

	GossipField dat;
	dat.type = GossipType::GossipQuestComplete;
	dat.entry = questId;	
	m_serverGossipMenu[m_gossipOptions->getNumLines()] = dat;
	m_gossipOptions->addLine("     " + sContentMgr->db("quest_template").data(questId, "name") + " (Complete)", getGossipOptionColor());
}