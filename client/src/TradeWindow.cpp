#include "stdafx.h"
#include "TradeWindow.h"
#include "ItemIcon.h"
#include "TextBoxRo.h"
#include "ContentMgr.h"
#include "Text.h"
#include "SpriteRo.h"
#include "Sprite.h"
#include "World.h"
#include "Tooltip.h"
#include "Application.h"
#include "ConfirmMessageBox.h"
#include "Connector.h"
#include "ClientPlayer.h"
#include "Inventory.h"
#include "Game.h"
#include "PopupPrompt.h"

#include "..\StringHelpers.h"
#include "..\SqlConnector\QueryResult.h"
#include "..\Shared\GamePacketClient.h"

#include <assert.h>

TradeWindow::TradeWindow(World& owner, const int id) :
	DialogPanel(owner, id, "trade.png", sf::Vector2i(365, 19))
{
	setMultiInput(true);

	// Portrait positions - top area where the two slots would have been
	m_localPortraitPos = sf::Vector2i(60, 103);
	m_remotePortraitPos = sf::Vector2i(253, 103);

	// Name labels above/below portraits
	addRenderObject(attachObj(m_localNameTxt = make_shared<TextBoxRo>(*this, Interface::LocalNameText, FontId::Palatino, 100, 14, TextBox::AlignLeft, false, 1.f), sf::Vector2i(107, 65)));
	addRenderObject(attachObj(m_remoteNameTxt = make_shared<TextBoxRo>(*this, Interface::RemoteNameText, FontId::Palatino, 100, 14, TextBox::AlignLeft, false, 1.f), sf::Vector2i(300, 65)));

	// Trade and Cancel buttons at bottom
	addRenderObject(attachObj(make_shared<Button>(*this, "generic_highlight_medium", Interface::TradeButton), sf::Vector2i(310, 514)));

	// Gold highlight button (clickable area over local gold)
	addRenderObject(attachObj(m_localGoldHighlight = make_shared<Button>(*this, "generic_highlight_small", Interface::LocalGoldHighlight), sf::Vector2i(45, 471)));

	// Gold text displays
	addRenderObject(attachObj(m_localGoldTxt = make_shared<TextBoxRo>(*this, Interface::LocalGoldText, FontId::Constantia, 80, 14, TextBox::AlignLeft, false, 1.f), sf::Vector2i(58, 469)));
	addRenderObject(attachObj(m_remoteGoldTxt = make_shared<TextBoxRo>(*this, Interface::RemoteGoldText, FontId::Constantia, 80, 14, TextBox::AlignLeft, false, 1.f), sf::Vector2i(251, 469)));

	// Ready indicators (checkmarks or similar) near portraits
	auto readySprite = sContentMgr->spawnSprite("trade_ready.png");
	m_localReadySprite = readySprite;
	m_remoteReadySprite = sContentMgr->spawnSprite("trade_ready.png");

	reset();
}

TradeWindow::~TradeWindow()
{
}

void TradeWindow::input() /*final*/
{
	__super::input();
	__super::grabIcon();

	int buttonId = popFirstRightClickButtonId();

	// Right clicking an item is removing it
	if (isLocalIconId(buttonId))
	{
		int slot = buttonId - Interface::IconSlotLocal1;

		if (auto icon = dynamic_pointer_cast<ItemIcon>(getRenderObject(buttonId)))
		{
			// Send packet to remove item from trade
			GP_Client_TradeRemoveItem packet;
			packet.m_itemGuid = icon->getItemGuid();
			sConnector->sendPacket(packet.build(StlBuffer{}));
		}
	}

	// Trade button
	if (popButtonId(Interface::TradeButton))
	{
		GP_Client_TradeConfirm packet;
		sConnector->sendPacket(packet.build(StlBuffer{}));
	}

	// Local gold highlight button clicked - open popup prompt
	if (popButtonId(Interface::LocalGoldHighlight))
	{
		sApplication->spawnPopupPrompt("Enter gold amount to offer:", PopupPrompt::Codes::TradeSetGold);
	}

	// Check for popup prompt response
	if (auto promptPopup = sApplication->popPopupPrompt(
		{
			PopupPrompt::TradeSetGold
		}))
	{
		if (promptPopup->isAccepted())
		{
			switch (promptPopup->getCode())
			{
				case PopupPrompt::TradeSetGold:
				{
					int goldAmount = atoi(promptPopup->getContent().c_str());

					if (goldAmount < 0)
						goldAmount = 0;

					GP_Client_TradeSetGold packet;
					packet.m_amount = goldAmount;
					sConnector->sendPacket(packet.build(StlBuffer{}));
					break;
				}
			}
		}
	}
}

void TradeWindow::render() /*final*/
{
	__super::render();

	renderPortrait(true);
	renderPortrait(false);

	// Render ready indicators on top of portraits
	if (m_localReady && m_localReadySprite != nullptr)
		m_localReadySprite->render(sf::Vector2f(getTopLeftCornerRef() + sf::Vector2i(92, 82)));

	if (m_remoteReady && m_remoteReadySprite != nullptr)
		m_remoteReadySprite->render(sf::Vector2f(getTopLeftCornerRef() + sf::Vector2i(285, 82)));

	if (m_localPlayer != nullptr && m_remotePlayer != nullptr)
	{
		if (m_localPlayer->getWorldPosition().getDist(m_remotePlayer->getWorldPosition()) > PlayerDefines::Trade::TradeDistance_Cells)
			m_world.closePanel(static_cast<World::Interface>(getId()));
	}
}

void TradeWindow::renderPortrait(bool isLocal)
{
	shared_ptr<Sprite> portrait = isLocal ? m_localPortrait : m_remotePortrait;
	const int& offset = isLocal ? m_localPortraitOffset : m_remotePortraitOffset;
	const sf::Vector2i& pos = isLocal ? m_localPortraitPos : m_remotePortraitPos;

	if (portrait != nullptr)
		portrait->renderAsCircle(sf::Vector2f(pos + getTopLeftCornerRef()), PORTRAIT_RADIUS, { 0, offset });
}

void TradeWindow::givenGabbedIcon(shared_ptr<GameIcon> myIcon, shared_ptr<GameIcon> heldIcon) /*final*/
{
	auto inventory = dynamic_cast<Inventory const*>(heldIcon->getOwner());


	// We only can trade FROM the inventory
	if (inventory == nullptr)
		return;

	if (heldIcon->getEntry() != 0 && myIcon->getId() >= Interface::IconSlotLocal1 && myIcon->getId() <= IconSlotLocal5)
	{
		if (auto ptr = dynamic_pointer_cast<ItemIcon>(heldIcon))
		{
			GP_Client_TradeAddItem packet;
			packet.m_invSlot = heldIcon->getId();
			sConnector->sendPacket(packet.build(StlBuffer{}));
			Game::playItemSound(ptr->getItemDef().m_itemId);
		}
	}
}

void TradeWindow::reset()
{
	m_localReady = false;
	m_remoteReady = false;

	// Reset gold displays
	m_localGoldTxt->setString("0", sf::Color(255, 215, 0, 255), sf::Color(0, 0, 0, 50));
	m_remoteGoldTxt->setString("0", sf::Color(255, 215, 0, 255), sf::Color(0, 0, 0, 50));

	// Highlights for local slots
	for (int i = Interface::HighlightLocal1; i <= Interface::HighlightLocal5; ++i)
		addRenderObject(attachHighlight(make_shared<Button>(*this, "vendor_frame", i), true));

	// Highlights for remote slots
	for (int i = Interface::HighlightRemote1; i <= Interface::HighlightRemote5; ++i)
		addRenderObject(attachHighlight(make_shared<Button>(*this, "vendor_frame", i), false));

	// Titles for local slots
	for (int i = Interface::TitleLocal1; i <= Interface::TitleLocal5; ++i)
		addRenderObject(attachTitle(make_shared<TextBoxRo>(*this, i, FontId::Constantia, 110, 12, TextBox::AlignLeft, false, 1.f), true));

	// Titles for remote slots
	for (int i = Interface::TitleRemote1; i <= Interface::TitleRemote5; ++i)
		addRenderObject(attachTitle(make_shared<TextBoxRo>(*this, i, FontId::Constantia, 110, 12, TextBox::AlignLeft, false, 1.f), false));

	// Icons for local slots
	for (int i = Interface::IconSlotLocal1; i <= Interface::IconSlotLocal5; ++i)
		addRenderObject(attachIcon(make_shared<ItemIcon>(*this, i, "gameicon40"), true));

	// Icons for remote slots
	for (int i = Interface::IconSlotRemote1; i <= Interface::IconSlotRemote5; ++i)
		addRenderObject(attachIcon(make_shared<ItemIcon>(*this, i, "gameicon40"), false));
}

void TradeWindow::setLocalPlayer(shared_ptr<ClientPlayer> player)
{
	m_localPlayer = player;
	updatePortrait(player, true);

	if (player != nullptr)
		m_localNameTxt->setString(player->getName(), sf::Color(124, 114, 101, 255), sf::Color(0, 0, 0, 50));
}

void TradeWindow::setRemotePlayer(shared_ptr<ClientPlayer> player)
{
	m_remotePlayer = player;
	updatePortrait(player, false);

	if (player != nullptr)
	{
		m_remoteNameTxt->setString(player->getName(), sf::Color(124, 114, 101, 255), sf::Color(0, 0, 0, 50));
	}
}

void TradeWindow::setLocalReady(const bool ready)
{
	if (ready && !m_localReady)
		sContentMgr->playSound(SfxId::AlertTradeConfirm);

	m_localReady = ready;
}

void TradeWindow::setRemoteReady(const bool ready)
{
	if (ready && !m_remoteReady)
		sContentMgr->playSound(SfxId::AlertTradeConfirm);

	m_remoteReady = ready;
}

void TradeWindow::updatePortrait(shared_ptr<ClientPlayer> player, bool isLocal)
{
	string textureName = "portrait_grey.png";
	int offset = 0;

	if (player != nullptr)
	{
		const string genderStr = player->getGender() == PlayerDefines::Gender::Male ? "male" : "female";
		textureName = Util::fmtStr("portrait_%s (%d).png", genderStr.c_str(), player->getPortrait());
	}

	auto portrait = sContentMgr->spawnPortrait(textureName);

	if (portrait == nullptr)
	{
		portrait = sContentMgr->spawnPortrait("portrait_grey.png");
		offset = 0;
	}
	else
	{
		offset = sContentMgr->getPortraitOffset(*portrait);
	}

	if (isLocal)
	{
		m_localPortrait = portrait;
		m_localPortraitOffset = offset;
	}
	else
	{
		m_remotePortrait = portrait;
		m_remotePortraitOffset = offset;
	}
}

void TradeWindow::setLocalGold(const int amount)
{
	m_localGold = amount;
	m_localGoldTxt->setString(Util::formatMoneyCommas(amount), sf::Color(255, 215, 0, 255), sf::Color(0, 0, 0, 50));
}

void TradeWindow::setRemoteGold(const int amount)
{
	m_remoteGold = amount;
	m_remoteGoldTxt->setString(Util::formatMoneyCommas(amount), sf::Color(255, 215, 0, 255), sf::Color(0, 0, 0, 50));
}

// --- Lua-facing read + drive ---
string TradeWindow::localName() const  { return m_localPlayer  ? m_localPlayer->getName()  : string(); }
string TradeWindow::remoteName() const { return m_remotePlayer ? m_remotePlayer->getName() : string(); }

bool TradeWindow::tradeSlot(bool isLocal, int slot, int& itemId, int& count, int& itemGuid) const
{
	if (slot < 0 || slot > (IconSlotLocal5 - IconSlotLocal1))
		return false;
	const int id = (isLocal ? IconSlotLocal1 : IconSlotRemote1) + slot;
	auto icon = dynamic_pointer_cast<ItemIcon>(getRenderObject(id));
	if (!icon || icon->getItemDef().m_itemId == 0)
		return false;
	itemId   = icon->getItemDef().m_itemId;
	count    = icon->getStackCount();
	itemGuid = icon->getItemGuid();
	return true;
}

void TradeWindow::addTradeBagItem(int bagSlot)
{
	GP_Client_TradeAddItem packet; packet.m_invSlot = bagSlot;
	sConnector->sendPacket(packet.build(StlBuffer{}));
}
void TradeWindow::removeTradeItem(int itemGuid)
{
	GP_Client_TradeRemoveItem packet; packet.m_itemGuid = itemGuid;
	sConnector->sendPacket(packet.build(StlBuffer{}));
}
void TradeWindow::sendTradeGold(int amount)
{
	GP_Client_TradeSetGold packet; packet.m_amount = amount < 0 ? 0 : amount;
	sConnector->sendPacket(packet.build(StlBuffer{}));
}
void TradeWindow::confirmTrade()
{
	GP_Client_TradeConfirm packet;
	sConnector->sendPacket(packet.build(StlBuffer{}));
}

shared_ptr<Tooltip> TradeWindow::buildTradeTooltip(bool isLocal, int slot)
{
	if (slot < 0 || slot > (IconSlotLocal5 - IconSlotLocal1))
		return nullptr;
	const int id = (isLocal ? IconSlotLocal1 : IconSlotRemote1) + slot;
	auto icon = dynamic_pointer_cast<ItemIcon>(getRenderObject(id));
	return icon ? icon->buildTooltip() : nullptr;
}

void TradeWindow::addLocalItem(const ItemDefines::ItemDefinition entry, const int itemGuid, const int stackSize, const int slot)
{
	if (slot < 0 || slot >= 5)
		return;

	// Update the visual
	int iconId = Interface::IconSlotLocal1 + slot;
	int highlightId = Interface::HighlightLocal1 + slot;
	int titleId = Interface::TitleLocal1 + slot;

	if (auto icon = dynamic_pointer_cast<ItemIcon>(getRenderObject(iconId)))
	{
		icon->setItemGuid(itemGuid);
		icon->setItemDef(entry);
		icon->setStackCount(stackSize);
		icon->setHidden(false);
	}

	if (auto highlight = dynamic_pointer_cast<Button>(getRenderObject(highlightId)))
		highlight->setHidden(false);

	if (auto titleText = dynamic_pointer_cast<TextBoxRo>(getRenderObject(titleId)))
	{
		if (auto icon = dynamic_pointer_cast<ItemIcon>(getRenderObject(iconId)))
		{
			titleText->setString(icon->deduceTitle(), sf::Color(124, 114, 101, 255), sf::Color(0, 0, 0, 50), 2);
			titleText->getTextRef().getTextRef().setShadowOffset(1);
			titleText->setHidden(false);

			// Adjust Y position if title has 2 lines
			const int idBase = slot;
			int titleY = 152 + int(66.5f * float(idBase));
			if (titleText->getTextRef().getNumLines() >= 2)
				titleY -= 5;

			titleText->setOffset({ titleText->getOffset().x, titleY });
		}
	}
}

void TradeWindow::removeLocalItem(const int slot)
{
	if (slot < 0 || slot >= 5)
		return;

	int iconId = Interface::IconSlotLocal1 + slot;
	int highlightId = Interface::HighlightLocal1 + slot;
	int titleId = Interface::TitleLocal1 + slot;

	if (auto icon = dynamic_pointer_cast<ItemIcon>(getRenderObject(iconId)))
	{
		icon->setItemDef({});
		icon->setStackCount(0);
	}

	if (auto highlight = dynamic_pointer_cast<Button>(getRenderObject(highlightId)))
		highlight->setHidden(true);

	if (auto titleText = dynamic_pointer_cast<TextBoxRo>(getRenderObject(titleId)))
		titleText->setHidden(true);
}

void TradeWindow::clearLocalItems()
{
	for (int i = 0; i < 5; ++i)
		removeLocalItem(i);

	setLocalGold(0);
}

void TradeWindow::addRemoteItem(const ItemDefines::ItemDefinition entry, const int stackSize, const int slot)
{
	if (slot < 0 || slot >= 5)
		return;

	int iconId = Interface::IconSlotRemote1 + slot;
	int highlightId = Interface::HighlightRemote1 + slot;
	int titleId = Interface::TitleRemote1 + slot;

	if (auto icon = dynamic_pointer_cast<ItemIcon>(getRenderObject(iconId)))
	{
		icon->setItemDef(entry);
		icon->setStackCount(stackSize);
		icon->setHidden(false);
	}

	if (auto highlight = dynamic_pointer_cast<Button>(getRenderObject(highlightId)))
		highlight->setHidden(false);

	if (auto titleText = dynamic_pointer_cast<TextBoxRo>(getRenderObject(titleId)))
	{
		if (auto icon = dynamic_pointer_cast<ItemIcon>(getRenderObject(iconId)))
		{
			titleText->setString(icon->deduceTitle(), sf::Color(124, 114, 101, 255), sf::Color(0, 0, 0, 50), 2);
			titleText->getTextRef().getTextRef().setShadowOffset(1);
			titleText->setHidden(false);

			// Adjust Y position if title has 2 lines
			const int idBase = slot;
			int titleY = 152 + int(66.5f * float(idBase));

			if (titleText->getTextRef().getNumLines() >= 2)
				titleY -= 5;

			titleText->setOffset({ titleText->getOffset().x, titleY });
		}
	}
}

void TradeWindow::removeRemoteItem(const int slot)
{
	if (slot < 0 || slot >= 5)
		return;

	int iconId = Interface::IconSlotRemote1 + slot;
	int highlightId = Interface::HighlightRemote1 + slot;
	int titleId = Interface::TitleRemote1 + slot;

	if (auto icon = dynamic_pointer_cast<ItemIcon>(getRenderObject(iconId)))
	{
		icon->setItemDef({});
		icon->setHidden(true);
	}

	if (auto highlight = dynamic_pointer_cast<Button>(getRenderObject(highlightId)))
		highlight->setHidden(true);

	if (auto titleText = dynamic_pointer_cast<TextBoxRo>(getRenderObject(titleId)))
		titleText->setHidden(true);
}

void TradeWindow::onClose() /*final*/
{
	GP_Client_TradeCancel packet;
	sConnector->sendPacket(packet.build(StlBuffer{}));
}

void TradeWindow::clearRemoteItems()
{
	for (int i = 0; i < 5; ++i)
		removeRemoteItem(i);

	setRemoteGold(0);
}

shared_ptr<GameIcon> TradeWindow::attachIcon(shared_ptr<GameIcon> gi, bool isLocal)
{
	int idBase;
	int xOffset;

	if (isLocal)
	{
		ASSERT(gi->getId() >= Interface::IconSlotLocal1 && gi->getId() <= Interface::IconSlotLocal5);
		idBase = gi->getId() - Interface::IconSlotLocal1;
		xOffset = 33; // Left column
	}
	else
	{
		ASSERT(gi->getId() >= Interface::IconSlotRemote1 && gi->getId() <= Interface::IconSlotRemote5);
		idBase = gi->getId() - Interface::IconSlotRemote1;
		xOffset = 226; // Right column (33 + 193)
	}

	// Start lower since top row is reserved for portraits
	// Row offset: starting at y=115 (below portrait area), each row is 66.5 pixels
	const int y = idBase;
	const auto offset = sf::Vector2i(xOffset, 150 + int(66.5f * float(y)));

	//gi->setHidden(true);
	gi->setAnchor(&getTopLeftCornerRef());
	gi->setOffset(offset);
	gi->setAllowRightClick(true);
	return gi;
}

shared_ptr<Button> TradeWindow::attachHighlight(shared_ptr<Button> button, bool isLocal)
{
	int idBase;
	int xOffset;

	if (isLocal)
	{
		ASSERT(button->getId() >= Interface::HighlightLocal1 && button->getId() <= Interface::HighlightLocal5);
		idBase = button->getId() - Interface::HighlightLocal1;
		xOffset = 22; // Left column
	}
	else
	{
		ASSERT(button->getId() >= Interface::HighlightRemote1 && button->getId() <= Interface::HighlightRemote5);
		idBase = button->getId() - Interface::HighlightRemote1;
		xOffset = 215; // Right column (22 + 193)
	}

	const int y = idBase;
	const auto offset = sf::Vector2i(xOffset, 144 + int(66.5f * float(y)));

	button->setHidden(true);
	button->setAnchor(&getTopLeftCornerRef());
	button->setOffset(offset);
	return button;
}

shared_ptr<TextBoxRo> TradeWindow::attachTitle(shared_ptr<TextBoxRo> txt, bool isLocal)
{
	int idBase;
	int xOffset;

	if (isLocal)
	{
		ASSERT(txt->getId() >= Interface::TitleLocal1 && txt->getId() <= Interface::TitleLocal5);
		idBase = txt->getId() - Interface::TitleLocal1;
		xOffset = 83; // Left column (33 + 50, next to icon)
	}
	else
	{
		ASSERT(txt->getId() >= Interface::TitleRemote1 && txt->getId() <= Interface::TitleRemote5);
		idBase = txt->getId() - Interface::TitleRemote1;
		xOffset = 276; // Right column (226 + 50)
	}

	const int y = idBase;
	const auto offset = sf::Vector2i(xOffset, 152 + int(66.5f * float(y)));

	txt->setHidden(true);
	txt->setAnchor(&getTopLeftCornerRef());
	txt->setOffset(offset);
	txt->getTextRef().getTextRef().setLineSpacing(0.8f);
	return txt;
}
