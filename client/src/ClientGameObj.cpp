#include "stdafx.h"
#include "ClientGameObj.h"
#include "ContentMgr.h"
#include "Sprite.h"
#include "SpriteScript.h"
#include "ClientMap.h"
#include "SpriteAnimation.h"
#include "ClientObjectOverhead.h"
#include "Application.h"
#include "Connector.h"
#include "SpellVisualKit.h"
#include "Tooltip.h"
#include "World.h"
#include "ClientPlayer.h"

#include "..\StringHelpers.h"
#include "..\Shared\SpellDefines.h"
#include "..\Shared\GamePacketClient.h"
#include "..\SqlConnector\QueryResult.h"

ClientGameObj::ClientGameObj(const int guid, const int entry, ClientMap* clientMap) :
	ClientObject(guid, clientMap),
	m_entry(entry)
{	
	m_overheads = make_unique<ClientObjectOverhead>(*this);
	initType(ClientObject::GameObject);
	setName(sContentMgr->db("gameobject_template").data(entry, "name"));
	setToggleState(GoDefines::Closed);	
	m_goType = (GoDefines::Type)atoi(sContentMgr->db("gameobject_template").data(entry, "type").c_str());
}

ClientGameObj::~ClientGameObj()
{

}

void ClientGameObj::update() /*final*/
{
	__super::update();

	if (interactable())
	{
		if (popClickedOnInWorld(sf::Mouse::Right))
		{
			GP_Client_CastSpell packet;
			packet.m_targetGuid = getGuid();
			packet.m_spellId = SpellDefines::StaticSpells::LootGameObj;
			sConnector->sendPacket(packet.build(StlBuffer{}));

			if (getWorld() != nullptr)
			{
				int model = atoi(sContentMgr->db("gameobject_template").data(getEntry(), "model").c_str());
				getWorld()->setNextCastSound(packet.m_spellId, sContentMgr->db("gameobject_models").data(model, "sound_use"));
			}
		}
	}
}

void ClientGameObj::render() /*final*/
{
	__super::render();

	uint8_t modelAlpha = uint8_t(255.f * getAlphaPct() * getFadeInPct());	
	sf::Vector2i screenPos = getScreenPosition();

	if (interactable() && mousedInWorld())
	{
		sApplication->queueCursor("cursor_loot.png");

		// First come first serve, same as popClickedOnInWorld
		if (!sApplication->isTooltipRegistered())
			useTooltip();
	}

	drawAuras_below();

	if (m_model != nullptr)
	{	
		float brightenPct = 0.f;

		// Brighten when moused over
		if (interactable() && isMousedOver() && !Util::maskHas(getVariable(ObjDefines::Variable::GoFlags), GoDefines::Flags::NoMouseoverBrighten))
			brightenPct = 0.1f;
		
		m_model->setColor(sf::Color(255, 255, 255, modelAlpha));
		m_model->renderScript(sf::Vector2f(screenPos), false, true, brightenPct);

		m_topLeftCorner = screenPos - m_model->getHotspot();
		m_bottomRightCorner = m_topLeftCorner + sf::Vector2i(static_cast<int>(m_model->getGlobalBounds().width), static_cast<int>(m_model->getGlobalBounds().height));
	}
	
	if (m_spriteAnimation != nullptr)
	{
		m_topLeftCorner = screenPos - sf::Vector2i(m_spriteAnimation->getSize() / 2, m_spriteAnimation->getSize() / 2) + (m_sprAnimOffset/  2);
		m_bottomRightCorner = screenPos + sf::Vector2i(m_spriteAnimation->getSize() / 2, m_spriteAnimation->getSize() / 2) + (m_sprAnimOffset / 2);

		m_spriteAnimation->setColor(sf::Color(255, 255, 255, modelAlpha));
		m_spriteAnimation->render(getScreenPosition() + m_sprAnimOffset);
	}
	
	drawAuras_top();
		
	if (getMap() != nullptr && ((m_model == nullptr && m_spriteAnimation == nullptr) || Util::maskHas(getVariable(ObjDefines::Variable::GoFlags), GoDefines::Flags::ShowName)))
	{		
		if (interactable())
		{
			m_overheads->setAlpha(modelAlpha);
			getMap()->registerObjectOverhead(m_overheads);
		}
	}

	initHeight();
}

void ClientGameObj::useTooltip()
{
	if (getWorld() == nullptr)
		return;

	if (m_tooltip == nullptr)
	{
		m_tooltip = make_shared<Tooltip>(*getWorld(), sf::Vector2i{});
		m_tooltip->setShadowOffset(1);
		m_tooltip->addLine(FontId::Arial, 15, getName(), getNameColor());

		if (!getSubName().empty())
			m_tooltip->addLine(FontId::Arial, 15, getSubName());

		if (Util::maskHas(getVariable(ObjDefines::Variable::GoFlags), GoDefines::Flags::Lockpicking))
		{
			int requiredSkill = getVariable(ObjDefines::Variable::LockpickingLevel);
			int mySkill = 0;

			if (getWorld()->myself() != nullptr)
				mySkill = getWorld()->myself()->getStat(UnitDefines::Stat::Lockpicking);

			sf::Color col = sf::Color(181, 0, 175, 255);
			
			if (requiredSkill > mySkill)
				col = sf::Color::Red;
			else
				col = sf::Color::Green;

			m_tooltip->addLine(FontId::Arial, 15, Util::fmtStr("Lockpicking (%d)", requiredSkill), col);
		}
		
		int requiredItemId = atoi(sContentMgr->db("gameobject_template").data(getEntry(), "required_item").c_str());

		if (requiredItemId != 0)
		{
			string requiredItemName = "Unknown";

			auto testStr = sContentMgr->db("item_template").data(requiredItemId, "name");

			if (!testStr.empty())
				requiredItemName = testStr;

			m_tooltip->addLine(FontId::Arial, 15, Util::fmtStr("Requires [%s]", requiredItemName.c_str()), sf::Color::Yellow);
		}
	}
	
	m_tooltip->moveTo(sApplication->mousePos() + sf::Vector2i(50, 0 ));
	sApplication->setTooltip(m_tooltip);
}
		
bool ClientGameObj::interactable() const
{
	return getVariable(ObjDefines::Variable::DynInteractable);
}

bool ClientGameObj::isRenderedOnLayer(const ClientMap::Defines layer) /*final*/
{
	if (Util::maskHas(getVariable(ObjDefines::Variable::GoFlags), GoDefines::Flags::RenderOnFloor))
		return layer == ClientMap::Defines::Layer2;

	return __super::isRenderedOnLayer(layer);
}

void ClientGameObj::notifySubnameChanged() /*final*/
{
	m_overheads->refreshName();
}

void ClientGameObj::notifyNameChanged() /*final*/
{
	m_overheads->refreshName();
}

void ClientGameObj::addFlag(const GoDefines::Flags flag)
{
	int maskval = 1 << (flag - 1);
	int curMask = getVariable(ObjDefines::Variable::GoFlags);
	curMask |= maskval;
	setVariable(ObjDefines::Variable::GoFlags, curMask);
}

void ClientGameObj::setCurrentCell(const int c, MapCellClient& cell) /*final*/
{
	__super::setCurrentCell(c, cell);

	// Things we do near certain objects
	if (getGoType() == GoDefines::Type::Waypoint)
	{
		for (auto& itr : cell.getRenderablesRef())
		{
			if (auto player = dynamic_pointer_cast<ClientPlayer>(itr))
			{
				if (player->isLocal())
					player->setStandingWaypoint(*this);
			}
		}
	}
}

void ClientGameObj::setToggleState(const GoDefines::ToggleState state)
{
	setVariable(ObjDefines::Variable::ToggleState, state);
}

void ClientGameObj::notifyVariableChange(const ObjDefines::Variable var, const int value) /*final*/
{
	__super::notifyVariableChange(var, value);

	switch (var)
	{
		case ObjDefines::Variable::GoFlags:
		{
			m_tooltip = nullptr;
			break;
		}
		case ObjDefines::Variable::ToggleState:
		{
			int modelId = atoi(sContentMgr->db("gameobject_template").data(getEntry(), "model").c_str());

			string nameModel;
			string nameSpriteAnim;
			SpellVisualKit::BlendMode bm = SpellVisualKit::BlendMode::BlendAlpha;

			if (value == GoDefines::Closed)
			{
				nameSpriteAnim = sContentMgr->db("gameobject_models").data(modelId, "anim_name_closed");
				nameModel = sContentMgr->db("gameobject_models").data(modelId, "name_closed");
				bm = SpellVisualKit::BlendMode(atoi(sContentMgr->db("gameobject_models").data(modelId, "anim_closed_blend").c_str()));
				m_sprAnimOffset.x = atoi(sContentMgr->db("gameobject_models").data(modelId, "anim_closed_offset_x").c_str());
				m_sprAnimOffset.y = atoi(sContentMgr->db("gameobject_models").data(modelId, "anim_closed_offset_y").c_str());
			}
			else if (value == GoDefines::Open)
			{
				nameSpriteAnim = sContentMgr->db("gameobject_models").data(modelId, "anim_name_open");
				nameModel = sContentMgr->db("gameobject_models").data(modelId, "name_open");
				bm = SpellVisualKit::BlendMode(atoi(sContentMgr->db("gameobject_models").data(modelId, "anim_open_blend").c_str()));			
				m_sprAnimOffset.x = atoi(sContentMgr->db("gameobject_models").data(modelId, "anim_open_offset_x").c_str());
				m_sprAnimOffset.y = atoi(sContentMgr->db("gameobject_models").data(modelId, "anim_open_offset_y").c_str());
			}
			
			if (!nameModel.empty())
				m_model = sContentMgr->spawnSprite(nameModel.c_str());
			else
				m_model = nullptr;

			if (!nameSpriteAnim.empty())
				m_spriteAnimation = sContentMgr->spawnSpriteAnimation(nameSpriteAnim);
			else
				m_spriteAnimation = nullptr;

			if (m_spriteAnimation != nullptr)
				m_spriteAnimation->setBlendMode(SpellVisualKit::getSfBlend(bm));
			
			break;
		}
	}
}
		
void ClientGameObj::playToggleSound(const GoDefines::ToggleState newState)
{
	string soundName;

	if (newState == GoDefines::Closed)
		soundName = sContentMgr->db("gameobject_models").data(m_entry, "name_closed");
	else if (newState == GoDefines::Open)
		soundName = sContentMgr->db("gameobject_models").data(m_entry, "name_open");

	if (!soundName.empty())
		playSound(soundName);
}