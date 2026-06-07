#include "stdafx.h"
#include "LootWindow.h"
#include "DraggableNode.h"
#include "Application.h"
#include "Button.h"
#include "GameIconList.h"
#include "Connector.h"
#include "ContentMgr.h"
#include "ItemIcon.h"
#include "GameChat.h"
#include "World.h"

#include "..\Shared\GamePacketClient.h"

LootWindow::LootWindow(World& owner, const int id) :
	WorldPanel(owner, id, "loot_window.png", sf::Vector2i(184, 12), "panel_close_small")
{
	setIndependant(true);

	m_dragNode = make_unique<DraggableNode>();
	m_dragNode->addAnchor(&m_topLeftCorner);
	m_dragNode->addAnchor(&m_bottomRightCorner);
	m_dragNode->setDragNodeHeight(35);

	addRenderObject(m_upButton = make_shared<Button>(*this, "up_arrow", Interface::ScrollUp));
	addRenderObject(m_downButton = make_shared<Button>(*this, "down_arrow", Interface::ScrollDown));
	addRenderObject(m_takeallButton = make_shared<Button>(*this, "generic_highlight", Interface::TakeAll));

	m_upButton->setFlashing(true);
	m_downButton->setFlashing(true);

	// Off-screen scratch icon: set its def + build a tooltip on demand (loot tooltips for the Lua view).
	// Not added to the render holder — it never draws/inputs.
	m_tooltipIcon = make_shared<ItemIcon>(*this, Interface::TooltipScratch, "gameicon40");

	reset();
}

LootWindow::~LootWindow()
{

}

void LootWindow::addMoney(const int amount)
{
	m_gameIconList->addItemIcon({ ItemDefines::StaticItems::GoldItem }, amount);
}

void LootWindow::addItem(const ItemDefines::ItemDefinition itemId, const int count)
{
	m_gameIconList->addItemIcon(itemId, count);
}

void LootWindow::reset()
{
	m_gameIconList = make_shared<GameIconList>(world(), *this, Interface::IconList,
		GameIcon::Type::Item,
		4,
		125,
		sf::Vector2i(0, 0),
		"game_icon_list_fade",
		43,
		fontFile(FontId::Arial),
		14,
		sf::Color::White,
		sf::Color::Black,
		2.f,
		sf::Vector2i(45, 10));

	addRenderObject(m_gameIconList);
	getTopLeftCornerRef() = { sApplication->sW() / 3, sApplication->sH() / 3 };
}

void LootWindow::render()
{
	m_gameIconList->getTopLeftCornerRef() = getTopLeftCornerRef() + sf::Vector2i(10, 38);

	__super::render();

	const int nwidth = mouseableWidth();
	const int nheight = mouseableHeight();

	if (getTopLeftCornerRef().x < 0)
		getTopLeftCornerRef().x = 0;

	if (getTopLeftCornerRef().x + nwidth > sApplication->sW())
		getTopLeftCornerRef().x = sApplication->sW() - nwidth;

	if (getTopLeftCornerRef().y < 0)
		getTopLeftCornerRef().y = 0;

	if (getTopLeftCornerRef().y + nheight > sApplication->sH())
		getTopLeftCornerRef().y = sApplication->sH() - nheight;

	// Take All
	m_takeallButton->getTopLeftCornerRef() = sf::Vector2i(getTopLeftCornerRef() + sf::Vector2i(37, 217));

	// Up/Down arrows
	m_upButton->getTopLeftCornerRef() = sf::Vector2i(getTopLeftCornerRef() + sf::Vector2i(183, 40));
	m_downButton->getTopLeftCornerRef() = sf::Vector2i(getTopLeftCornerRef() + sf::Vector2i(183, 230));

	getRenderObject(Interface::ScrollDown)->setHidden(m_gameIconList->downMaxxed());
	getRenderObject(Interface::ScrollUp)->setHidden(m_gameIconList->upMaxxed());
}

void LootWindow::input()
{
	__super::input();
	m_dragNode->updateDraggableObject(*this);

	auto clickedpair = m_gameIconList->popClickedGameIcon();

	if (auto itemIcon = dynamic_pointer_cast<ItemIcon>(clickedpair.first))
	{
		if ((sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) && itemIcon != nullptr)
		{
			if (auto gameChat = dynamic_pointer_cast<GameChat>(world().getRenderObject(World::GameChatBox)))
			{
				sContentMgr->playSound(SfxId::ButtonClick);
				gameChat->promptLinkAnItem(itemIcon->getItemDef());
			}
		}
		else
		{
			GP_Client_LootItem packet;
			packet.m_sourceGuid = m_sourceGuid;
			packet.m_itemId = itemIcon->getItemDef();
			{ std::ofstream dbg("maploaddebug.txt", std::ios::app); dbg << "LOOTCLICK idx=" << clickedpair.second << " entry=" << packet.m_itemId.m_itemId << " affix=" << packet.m_itemId.m_affixId << " src=" << m_sourceGuid << "\n"; }
			sConnector->sendPacket(packet.build(StlBuffer{}));
			sContentMgr->playSound(SfxId::AlertEntry);
		}
	}

	switch (popFirstButtonId())
	{
		case Interface::ScrollDown:
		{
			m_gameIconList->scroll(1);
			break;
		}
		case Interface::ScrollUp:
		{
			m_gameIconList->scroll(-1);
			break;
		}
		case Interface::TakeAll:
		{
			GP_Client_LootItem packet;
			packet.m_sourceGuid = m_sourceGuid;
			packet.m_itemId = { uint16_t(GP_Client_LootItem::TakeAll) };
			sConnector->sendPacket(packet.build(StlBuffer{}));
			break;
		}
	}
}

// --- Lua-facing loot API (the Lua LootWindow.lua drives these while this C++ window is force-hidden) -------
int LootWindow::lootCount() const
{
	return m_gameIconList ? m_gameIconList->numEntries() : 0;
}

bool LootWindow::lootAt(const int index, ItemDefines::ItemDefinition& def, int& stack, bool& isGold) const
{
	if (!m_gameIconList || !m_gameIconList->itemEntryAt(index, def, stack))
		return false;
	isGold = (def.m_itemId == ItemDefines::StaticItems::GoldItem);
	return true;
}

void LootWindow::lootIndex(const int index)
{
	ItemDefines::ItemDefinition def; int stack = 0; bool gold = false;
	if (!lootAt(index, def, stack, gold))
		return;

	GP_Client_LootItem packet;
	packet.m_sourceGuid = m_sourceGuid;
	packet.m_itemId     = def;
	sConnector->sendPacket(packet.build(StlBuffer{}));
	sContentMgr->playSound(SfxId::AlertEntry);
}

void LootWindow::lootAll()
{
	GP_Client_LootItem packet;
	packet.m_sourceGuid = m_sourceGuid;
	packet.m_itemId     = { uint16_t(GP_Client_LootItem::TakeAll) };
	sConnector->sendPacket(packet.build(StlBuffer{}));
}

void LootWindow::linkIndex(const int index)
{
	ItemDefines::ItemDefinition def; int stack = 0; bool gold = false;
	if (!lootAt(index, def, stack, gold))
		return;

	if (auto gameChat = dynamic_pointer_cast<GameChat>(world().getRenderObject(World::GameChatBox)))
	{
		sContentMgr->playSound(SfxId::ButtonClick);
		gameChat->promptLinkAnItem(def);
	}
}

shared_ptr<Tooltip> LootWindow::buildLootTooltip(const int index)
{
	ItemDefines::ItemDefinition def; int stack = 0; bool gold = false;
	if (!lootAt(index, def, stack, gold) || !m_tooltipIcon)
		return nullptr;

	m_tooltipIcon->setItemDef(def);
	return m_tooltipIcon->buildTooltip();
}
