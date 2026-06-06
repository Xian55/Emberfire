#include "stdafx.h"
#include "BuffDebuffRenderer.h"
#include "Connector.h"
#include "SpellIcon.h"
#include "World.h"
#include "ClientPlayer.h"

#include "..\Shared\GamePacketClient.h"

BuffDebuffRenderer::BuffDebuffRenderer(World& world, RenderObject* owner, const int id /*= 0*/) :
	RenderObjectHolder(owner, id),
	m_world(world)
{

}
		
BuffDebuffRenderer::~BuffDebuffRenderer()
{

}
		
void BuffDebuffRenderer::setUnit(shared_ptr<ClientUnit> unit)
{
	m_unit = unit;
}

void BuffDebuffRenderer::setBuffs(const vector<GP_Server_UnitAuras::AuraInfo>& buffs)
{
	if (m_suppressed)
		return;
	setAuras(buffs, Interface::BuffStart, Interface::BuffEnd, highlightBuffDispelType());
}

void BuffDebuffRenderer::setDebuffs(const vector<GP_Server_UnitAuras::AuraInfo>& debuffs)
{
	if (m_suppressed)
		return;
	setAuras(debuffs, Interface::DebuffStart, Interface::DebuffEnd, !highlightBuffDispelType());
}

void BuffDebuffRenderer::setSuppressed(const bool v)
{
	m_suppressed = v;
	if (v)   // hide any currently-shown icons
	{
		setAuras({}, Interface::BuffStart, Interface::BuffEnd, false);
		setAuras({}, Interface::DebuffStart, Interface::DebuffEnd, false);
	}
}

void BuffDebuffRenderer::setAuras(const vector<GP_Server_UnitAuras::AuraInfo>& auras, const int interfaceStart, const int interfaceEnd, const bool renderDispelType)
{
	int interfaceId = interfaceStart;

	for (int i = 0; interfaceId <= interfaceEnd; ++i, ++interfaceId)
	{
		auto ptr = dynamic_pointer_cast<GameIcon>(getRenderObject(interfaceId));

		if (i < int(auras.size()))
		{
			m_auraCasterGuids[static_cast<Interface>(interfaceId)] = auras[i].casterGuid;

			ptr->setHidden(false);
			ptr->setStackCount(auras[i].stackCount);
			ptr->changeEntry(auras[i].spellId);
			ptr->setExpirationTimer(auras[i].startDate, auras[i].endDate);

			// Has to come after the entry is assigned otherwise it can't deduce the color
			ptr->setRenderDispelType(renderDispelType);
		}
		else
		{
			ptr->setHidden(true);
		}		
	}
}
		
void BuffDebuffRenderer::processRightClickedId(const int id)
{
	if (id <= 0)
		return;

	// Right clicking on a buff
	if (shared_ptr<RenderObject> rightClickedButton = getRenderObject(id))
	{
		if (rightClickedButton->getId() >= Interface::BuffStart && rightClickedButton->getId() <= Interface::BuffEnd)
		{
			// Client_CancelBuff has NO wire packet in r1189 (no client serializer in the decompile —
				// buff cancel is server-driven). Its opcode is an out-of-range placeholder; sending it would
				// DROP the connection. Bail before building/sending.
				if (!GamePacket::validOpcode(Opcode::Client_CancelBuff))
					return;
				GP_Client_CancelBuff packet;
			packet.m_spellId = dynamic_pointer_cast<GameIcon>(rightClickedButton)->getEntry();
			packet.m_casterGuid = m_auraCasterGuids[static_cast<Interface>(rightClickedButton->getId())];
			sConnector->sendPacket(packet.build(StlBuffer{}));
		}
	}
}

void BuffDebuffRenderer::refreshDispelTypeLogic()
{
	for (int interfaceId = Interface::BuffStart; interfaceId <= Interface::BuffEnd; ++interfaceId)
	{
		if (auto ptr = dynamic_pointer_cast<GameIcon>(getRenderObject(interfaceId)))
			ptr->setRenderDispelType(highlightBuffDispelType());
	}

	for (int interfaceId = Interface::DebuffStart; interfaceId <= Interface::DebuffEnd; ++interfaceId)
	{
		if (auto ptr = dynamic_pointer_cast<GameIcon>(getRenderObject(interfaceId)))
			ptr->setRenderDispelType(!highlightBuffDispelType());
	}
}

bool BuffDebuffRenderer::highlightBuffDispelType() const
{
	if (m_unit != nullptr)
	{
		if (m_world.myself() != nullptr)
			return !m_world.myself()->seesAsFriendly(*m_unit);

		switch (m_unit->faction())
		{
			case UnitDefines::Faction::PlayerDefault: return false;
			case UnitDefines::Faction::Friendly: return false;
			case UnitDefines::Faction::Neutral: return true;
			case UnitDefines::Faction::Hostile: return true;
			case UnitDefines::Faction::PvP: return true;
		}
	}

	return m_direction == Direction::RightLeft;
}

void BuffDebuffRenderer::createBuffIcon(const Interface id, sf::Vector2i* pAnchor)
{
	int index = id - Interface::BuffStart;

	if (m_direction == Direction::RightLeft)
		index = -index;

	auto gameIcon = make_shared<SpellIcon>(*this, id, m_auraButtonFrame);
	gameIcon->setAnchor(pAnchor);
	gameIcon->setOffset(sf::Vector2i((index * m_auraSpacing.x) + m_aurasOffset.x, m_aurasOffset.y));
	gameIcon->setAllowRightClick(true);
	gameIcon->setHideIfNull(true);
	addRenderObject(gameIcon);
}

void BuffDebuffRenderer::createDebuffIcon(const Interface id, sf::Vector2i* pAnchor)
{
	int index = id - Interface::DebuffStart;

	if (m_direction == Direction::RightLeft)
		index = -index;

	auto gameIcon = make_shared<SpellIcon>(*this, id, m_auraButtonFrame);
	gameIcon->setAnchor(pAnchor);
	gameIcon->setOffset(sf::Vector2i((index * m_auraSpacing.x) + m_aurasOffset.x, m_aurasOffset.y + m_auraSpacing.y));
	gameIcon->setAllowRightClick(true);
	gameIcon->setHideIfNull(true);
	addRenderObject(gameIcon);
}

void BuffDebuffRenderer::registerAuraObjs(sf::Vector2i* pAnchor)
{
	for (int i = Interface::BuffStart; i <= Interface::BuffEnd; ++i)
		createBuffIcon(static_cast<Interface>(i), pAnchor);

	for (int i = Interface::DebuffStart; i <= Interface::DebuffEnd; ++i)
		createDebuffIcon(static_cast<Interface>(i), pAnchor);
}