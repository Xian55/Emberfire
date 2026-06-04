#include "stdafx.h"
#include "ClientObject.h"
#include "Application.h"
#include "Game.h"
#include "World.h"
#include "AuraAnimation.h"
#include "ContentMgr.h"

#include "..\SqlConnector\QueryResult.h"
#include "..\Shared\SpellDefines.h"

#include <assert.h>

ClientObject::ClientObject(const int guid, ClientMap* clientMap) :
	MutualObject(guid),
	WorldRenderable(clientMap),
	m_height(0),
	m_respectApplicationMouseover(true)
{

}

ClientObject::~ClientObject()
{

}
		
void ClientObject::render() /*override*/
{
	__super::render();

	m_fadeInPct += sApplication->delta();

	if (m_fadeInPct >= 1.f)
		m_fadeInPct = 1.f;
}
		
void ClientObject::update() /*override*/
{
	__super::update();

	updateAuraAnimations(m_auraAnimationsOnTop);
	updateAuraAnimations(m_auraAnimationsBelow);
}

void ClientObject::drawAuras_below()
{
	if (!m_auraAnimationsBelow.empty())
	{
		for (auto& itr : m_auraAnimationsBelow)
			itr.second->render();
	}
}

void ClientObject::updateAuraAnimations(map<int, shared_ptr<AuraAnimation>>& m)
{
	auto itr = m.begin();

	while (itr != m.end())
	{
		auto& auraAnimation = itr->second;

		// Let an animation finish up remaining particles and/or frames once it's stopped.
		if (auraAnimation->finished())
		{
			itr = m.erase(itr);
		}
		else
		{
			if (hasAura(itr->first))
			{
				if (auraAnimation->isStopped())
					auraAnimation->resume();
			}
			else
			{
				if (!auraAnimation->isStopped())
					auraAnimation->stop();
			}

			auraAnimation->update();
			++itr;
		}
	}
}

void ClientObject::drawAuras_top()
{
	for (auto& itr : m_auraAnimationsOnTop)
		itr.second->render();
}

bool ClientObject::isMousedOver(const bool useRealPosition /*= false*/) /*override*/
{ 
	if (m_respectApplicationMouseover && sApplication->getTopMostMousedOver() != 0)
		return false;

	return __super::isMousedOver(useRealPosition); 
}

bool ClientObject::mousedInWorld() 
{
	if (!isMousedOver(true))
		return false;

	auto world = getWorld();

	if (world == nullptr)
		return false;

	auto game = dynamic_pointer_cast<Game>(sApplication->getRenderObject(Application::RoGame));

	if (game == nullptr)
		return false;

	// Options menu is open
	if (game->getRenderObject(Game::RoOptions) != nullptr)
		return false;

	world->setLastMousedGuid(getGuid());
	return true;
}

void ClientObject::fillAuraAnimation(const int spellId, const int kit, map<int, shared_ptr<AuraAnimation>>& m)
{
	auto itr = m.find(spellId);

	// If we're already playing this aura animation, then don't restart.
	// Do, however, resume if it was stopped due to pending removal.
	if (itr != m.end())
	{
		itr->second->resume();
	}
	else
	{
		auto ptr = make_shared<AuraAnimation>(kit, *this);
		ptr->setUseHolderPositionForVolume(!(isLocal() || getWorld() == nullptr || getWorld()->getSelectedGuid() == getGuid()));
		ptr->resume();
		m[spellId] = ptr;
	}
}

void ClientObject::auraAnimationIncrement(const int spellId)
{
	++m_auras[spellId];

	auto& sv = sContentMgr->db("spell_visual");

	if (const int kit = atoi(sv.data(spellId, "aura_kit_ontop").c_str()))
		fillAuraAnimation(spellId, kit, m_auraAnimationsOnTop);

	if (const int kit = atoi(sv.data(spellId, "aura_kit_below").c_str()))
		fillAuraAnimation(spellId, kit, m_auraAnimationsBelow);
}

void ClientObject::auraAnimationDecrement(const int spellId)
{
	auto itr = m_auras.find(spellId);

	if (itr == m_auras.end())
		return;

	if (--itr->second <= 0)
		m_auras.erase(itr);
}

void ClientObject::clearAuras()
{ 
	m_auras.clear(); 
	m_auraAnimationsOnTop.clear();
	m_auraAnimationsBelow.clear();
}
	
bool ClientObject::isWithinScreenPosition() const
{
	if (getScreenPosition().x < 0 || getScreenPosition().y < 0 || getScreenPosition().x > sApplication->sW()  || getScreenPosition().y > sApplication->sH())
		return false;

	return true;
}

bool ClientObject::hasAura(const int spellId) const
{
	auto itr = m_auras.find(spellId);

	if (itr == m_auras.end())
		return false;

	return itr->second > 0;
}

bool ClientObject::popClickedOnInWorld(const sf::Mouse::Button key)
{
	if (!mousedInWorld())
		return false;

	if (sApplication->mouseUp(key))
	{
		sApplication->clearMouseUp();
		return true;
	}

	return false;
}

void ClientObject::initHeight()
{
	int newHeight = max(50, __super::mouseableHeight());

	if (abs(newHeight - m_height) > 20)
		m_height = newHeight;
}
		
void ClientObject::notifyVariableChange(const ObjDefines::Variable var, const int value) /*override*/
{
	__super::notifyVariableChange(var, value);

	switch (var)
	{
		case ObjDefines::Variable::DynLootable:
		case ObjDefines::Variable::DynSparkle:
		{
			if (value && !hasAura(SpellDefines::StaticSpells::LootVisual))
				auraAnimationIncrement(SpellDefines::StaticSpells::LootVisual);
			else if (!value)
				auraAnimationDecrement(SpellDefines::StaticSpells::LootVisual);

			break;	
		}
		case ObjDefines::Variable::DynGossipStatus:
		{
			// QuestComplete
			if (value == ObjDefines::GossipStatus::QuestComplete && !hasAura(SpellDefines::QuestDone))
				auraAnimationIncrement(SpellDefines::QuestDone);
			else if (value != ObjDefines::GossipStatus::QuestComplete)
				auraAnimationDecrement(SpellDefines::QuestDone);

			// QuestAvailable
			if (value == ObjDefines::GossipStatus::QuestAvailable && !hasAura(SpellDefines::QuestDone))
				auraAnimationIncrement(SpellDefines::QuestReady);
			else if (value != ObjDefines::GossipStatus::QuestAvailable)
				auraAnimationDecrement(SpellDefines::QuestReady);

			break;	
		}
	}
}