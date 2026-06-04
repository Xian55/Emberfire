#pragma once

#include "ClientUnit.h"

class ClientNpc : public ClientUnit
{
	public:
		ClientNpc(const int guid, ClientMap* clientMap);
		virtual ~ClientNpc();

		int getEntry() const final { return m_entry; }
		
	public:
		void setEntry(const int e);
		void render() final;	

		UnitDefines::Faction faction() const final { return m_faction; }

	private:
		void notifyVariableChange(const ObjDefines::Variable var, const int value) final;
		void playSoundMelee(ClientUnit& victim) final;

		bool playSoundDamaged() final;
		bool playDeathSound() final;
		bool playSoundAttack() final;
		bool playSoundAggro() final;

		vector<string> getSounds(const string& eventName);

		int m_entry{0};
		clock_t m_lastDamagedTime{0};
		clock_t m_lastAttackTime{0};
		string m_lastDamageSound;
		UnitDefines::Faction m_faction;
};

