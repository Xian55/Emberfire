#pragma once

#include "ClientUnit.h"

#include "..\Shared\PlayerDefines.h"

class ClientGameObj;

class ClientPlayer : public ClientUnit
{
	public:
		ClientPlayer(const int guid, ClientMap* clientMap, const PlayerDefines::Classes classId, const PlayerDefines::Gender gender, const int portrait);
		virtual ~ClientPlayer();

		virtual void render() override;

		void setEquipment(const UnitDefines::EquipSlot slot, const ItemDefines::ItemDefinition itemId);
		void clearEquipment(const UnitDefines::EquipSlot slot);
		void clearEquipmentModel(const UnitDefines::EquipSlot slot);
		void resetEquipment();
		void queueDiedPopup();
		void setStandingWaypoint(ClientGameObj& gameObj);

		// Your class will never change, but perhaps we might have a game spell/ability that morphs your appearance. 
		void setGender(const PlayerDefines::Gender gender);

		bool hasBrokenEquipment() const;

		auto getGender() const { return m_gender; }
		auto getClass() const { return m_class; }
		auto getPortrait() const { return m_portrait; }
		auto getEquipmentItemId(const UnitDefines::EquipSlot e) { return m_equipment[e]; }
		auto getWaypointStandingGuid() const { return m_waypointStandingGuid; }

		UnitDefines::Faction faction() const final;

	private:
		void died() final;
		void playSoundMelee(ClientUnit& victim) final;
		void notifyVariableChange(const ObjDefines::Variable var, const int value) final;
		void setCurrentCell(const int c, MapCellClient& cell) final;

		bool playSoundDamaged() final;

		string formBodyPartName(const string& str) const;

		sf::Color getNameColor() const final;

		clock_t m_lastDamagedTime{0};

		string m_defaultHead;
		float m_diedPopupTimer;
		
		int m_waypointStandingGuid{0};

		PlayerDefines::Gender m_gender;
		const PlayerDefines::Classes m_class;
		const int m_portrait;

		unordered_map<UnitDefines::EquipSlot, ItemDefines::ItemDefinition> m_equipment;
};

