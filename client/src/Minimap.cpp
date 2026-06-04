#include "stdafx.h"
#include "Minimap.h"
#include "ContentMgr.h"
#include "SpriteRo.h"
#include "Application.h"
#include "Button.h"
#include "Text.h"
#include "Sprite.h"
#include "World.h"
#include "ClientPlayer.h"
#include "ClientMap.h"
#include "Connector.h"
#include "Keybinds.h"

#include "..\StringHelpers.h"
#include "..\Shared\GameMap.h"
#include "..\Shared\GamePacketClient.h"

#include <assert.h>

Minimap::Minimap(World& owner, const int id) :
	WorldChild(owner, id, owner)
{
	// Map
	m_bgSpr = sContentMgr->spawnSprite("minimap.png");
	addRenderObject(attachObj(make_shared<SpriteRo>(*this, m_bgSpr, Interface::RoBackground), sf::Vector2i{}));
	m_mapDecal = sContentMgr->getTexture("miniamp_decal.png");
	
	// Label
	m_label = make_shared<Text>(sContentMgr->getFont(FontId::Ringbearer));
	m_label->setOutlineColor(sf::Color(0, 0, 0, 125));
	m_label->setOutlineThickness(2.f);
	m_label->setOriginalColor(sf::Color(201, 179, 149));
	m_label->setCharacterSize(18);

	// Label
	m_channelLabel = make_shared<Text>(sContentMgr->getFont(FontId::Ringbearer));
	m_channelLabel->setOutlineColor(sf::Color(0, 0, 0, 125));
	m_channelLabel->setOutlineThickness(2.f);
	m_channelLabel->setOriginalColor(sf::Color(201, 179, 149));
	m_channelLabel->setCharacterSize(18);
	
	// Buttons
	m_button = static_pointer_cast<Button>(attachObj(make_shared<Button>(*this, "minimap_button", Interface::RoMinimapButton), sf::Vector2i(101, 254)));
	m_mailLootButton = static_pointer_cast<Button>(attachObj(make_shared<Button>(*this, "gold_pouch", Interface::RoLootbutton), sf::Vector2i(-5, 240)));
	m_mailLootButton->setHidden(true);

	// Cache dots
	m_dotEnemy = sContentMgr->spawnSprite("minimap_enemy.png");
	m_dotEnemy->setHotspotEasy(true, true);

	m_dotNeutral = sContentMgr->spawnSprite("minimap_neutral.png");
	m_dotNeutral->setHotspotEasy(true, true);

	m_dotFriendly = sContentMgr->spawnSprite("minimap_friendly.png");
	m_dotFriendly->setHotspotEasy(true, true);

	m_dotDead = sContentMgr->spawnSprite("minimap_dead.png");	
	m_dotDead->setHotspotEasy(true, true);

	m_dotQuestStart = sContentMgr->spawnSprite("quest_ready_map.png");
	m_dotQuestStart->setHotspotEasy(true, true);

	m_dotQuestEnd = sContentMgr->spawnSprite("quest_done_map.png");
	m_dotQuestEnd->setHotspotEasy(true, true);

	m_dotGossip = sContentMgr->spawnSprite("gossip_option.png");
	m_dotGossip->setHotspotEasy(true, true);

	// debugging;
	setTitle("Unknown Zone");
	setChannelName("Channel 1");
}

Minimap::~Minimap()
{

}

void Minimap::input() /*final*/
{	
	__super::input();

	m_button->setKeyEvent(sKeybinds->getKeybindData("Map"));
	m_button->attemptInput();
	m_mailLootButton->attemptInput();

	if (m_mailLootButton->popActivated())
	{
		GP_Client_RecoverMailLoot pk;
		sConnector->sendPacket(pk.build(StlBuffer{}));
	}

	if (m_button->popLeftClicked())
	{
		vector<pair<string, sf::Color>> lines;

		for (size_t i = 0; i < m_cachedChannels.size(); ++i)
		{
			string str = Util::fmtStr("Channel #%d (%d/%d)", i + 1, m_cachedChannels[i], m_cachedChannelSizes);
			lines.push_back({ str, sf::Color::White });
		}

		if (RenderObjectHolder* owner = dynamic_cast<RenderObjectHolder*>(getOwner()))
			owner->registerContextMenuEx(World::Interface::ChannelCtxMenu, getId(), { sApplication->mousePos().x - 200, sApplication->mousePos().y }, lines);
	}
	else if (m_button->popKeyBound())
	{
		if (m_minimapTexture == nullptr)
		{
			world().closePanel(World::Interface::MapQuesterPanel);
		}
		else
		{
			world().closeDialogPanels();
			world().togglePanel(World::Interface::MapQuesterPanel);
		}
	}
}

void Minimap::render() /*final*/
{
	__super::render();
	
	getBottomRightCornerRef() = getTopLeftCornerRef() + sf::Vector2i(int(m_bgSpr->getGlobalBounds().width), int(m_bgSpr->getGlobalBounds().height));

	if (m_minimapTexture != nullptr && world().myself() != nullptr)
	{		
		// World position -> Render position -> Location on the 3200x1800 sprite
		Geo2d::Vector2 maptexLoc = world().myself()->getWorldPosition();
		maptexLoc = world().getMap().getCellRenderPosF(maptexLoc.x, maptexLoc.y);
		mapRenderPosToMinimapPixelCord(maptexLoc.x, maptexLoc.y, world().myself()->getMap()->getMapWidth());
		maptexLoc.x -= float(m_mapDecal->getSize().x) / 8;
		maptexLoc.y -= float(m_mapDecal->getSize().y) / 8;

		// Prepare cavas
		if (m_renderTexture.getSize().x == 0)
			m_renderTexture.create(m_mapDecal->getSize().x, m_mapDecal->getSize().y);

		m_renderTexture.clear(sf::Color::Transparent);
				
		// Map
		sf::Sprite overlaySprite(*m_minimapTexture.get());
		overlaySprite.setScale({4.f, 4.f});
		overlaySprite.setPosition(sf::Vector2f(-maptexLoc.x * 4, -maptexLoc.y * 4));
		overlaySprite.setColor(sf::Color(255, 255, 255, 230));
		m_renderTexture.draw(overlaySprite, sf::RenderStates(&sContentMgr->getShader(Assets::ShaderId::Minimap)));

		// Dots
		for (auto& itr : m_dots)
		{
			Geo2d::Vector2 worldPos = itr.second.second;
			worldPos = world().getMap().getCellRenderPosF(worldPos.x, worldPos.y);
			mapRenderPosToMinimapPixelCord(worldPos.x, worldPos.y, world().myself()->getMap()->getMapWidth());
			
			if (auto ptr = getDotSprite(itr.second.first))
			{
				sf::Vector2f myPos(-worldPos.x * 4, -worldPos.y * 4);
				myPos = overlaySprite.getPosition() - myPos;
				ptr->setPosition(myPos - sf::Vector2f(ptr->getHotspot()));
				m_renderTexture.draw(*ptr);
			}
		}

		// Decal
		m_renderTexture.draw(sf::Sprite(*m_mapDecal.get()), sf::BlendMode(sf::BlendMode::Zero, sf::BlendMode::One, sf::BlendMode::Add, sf::BlendMode::DstAlpha, sf::BlendMode::OneMinusSrcAlpha, sf::BlendMode::Subtract));
		m_renderTexture.display();

		// Draw cavas
		sf::Sprite renderSprite(m_renderTexture.getTexture());
		sf::Vector2i drawLoc = getTopLeftCornerRef() + sf::Vector2i(3, 45);	
		renderSprite.setPosition(float(drawLoc.x), float(drawLoc.y));
		sApplication->canvas().draw(renderSprite);
	}

	m_dots.clear();
	m_button->attemptRender();
	m_mailLootButton->attemptRender();

	int textX = getTopLeftCornerRef().x + (mouseableWidth() / 2);
	textX -= int(m_label->getGlobalBounds().width) / 2;
	m_label->draw(textX, getTopLeftCornerRef().y + 15);

	textX = getTopLeftCornerRef().x + (mouseableWidth() / 2);
	textX -= int(m_channelLabel->getGlobalBounds().width) / 2;
	m_channelLabel->draw(textX, getTopLeftCornerRef().y + 300);
}

void Minimap::notifyCtxMenuClicked(const int id, const string& lineClicked) /*final*/
{
	if (lineClicked.empty())
		return;

	if (id >= World::Interface::ChannelCtxMenu)
	{
		uint32_t channelId = 0;
		sscanf(lineClicked.c_str(), "Channel #%u", &channelId);

		GP_Client_ChangeChannels packet;
		packet.m_channelId = channelId - 1;
		sConnector->sendPacket(packet.build(StlBuffer{}));
	}
}

void Minimap::setCachedChannelInfo(const int channelSizes, vector<int> channels)
{
	m_cachedChannelSizes = channelSizes;
	m_cachedChannels = channels;
}

void Minimap::setMailLootStatus(const bool value)
{
	m_mailLootButton->setHidden(!value);
}

bool Minimap::getMailLootStatus() const
{
	return !m_mailLootButton->isHidden();
}

Sprite* Minimap::getDotSprite(const DotType dot)
{
	switch (dot)
	{		
		case Minimap::DotType::EnemyUnit: return m_dotEnemy.get();
		case Minimap::DotType::NeutralUnit: return m_dotNeutral.get();
		case Minimap::DotType::FriendlyUnit: return m_dotFriendly.get();
		case Minimap::DotType::DeadUnit: return m_dotDead.get();
		case Minimap::DotType::QuestStart: return m_dotQuestStart.get();
		case Minimap::DotType::QuestComplete: return m_dotQuestEnd.get();
		case Minimap::DotType::GossipAvailable: return m_dotGossip.get();
	}

	return nullptr;
}

void Minimap::registerDot(const int guid, const Geo2d::Vector2& worldPos, const Minimap::DotType dot)
{
	m_dots[guid] = { dot, worldPos };
}

void Minimap::setChannelName(const string& channelName)
{
	m_channelLabel->setOriginalString(Util::toLowerCase(channelName));
}

void Minimap::setTitle(const string& zonename)
{
	m_label->setOriginalString(Util::toLowerCase(zonename));
}

void Minimap::setMap(const string& mapname)
{
	if (m_mapname == mapname)
		return;

	m_mapname = mapname;

	if (m_minimapTexture = sContentMgr->getTexture(Util::fmtStr("%s_map.jpg", mapname.c_str())))
		m_minimapTexture->setSmooth(true);

	setTitle(mapname);
}
		
void Minimap::minimapPixelCordToMapRenderPos(float& x, float& y, const int mapWidth)
{
	float fullW = float(mapWidth) * GameMap::BaseCellWidth;
	float scale = Minimap::MapPixelWidth / fullW;
	
	y -= 64.f;
	x -= float(Minimap::MapPixelWidth) / 2.f;
	y /= scale;
	x /= scale;
}
		
void Minimap::mapRenderPosToMinimapPixelCord(float& x, float& y, const int mapWidth)
{
	float fullW = float(mapWidth) * GameMap::BaseCellWidth;
	float scale = Minimap::MapPixelWidth / fullW;

	x += fullW / 2.f;
	x *= scale;
	y *= scale;
	y += 64.f;
}