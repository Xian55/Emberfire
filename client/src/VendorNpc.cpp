#include "stdafx.h"
#include "VendorNpc.h"
#include "ItemIcon.h"
#include "TextBoxRo.h"
#include "ContentMgr.h"
#include "Text.h"
#include "SpriteRo.h"
#include "World.h"
#include "Tooltip.h"
#include "Application.h"
#include "ConfirmMessageBox.h"
#include "Connector.h"
#include "PopupPrompt.h"
#include "World.h"
#include "GameChat.h"

#include "..\StringHelpers.h"
#include "..\SqlConnector\QueryResult.h"
#include "..\Shared\GamePacketClient.h"

#include <assert.h>

VendorNpc::VendorNpc(World& owner, const int id) :
	DialogPanel(owner, id, "vendor.png", sf::Vector2i(365, 19))
{
	setMultiInput(true);

	addRenderObject(attachObj(m_localMoneyTxt = make_shared<TextBoxRo>(*this, Interface::LocalMoneyText, "Palatino Linotype Regular.ttf", 110, 14, TextBox::AlignLeft, false, 2.f), sf::Vector2i(325, 519)));
	addRenderObject(attachObj(make_shared<Button>(*this, "generic_highlight_medium", Interface::RepairButton), sf::Vector2i(35, 512)));
	addRenderObject(attachObj(make_shared<Button>(*this, "undo_buyback", Interface::BuybackButton), sf::Vector2i(125, 521)));
	addRenderObject(attachObj(make_shared<Button>(*this, "left_arrow", Interface::LeftBtn), sf::Vector2i(35, 478)));
	addRenderObject(attachObj(make_shared<Button>(*this, "right_arrow", Interface::RightBtn), sf::Vector2i(370, 478)));

	reset();
}

VendorNpc::~VendorNpc()
{

}

void VendorNpc::input() /*final*/
{
	__super::input();

	// Left click
	int buttonId = popFirstButtonId();

	if (buttonId >= Interface::IconSlot1 && buttonId <= Interface::IconSlot12)
	{
		auto itemInfo = getItemInfo(Interface(buttonId));

		if (itemInfo.m_itemId.m_itemId != 0)
		{	
			auto& it = sContentMgr->db("item_template");

			string itemName = ItemIcon::formItemTitle(itemInfo.m_itemId);			
			
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift))
			{
				if (auto gameChat = dynamic_pointer_cast<GameChat>(world().getRenderObject(World::GameChatBox)))
				{
					sContentMgr->playSound("button_click_a.ogg");
					gameChat->promptLinkAnItem(itemInfo.m_itemId);
				}
			}
			else
			{
				m_popupBuyEntry = itemInfo.m_itemId;
				sApplication->spawnPopup(Util::fmtStr("Purchase %s for %sg?", itemName.c_str(), Util::formatMoneyCommas(itemInfo.m_cost).c_str()).c_str(),
					ConfirmMessageBox::ConfirmBox_YesNo, ConfirmMessageBox::ConfirmCode_BuyItem);
			}
		}
	}

	if (buttonId == Interface::BuybackButton)
	{
		GP_Client_Buyback packet;
		sConnector->sendPacket(packet.build(StlBuffer{}));
	}

	if (buttonId == Interface::RepairButton)
	{
		GP_Client_Repair packet;
		packet.m_confirmed = false;
		sConnector->sendPacket(packet.build(StlBuffer{}));
	}

	if (buttonId == Interface::RightBtn || buttonId == Interface::LeftBtn)
	{
		// Lazy solution, make a copy and re-insert the next 12
		decltype(m_items) cpy = m_items;
		decltype(m_pageNumber) pageNumber = m_pageNumber;

		reset();

		m_pageNumber = pageNumber;

		if (buttonId == Interface::LeftBtn && m_pageNumber > 0)
			--m_pageNumber;

		if (buttonId == Interface::RightBtn)
			++m_pageNumber;

		for (size_t i = 12 * m_pageNumber; i < cpy.size() && m_items.size() < 12; ++i)
			registerItem(cpy[i].m_itemId, cpy[i].m_cost, cpy[i].m_supply);

		// Done scrolling ?
		getRenderObject(Interface::LeftBtn)->setHidden(m_pageNumber == 0);
		getRenderObject(Interface::RightBtn)->setHidden(m_items.size() < 12);

		m_items = cpy;
	}
		
	// ConfirmCode_BuyItem
	if (auto confirmBox = sApplication->popConfirmBox({ ConfirmMessageBox::ConfirmCode_BuyItem }))
	{
		if (confirmBox->getResult() == ConfirmMessageBox::ConfirmBox_Yes)
			buyItem(m_popupBuyEntry, 1);
	}
		
	// ConfirmCode_BuyItemStack
	if (auto confirmBox = sApplication->popPopupPrompt({ ConfirmMessageBox::ConfirmCode_BuyItemStack }))
		buyItem(m_popupBuyEntry, atoi(confirmBox->getContent().c_str()));

	// Right click
	buttonId = popFirstRightClickButtonId();

	if (buttonId >= Interface::IconSlot1 && buttonId <= Interface::IconSlot12)
	{
		auto itemInfo = getItemInfo(Interface(buttonId));

		if (itemInfo.m_itemId.m_itemId != 0)
		{
			auto& it = sContentMgr->db("item_template");
			string stackCount = it.data(itemInfo.m_itemId.m_itemId, "stack_count");
			string itemName = ItemIcon::formItemTitle(itemInfo.m_itemId);
			int quality = atoi(it.data(itemInfo.m_itemId.m_itemId, "quality").c_str());

			// Shift + Right click = "How many?" if it can stack
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) && atoi(stackCount.c_str()) > 1)
			{
				sApplication->spawnPopupPrompt(Util::fmtStr("How many '%s' to buy? (Max: %s)", itemName.c_str(), stackCount.c_str()), ConfirmMessageBox::ConfirmCode_BuyItemStack, true);
			}
			
			// Otherwise, it's a straight up buy.... unless its a quality item. For those, prompt to be sure. No mistaken large purchases
			else if (quality > ItemDefines::Quality::QualityLv3)
			{
				m_popupBuyEntry = itemInfo.m_itemId;
				sApplication->spawnPopup(Util::fmtStr("Purchase %s for %sg?", itemName.c_str(), Util::formatMoneyCommas(itemInfo.m_cost).c_str()).c_str(),
					ConfirmMessageBox::ConfirmBox_YesNo, ConfirmMessageBox::ConfirmCode_BuyItem);
			}
			else
			{
				buyItem(itemInfo.m_itemId, 1);
			}
		}
	}
}

void VendorNpc::render() /*final*/
{
	__super::render();
}
	
void VendorNpc::buyItem(const ItemDefines::ItemDefinition entry, const int count)
{
	GP_Client_BuyVendorItem packet;
	packet.m_itemId = entry;
	packet.m_count = max(1, count);
	sConnector->sendPacket(packet.build(StlBuffer{}));
}

void VendorNpc::reset()
{
	m_items.clear();
	m_legend.clear();

	m_pageNumber = 0;
	
	getRenderObject(Interface::LeftBtn)->setHidden(true);
	getRenderObject(Interface::RightBtn)->setHidden(true);

	// Highlight
	for (int i = Interface::Highlight1; i <= Interface::Highlight12; ++i)
		addRenderObject(attachHighlight(make_shared<Button>(*this, "vendor_frame", i)));

	// Titles
	for (int i = Interface::Title1; i <= Interface::Title12; ++i)
		addRenderObject(attachTitle(make_shared<TextBoxRo>(*this, i, "Constantia Regular.ttf", 110, 12, TextBox::AlignLeft, false, 1.f)));

	auto coldCoinSprite = sContentMgr->spawnSprite("vendor_gold_coin.png");

	// Gold coins
	for (int i = Interface::GoldCoin1; i <= Interface::GoldCoin12; ++i)
		addRenderObject(attachGoldCoin(make_shared<SpriteRo>(*this, coldCoinSprite, i)));
	
	// Cost
	for (int i = Interface::Cost1; i <= Interface::Cost12; ++i)
		addRenderObject(attachCost(make_shared<TextBoxRo>(*this, i, "Palatino Linotype Regular.ttf", 110, 14, TextBox::AlignLeft, false, 1.f)));

	// Icons
	for (int i = Interface::IconSlot1; i <= Interface::IconSlot12; ++i)
		addRenderObject(attachIcon(make_shared<ItemIcon>(*this, i, "gameicon40")));
}
	
void VendorNpc::setLocalMoney(const int money)
{
	m_localMoneyTxt->setString(Util::formatMoneyCommas(money), sf::Color(95, 77, 64, 255), sf::Color(0, 0, 0, 100));
}

GP_Server_GossipMenu::VendorSlot VendorNpc::getItemInfo(const Interface id) const
{
	auto itr = m_legend.find(id);

	if (itr == m_legend.end())
		return {};

	return itr->second;
}
		
void VendorNpc::updateItemQuantity(const ItemDefines::ItemDefinition entry, const int stack)
{
	for (auto& itr : m_legend)
	{
		if (itr.second.m_itemId == entry)
		{
			itr.second.m_supply = stack;
			break;
		}
	}

	for (int buttonId = Interface::IconSlot1; buttonId <= Interface::IconSlot12; ++buttonId)
	{
		if (auto icon = dynamic_pointer_cast<ItemIcon>(getRenderObject(buttonId)))
		{
			if (icon->getItemDef() == entry)
			{
				if (stack == 0)
					icon->setDarken(true);
				
				icon->setStackCount(stack);
			}
		}
	}
}

void VendorNpc::registerItem(const ItemDefines::ItemDefinition entry, const int cost, const int stack)
{
	GP_Server_GossipMenu::VendorSlot obj;
	obj.m_cost = cost;
	obj.m_itemId = entry;
	obj.m_supply = stack;
	m_items.push_back(obj);

	if (m_items.size() > 12)
	{
		getRenderObject(Interface::RightBtn)->setHidden(false);
		return;
	}

	size_t slot = m_items.size();

	auto& it = sContentMgr->db("item_template");
	auto icon = dynamic_pointer_cast<ItemIcon>(getRenderObject(slot + Interface::IconSlot1 - 1));
	auto titleText = dynamic_pointer_cast<TextBoxRo>(getRenderObject(slot + Interface::Title1 - 1));
	auto coinSprite = dynamic_pointer_cast<SpriteRo>(getRenderObject(slot + Interface::GoldCoin1 - 1));
	auto costText = dynamic_pointer_cast<TextBoxRo>(getRenderObject(slot + Interface::Cost1 - 1));
	auto highlightBtn = dynamic_pointer_cast<Button>(getRenderObject(slot + Interface::Highlight1 - 1));

	// Clicking on this button needs to know info about the item
	m_legend[static_cast<Interface>(icon->getId())] = obj;

	if (obj.m_supply == 0)
		icon->setDarken(true);
		
	icon->setShowStackAmtThreshold(0);
	icon->setStackCount(obj.m_supply);
	icon->setRenderStackAtBottom(true);
	icon->setHidden(false);
	titleText->setHidden(false);
	coinSprite->setHidden(false);
	costText->setHidden(false);
	highlightBtn->setHidden(false);
	icon->setItemDef(entry);

	titleText->setString(icon->deduceTitle(), sf::Color(124, 114, 101, 255), sf::Color(0, 0, 0, 50), 2);
	costText->setString(Util::formatMoneyCommas(obj.m_cost).c_str(), sf::Color(115, 97, 84, 255), sf::Color(0, 0, 0, 50), 1);
	titleText->getTextRef().getTextRef().setShadowOffset(1);
	costText->getTextRef().getTextRef().setShadowOffset(1);
	
	// Two lines of title text needs to be moved up a bit
	const int titleIdBase = titleText->getId() - Interface::Title1;
	int titleY = 85 + int(66.5f * float(titleIdBase / 2));

	if (titleText->getTextRef().getNumLines() >= 2)
		titleY -= 5;

	titleText->setOffset({ titleText->getOffset().x, titleY });

	// "Cost: Cost" needs to be right under the title, which may be 1 or 2 lines
	sf::Vector2i coinOffset = coinSprite->getOffset();
	sf::Vector2i costOffset = costText->getOffset();
	
	if (titleText->getTextRef().getNumLines() >= 2)
	{
		coinOffset.y = titleText->getOffset().y + titleText->getDrawnHeight() + 11;
		costOffset.y = titleText->getOffset().y + titleText->getDrawnHeight() + 7;
	}
	else
	{
		coinOffset.y = titleText->getOffset().y + titleText->getDrawnHeight() + 13;
		costOffset.y = titleText->getOffset().y + titleText->getDrawnHeight() + 9;
	}

	coinSprite->setOffset(coinOffset);
	costText->setOffset(costOffset);
}

shared_ptr<GameIcon> VendorNpc::attachIcon(shared_ptr<GameIcon> gi)
{
	ASSERT(gi->getId() <= Interface::IconSlot12);

	const int y = (gi->getId() - 1) / 2;
	const int x = (gi->getId() - 1) % 2;
	const auto offset = sf::Vector2i(33 + (193 * x), 83 + int(66.5f * float(y)));

	gi->setHidden(true);
	gi->setAnchor(&getTopLeftCornerRef());
	gi->setOffset(offset);
	gi->setAllowRightClick(true);
	gi->setAlwaysLeftSide(true);
	return gi;
}

shared_ptr<TextBoxRo> VendorNpc::attachTitle(shared_ptr<TextBoxRo> txt)
{
	ASSERT(txt->getId() >= Interface::Title1 && txt->getId() <= Interface::Title12);

	const int idbase = txt->getId() - Interface::Title1;
	const int x = idbase % 2;
	const auto offset = sf::Vector2i(83 + (193 * x), 0);
	
	txt->setHidden(true);
	txt->setAnchor(&getTopLeftCornerRef());
	txt->setOffset(offset);
	txt->getTextRef().getTextRef().setLineSpacing(0.8f);
	return txt;
}

shared_ptr<SpriteRo> VendorNpc::attachGoldCoin(shared_ptr<SpriteRo> sprite)
{
	ASSERT(sprite->getId() >= Interface::GoldCoin1 && sprite->getId() <= Interface::GoldCoin12);

	const int idbase = sprite->getId() - Interface::GoldCoin1;
	const int x = idbase % 2;
	const auto offset = sf::Vector2i(83 + (193 * x), 0);
	
	sprite->setHidden(true);
	sprite->setAnchor(&getTopLeftCornerRef());
	sprite->setOffset(offset);
	return sprite;
}

shared_ptr<TextBoxRo> VendorNpc::attachCost(shared_ptr<TextBoxRo> txt)
{
	ASSERT(txt->getId() >= Interface::Cost1 && txt->getId() <= Interface::Cost12);

	const int idbase = txt->getId() - Interface::Cost1;
	const int x = idbase % 2;
	const auto offset = sf::Vector2i(97 + (193 * x), 0);
	
	txt->setHidden(true);
	txt->setAnchor(&getTopLeftCornerRef());
	txt->setOffset(offset);
	return txt;
}

shared_ptr<Button> VendorNpc::attachHighlight(shared_ptr<Button> button)
{
	ASSERT(button->getId() >= Interface::Highlight1 && button->getId() <= Interface::Highlight12);

	const int idbase = button->getId() - Interface::Highlight1;
	const int y = idbase / 2;
	const int x = idbase % 2;
	const auto offset = sf::Vector2i(22 + (193 * x), 72 + int(66.5f * float(y)));

	button->setHidden(true);
	button->setAnchor(&getTopLeftCornerRef());
	button->setOffset(offset);
	return button;
}