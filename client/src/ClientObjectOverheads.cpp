#include "stdafx.h"
#include "ClientObjectOverhead.h"
#include "ClientUnit.h"
#include "Text.h"
#include "World.h"
#include "Application.h"
#include "ContentMgr.h"
#include "UnitFrame.h"
#include "Sprite.h"
#include "ChatBubble.h"
#include "GameChat.h"
#include "ClientPlayer.h"
#include "Options.h"
#include "MapQuester.h"

#include "..\Shared\Config.h"
#include "..\SqlConnector\QueryResult.h"

ClientObjectOverhead::ClientObjectOverhead(ClientObject const& owner) :
	m_owner(owner),
	m_chatBubbleEnd(0)
{
	m_killMarker = sContentMgr->spawnSprite("quest_kill_map.png");
	m_killMarker->setHotspotEasy(true, true);

	m_nameDraw = make_unique<Text>(sContentMgr->getFont("Helvetica 400.ttf"));
	m_nameDraw->setCharacterSize(16);
	m_nameDraw->setOutlineThickness(1.f);
	m_nameDraw->setShadowOffset(1);

	m_subNameDraw = make_unique<Text>(sContentMgr->getFont("Helvetica 400.ttf"));
	m_subNameDraw->setCharacterSize(16);
	m_subNameDraw->setOutlineThickness(1.f);
	m_subNameDraw->setShadowOffset(1);
		
	m_barBg = sContentMgr->spawnSprite("nameplate_bg.png");
	//m_barMp = sContentMgr->spawnSprite("nameplate_mp.png");
	m_barHp = sContentMgr->spawnSprite("nameplate_hp.png");
	m_barParty = sContentMgr->spawnSprite("nameplate_hp_party.png");

	m_alpha = 255;

	//setChatBubble("This is a free online calculator which counts the number of characters or letters in a text.", GameChat::getChatColorInt(ChatDefines::Say));
}

ClientObjectOverhead::~ClientObjectOverhead()
{

}

void ClientObjectOverhead::draw()
{
	int upwardY = m_owner.getScreenPosition().y - m_owner.mouseableHeight() - 15;

	// Hp/Mp
	if (!m_owner.isLocal() && shouldDrawNameplate())
		drawHp(upwardY);

	// Can't be moused over if bars are hidden
	else
		getBottomRightCornerRef() = getTopLeftCornerRef();

	// Name
	if (m_nameDraw != nullptr && shouldDrawName())
	{
		drawName(*m_subNameDraw, upwardY);
		drawName(*m_nameDraw, upwardY);
	}

	// Chat bubble
	if (m_chatBubble != nullptr)
		drawChatBubble(upwardY);
}
		
void ClientObjectOverhead::setDrawOptions(const bool hideHP, const bool hideNameAndChat)
{
	m_hideHP = hideHP;
	m_hideNameAndChat = hideNameAndChat;
}
		
int ClientObjectOverhead::getNameHeight() const
{
	int result = 0;

	if (!m_nameDraw->getString().isEmpty())
		result += int(m_nameDraw->getCharacterSize());

	if (!m_subNameDraw->getString().isEmpty())
		result += int(m_subNameDraw->getCharacterSize());

	return result;
}
	
bool ClientObjectOverhead::shouldDrawName() const
{
	if (m_owner.isLocal())
		return sConfig->getCache(Options::System_YourNameTick);

	if (m_owner.getType() == MutualObject::Type::Npc)
		return sConfig->getCache(Options::System_ShowNpcNameTick);

	if (m_owner.getType() == MutualObject::Type::Player)
		return sConfig->getCache(Options::System_ShowPlayerNameTick);

	return true;
}
		
bool ClientObjectOverhead::shouldDrawNameplate() const
{
	if (!m_owner.isAlive())
		return false;

	if (m_owner.isLocal())
		return false;

	if (auto unit = dynamic_cast<ClientUnit const*>(&m_owner))
	{
		if (unit->faction() == UnitDefines::Faction::Friendly)
			return sConfig->getCache(Options::System_FriendlyNameplateTick);

		if (unit->getType() == MutualObject::Type::Player && m_owner.getWorld() != nullptr && m_owner.getWorld()->myself() != nullptr)
		{
			// todo: Options Tick for party member health bars
			if (m_owner.getWorld()->isPartyMember(unit->getGuid()))
				return true;
			
			if (unit->seesAsHostile(*m_owner.getWorld()->myself()))
				return sConfig->getCache(Options::System_EnemyNameplateTick);

			return sConfig->getCache(Options::System_FriendlyNameplateTick);
		}
	}

	return sConfig->getCache(Options::System_EnemyNameplateTick);
}

void ClientObjectOverhead::drawHp(int& upwardY)
{
	const sf::Vector2i barTopLeft(m_owner.getScreenPosition().x - static_cast<int>(m_barBg->getGlobalBounds().width / 2), upwardY);

	uint8_t alhpa = m_alpha;

	// Target is stronger than the rest
	if (m_owner.getWorld() != nullptr && m_owner.getWorld()->myself() != nullptr)
	{
		if (!isMousedOver() && m_owner.getWorld()->getSelectedGuid() != m_owner.getGuid() && m_owner.getGuid() != m_owner.getWorld()->myself()->getGuid())
			alhpa /= 2;
	}

	if (!m_hideHP)
	{
		// Alpha
		m_barBg->setColor(sf::Color(255, 255, 255, alhpa));
		m_barHp->setColor(sf::Color(255, 255, 255, alhpa));
		m_barParty->setColor(sf::Color(255, 255, 255, alhpa));
		//m_barMp->setColor(sf::Color(255, 255, 255, m_alpha));

		// Health
		m_barBg->render(sf::Vector2f(barTopLeft));
	}
	else
	{
		m_barBg->setColor(sf::Color(0, 0, 0, 0));
		m_barHp->setColor(sf::Color(0, 0, 0, 0));
		m_barParty->setColor(sf::Color(0, 0, 0, 0));
	}

	sf::IntRect rec;
	
	// Different color if it's a party member
	if (m_owner.getType() == MutualObject::Type::Player && m_owner.getWorld() != nullptr && m_owner.getWorld()->isPartyMember(m_owner.getGuid()))
		rec = UnitFrame::renderBarCrop(*m_barParty, barTopLeft, m_owner.getHealthPct());
	else
		rec = UnitFrame::renderBarCrop(*m_barHp, barTopLeft, m_owner.getHealthPct());

	getTopLeftCornerRef() = barTopLeft;
	getBottomRightCornerRef() = barTopLeft + sf::Vector2i(rec.width, rec.height);

	// Kill icon
	if (!m_hideHP && m_owner.getType() == MutualObject::Type::Npc && m_owner.getWorld() != nullptr && m_owner.getWorld()->getMapQuester()->isNpcEntryMarkedKill(m_owner.getEntry()))
		m_killMarker->render(sf::Vector2f(getTopLeftCornerRef()) - sf::Vector2f(15, -5));
	
	// Mana
	//if (m_owner.getMaxMana() != 0)
	//{
	//	m_barBg->render(sf::Vector2f(sf::Vector2i(barTopLeft.x, barTopLeft.y + 4)));
	//	UnitFrame::renderBar(*m_barMp, { barTopLeft.x, barTopLeft.y + 4 }, m_barBg->getGlobalBounds().width, m_owner.getManaPct());
	//}
}

void ClientObjectOverhead::drawName(Text& obj, int& upwardY)
{
	if (obj.getString().isEmpty())
		return;
	
	auto color = m_owner.getNameColor();
	color.a = m_alpha;
	obj.setOriginalColor(color);

	float alphaPct = m_alpha / 255.f;
	obj.setOutlineColor(sf::Color(0, 0, 0, uint8_t(50.f * alphaPct)));
	obj.setOutlineColor(sf::Color(0, 0, 0, uint8_t(50.f * alphaPct)));

	// Center above head
	const int nameY = upwardY - static_cast<int>(obj.getCharacterSize()) - 7;

	if (!m_hideNameAndChat)
		obj.draw(m_owner.getScreenPosition().x - static_cast<int>(obj.getGlobalBounds().width / 2), nameY);

	upwardY = nameY;
}

void ClientObjectOverhead::drawChatBubble(int& upwardY)
{
	if (time(nullptr) > m_chatBubbleEnd)
	{
		m_chatBubble = nullptr;
		return;
	}

	const int chatBubbleHeight = abs(m_chatBubble->getTopLeftCornerRef().y - m_chatBubble->getBottomRightCornerRef().y);
	const int chatBubbleWidth = abs(m_chatBubble->getTopLeftCornerRef().x - m_chatBubble->getBottomRightCornerRef().x);

	m_chatBubble->moveTo({ m_owner.getScreenPosition().x - (chatBubbleWidth / 2), upwardY - chatBubbleHeight });
	upwardY = m_chatBubble->getTopLeftCornerRef().y;

	if (m_chatBubble->getBottomRightCornerRef().x < 0 || m_chatBubble->getTopLeftCornerRef().x > sApplication->sW() ||
		m_chatBubble->getBottomRightCornerRef().y < 0 || m_chatBubble->getTopLeftCornerRef().y > sApplication->sH() ||
		chatBubbleHeight <= 0 || chatBubbleWidth <= 0)
	{
		// No point drawing if it's not on screen
		m_chatBubble->refreshBounds();
		return;
	}
	
	if (!m_hideNameAndChat)
		m_chatBubble->draw();
	else
		m_chatBubble->refreshBounds();
}

void ClientObjectOverhead::refreshName()
{
	string name = m_owner.getName();

	if (m_owner.getType() == MutualObject::Type::Player && sConfig->getCache(Options::System_PlayerRanksTick))
	{
		string rankName = sContentMgr->db("player_exp_levels").data(m_owner.getLevel(), "name");

		if (!rankName.empty())
			name = rankName + " " + name;
	}

	m_nameDraw->setOriginalString(name);

	if (!m_owner.getSubName().empty())
		m_subNameDraw->setOriginalString("<" + m_owner.getSubName() + ">");
	else
		m_subNameDraw->setOriginalString("");
}

void ClientObjectOverhead::setChatBubble(const string& str, const sf::Color colorInt)
{
	m_chatBubble = make_unique<ChatBubble>(*sApplication, sf::Vector2i());
	m_chatBubble->setMouseable(false);
	m_chatBubble->setText("Helvetica 400.ttf", 12, str, colorInt);
	m_chatBubbleEnd = time(nullptr) + max(3, int(str.size() / 10));
}
