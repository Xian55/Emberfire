#include "stdafx.h"
#include "ClientNpc.h"
#include "ContentMgr.h"

#include "..\Rand.h"
#include "..\SqlConnector\QueryResult.h"

ClientNpc::ClientNpc(const int guid, ClientMap* clientMap) :
	ClientUnit(guid, clientMap),
	m_faction(UnitDefines::Faction::Neutral)
{
	initType(ClientObject::Npc);
}

ClientNpc::~ClientNpc()
{

}

void ClientNpc::render() /*final*/
{
	__super::render();
}

void ClientNpc::setEntry(const int e)
{
	m_entry = e;

	setName(sContentMgr->db("npc_template").data(e, "name"));
	setSubName(sContentMgr->db("npc_template").data(e, "subname"));

	// Faction comes from npc_template (1=Friendly,2=Hostile,3=Neutral). Without this,
	// m_faction defaulted to 0 -> getFactionColor() red, so every NPC looked hostile
	// (Vincent=faction1=Friendly wrongly showed as an enemy). A server FactionId var can
	// still override for dynamic reactions.
	m_faction = static_cast<UnitDefines::Faction>(atoi(sContentMgr->db("npc_template").data(e, "faction").c_str()));
}

void ClientNpc::notifyVariableChange(const ObjDefines::Variable var, const int value) /*final*/
{
	__super::notifyVariableChange(var, value);

	// REMOVED the var-0x07 ("FactionId", a GUESS) -> m_faction override: in combat the server sends
	// 0x07 with a value the client read as Friendly(1) -> the enemy turned GREEN mid-fight -> offensive
	// spells greyed (can't target a "friendly") -> only Melee Swing worked. Faction is authoritative
	// from npc_template (setEntry: Antling=Hostile, Vincent=Friendly), so the enemy stays red in combat.
	// (The real per-viewer reaction var, if any, must be identified from a live combat capture before
	// re-adding any var->faction override.)
}

void ClientNpc::playSoundMelee(ClientUnit& victim) /*final*/
{
	victim.playSound(Util::randomChoice("attack_hit_var01.wav", "attack_hit_var02.wav", "attack_hit_var03.wav", "attack_hit_var04.wav", "attack_hit_var05.wav"), 1.f, getCombatVolume());
}

bool ClientNpc::playSoundAggro() /*final*/
{
	auto sounds = getSounds("aggro");

	if (!sounds.empty())
	{
		int soundIdx = Util::irand(0, sounds.size() - 1);
		playSound(sounds[soundIdx]);
	}

	return !sounds.empty();
}

bool ClientNpc::playSoundAttack() /*final*/
{
	auto sounds = getSounds("attack");

	if (std::clock() - m_lastAttackTime > 5000)
	{
		if (!sounds.empty())
		{
			int soundIdx = Util::irand(0, sounds.size() - 1);
			playSound(sounds[soundIdx]);
		}
			
		m_lastAttackTime = std::clock();
	}

	return !sounds.empty();
}

bool ClientNpc::playDeathSound() /*final*/
{
	auto sounds = getSounds("die");

	if (!sounds.empty())
	{
		int soundIdx = Util::irand(0, sounds.size() - 1);
		playSound(sounds[soundIdx], 1.f, 1.f, 200);
	}
	
	return !sounds.empty();
}

bool ClientNpc::playSoundDamaged() /*final*/
{
	auto sounds = getSounds("damage");

	if (std::clock() - m_lastDamagedTime > 5000)
	{
		if (!sounds.empty())
		{
			// Avoid playing the same sound back to back, but that only makes sense if we have more than 1 sound
			if (sounds.size() > 1)
			{
				for (auto itr = sounds.begin(); itr != sounds.end(); ++itr)
				{
					if (*itr == m_lastDamageSound)
					{
						sounds.erase(itr);
						break;
					}
				}
			}

			int soundIdx = Util::irand(0, sounds.size() - 1);
			m_lastDamageSound = sounds[soundIdx];
			playSound(sounds[soundIdx], 1.f, 1.f, 100);
			m_lastDamagedTime = std::clock();
		}
	}

	return !sounds.empty();
}

vector<string> ClientNpc::getSounds(const string& eventName)
{
	vector<string> result;
	const string myModelName = sContentMgr->db("npc_models").data(getVariable(ObjDefines::ModelId), "name");
	auto& data = sContentMgr->db("npc_sounds").fetchData();

	for (auto& subItr : data)
	{
		if (subItr.size() < 3)
			continue;

		const string& modelName = subItr[0].getString();
		const string& itrEventName = subItr[1].getString();
		const string& soundName = subItr[2].getString();

		if (modelName == myModelName && eventName == itrEventName)
			result.push_back(soundName);
	}

	return result;
}