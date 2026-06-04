#include "stdafx.h"
#include "ClientPlayer.h"
#include "ContentMgr.h"
#include "Application.h"
#include "ConfirmMessageBox.h"
#include "Minimap.h"
#include "World.h"
#include "ClientObjectOverhead.h"
#include "Inventory.h"
#include "VendorNpc.h"
#include "ClientGameObj.h"
#include "Connector.h"
#include "GameChat.h"

#include "..\Rand.h"
#include "..\SqlConnector\QueryResult.h"
#include "..\Shared\GamePacketClient.h"
#include "..\Shared\GameDefines.h"

ClientPlayer::ClientPlayer(const int guid, ClientMap* clientMap, const PlayerDefines::Classes classId, const PlayerDefines::Gender gender, const int portrait) :
	ClientUnit(guid, clientMap),
	m_class(classId),
	m_portrait(portrait),
	m_diedPopupTimer(0)
{
	initType(ClientObject::Player);
	setGender(gender);

	// Default body
	clearEquipment(UnitDefines::EquipSlot::Chest);
	clearEquipment(UnitDefines::EquipSlot::Feet);
	clearEquipment(UnitDefines::EquipSlot::Hands);
	clearEquipment(UnitDefines::EquipSlot::Legs);
	clearEquipment(UnitDefines::EquipSlot::Helm);

}

ClientPlayer::~ClientPlayer()
{

}

/*virtual*/
void ClientPlayer::render() /*override*/
{
	__super::render();

	if (m_diedPopupTimer > 0)
	{
		m_diedPopupTimer -= sApplication->delta();

		if (m_diedPopupTimer <= 0)
			sApplication->spawnPopup("You have died. Respawn?", ConfirmMessageBox::ConfirmBox_Ok, ConfirmMessageBox::ConfirmCode_Respawn);
	}
}

void ClientPlayer::setCurrentCell(const int c, MapCellClient& cell) /*final*/
{
	__super::setCurrentCell(c, cell);

	if (!isLocal())
		return;

	m_waypointStandingGuid = 0;

	// Things we do near certain objects
	for (auto& itr : cell.getRenderablesRef())
	{
		if (auto gameObj = dynamic_pointer_cast<ClientGameObj>(itr))
		{
			if (gameObj->getGoType() == GoDefines::Type::Waypoint)
				setStandingWaypoint(*gameObj);
		}
	}
}

void ClientPlayer::setStandingWaypoint(ClientGameObj& gameObj)
{
	if (m_waypointStandingGuid == gameObj.getGuid())
		return;

	m_waypointStandingGuid = gameObj.getGuid();

	if (hasSpline())
		sContentMgr->playSound("alert_cooltime_over_a.ogg");
}

void ClientPlayer::notifyVariableChange(const ObjDefines::Variable var, const int value) /*final*/
{
	__super::notifyVariableChange(var, value);

	switch (var)
	{
		case ObjDefines::Variable::Money:
		{
			if (!isLocal() || getWorld() == nullptr)
				break;

			dynamic_pointer_cast<Inventory>(getWorld()->getRenderObject(World::Interface::InventoryPanel))->setMoney(value);
			dynamic_pointer_cast<VendorNpc>(getWorld()->getRenderObject(World::Interface::VendorNpcPanel))->setLocalMoney(value);
			break;
		}
		case ObjDefines::Variable::Level:
		{
			getOverhead()->refreshName();
			break;
		}
		case ObjDefines::Variable::InCombat:
		{
			if (!isLocal() || getWorld() == nullptr)
				break;
			
			string msg;

			if (value != 0)
				msg = "Entering Combat";
			else
				msg = "Leaving Combat";

			getWorld()->pushCombatMessage(msg, sf::Vector2i(sApplication->centerOfScreen().x, sApplication->centerOfScreen().y + 150), sf::Color(255, 0, 0, 125), false, 0.5f);
			break;
		}
		case ObjDefines::Variable::MailLoot:
		{
			if (!isLocal() || getWorld() == nullptr)
				break;

			auto minimap = dynamic_pointer_cast<Minimap>(getWorld()->getRenderObject(World::MinimapObj));
			minimap->setMailLootStatus(value != 0);

			if (value != 0)
			{
				if (auto world = getWorld())
				{
					if (auto gameChat = dynamic_pointer_cast<GameChat>(world->getRenderObject(World::GameChatBox)))
					{
						string msg = "You have mail. Check your minimap.";
						gameChat->addLine(msg, ChatDefines::Channels::System);
						world->pushScrollingMessage(msg, GameChat::getChatColor(ChatDefines::Channels::SystemCenter));
					}
				}

				sContentMgr->queueSound("alert_mail_a.ogg", 2000);
			}

			break;
		}
		case ObjDefines::Variable::GameMaster:
		{
			string name = getName();

			// if has GM tag, remove it
			if (value != 0 && name.find("[GM] ") == string::npos)
				setName("[GM] " + name);
			else if (value == 0 && name.find("[GM] ") != string::npos)
				setName(name.substr(5));

			getOverhead()->refreshName();
			break;
		}
		case ObjDefines::Variable::GmInvisible:
		{
			string name = getName();

			if (value != 0 && name.find(" [I]") == string::npos)
				setName(name + " [I]");
			else if (value == 0 && name.find(" [I]") != string::npos)
				setName(name.substr(0, name.length() - 4));

			getOverhead()->refreshName();
			break;
		}
	}
}

void ClientPlayer::setEquipment(const UnitDefines::EquipSlot slot, const ItemDefines::ItemDefinition itemId)
{
	m_equipment[slot] = itemId;

	if (itemId.m_itemId == 0)
	{
		clearEquipment(slot);
		return;
	}

	BodyPart bpart;

	switch (slot)
	{
		case UnitDefines::EquipSlot::Chest: bpart = BodyPart::Torso; break;
		case UnitDefines::EquipSlot::Feet: bpart = BodyPart::Feet; break;
		case UnitDefines::EquipSlot::Hands: bpart = BodyPart::Hands; break;
		case UnitDefines::EquipSlot::Legs: bpart = BodyPart::Legs; break;
		case UnitDefines::EquipSlot::Helm: bpart = BodyPart::Head; break;
		case UnitDefines::EquipSlot::Weapon1: bpart = BodyPart::Weapon; break;
		case UnitDefines::EquipSlot::Offhand: bpart = BodyPart::Offhand; break;
		case UnitDefines::EquipSlot::Ranged: bpart = BodyPart::WeaponRanged; break;
		default: 
			return;
	}

	if (slot == UnitDefines::EquipSlot::Ranged)
		setSheath(BodyPart::WeaponRanged);

	const string modelName = sContentMgr->db("item_template").data(itemId.m_itemId, "model");

	if (modelName.size() > 0)
		setBodyPart(formBodyPartName(modelName), bpart);
	else
		clearEquipmentModel(slot);
}
		
void ClientPlayer::queueDiedPopup() 
{ 
	m_diedPopupTimer = 2.f; 
}

void ClientPlayer::died() /*final*/
{
	if (isLocal())
		queueDiedPopup();

	if (getGender() == PlayerDefines::Gender::Male)
		playSound("vdamage3_mlb_4.ogg", 1.f, getCombatVolume());
	else
		playSound("vdamage3_flc_1.ogg", 1.f, getCombatVolume());
}

bool ClientPlayer::playSoundDamaged() /*final*/
{
	if (std::clock() - m_lastDamagedTime > 5000)
	{
		if (getGender() == PlayerDefines::Gender::Male)
			playSound(Util::randomChoice("vdefence_mlb_1.ogg", "vdefence_mlb_2.ogg", "vdefence_mlb_3.ogg", "vdefence_mlb_4.ogg"), 1.f, getCombatVolume() * 0.75f, 100);
		else
			playSound(Util::randomChoice("vdefence_flc_1.ogg", "vdefence_flc_2.ogg", "vdefence_flc_3.ogg", "vdefence_flc_4.ogg"), 1.f, getCombatVolume() * 0.75f, 100);

		m_lastDamagedTime = std::clock();
	}

	return true;
}

void ClientPlayer::playSoundMelee(ClientUnit& victim) /*final*/
{
	int equipmentId = getEquipmentItemId(UnitDefines::Weapon1).m_itemId;
	int equipType = equipmentId ? atoi(sContentMgr->db("item_template").data(equipmentId, "weapon_type").c_str()) : 0;

	if (equipmentId == 0)
	{
		// Unarmed
		victim.playSound(Util::randomChoice("attack_hit_var01.wav", "attack_hit_var02.wav", "attack_hit_var03.wav", "attack_hit_var04.wav", "attack_hit_var05.wav"), 1.f, getCombatVolume());
	}
	else if (equipType == ItemDefines::WeaponType::Dagger || equipType == ItemDefines::WeaponType::Sword || equipType == ItemDefines::WeaponType::Axe)
	{
		// Sharp
		victim.playSound(Util::randomChoice("attack_metal_hit_var01.wav", "attack_metal_hit_var02.wav", "attack_metal_hit_var03.wav", "attack_metal_hit_var04.wav", "attack_metal_hit_var05.wav"), 1.f, getCombatVolume());
	}
	else
	{
		// Blunt
		victim.playSound(Util::randomChoice(string("attack_sword_normal_h.ogg"), string("attack_sword_normal_m.ogg"), string("attack_sword_normal_m2.ogg"), string("attack_sword_normal_s.ogg")), 1.f, getCombatVolume());
	}
}

void ClientPlayer::clearEquipmentModel(const UnitDefines::EquipSlot slot)
{
	switch (slot)
	{
		case UnitDefines::EquipSlot::Chest: setBodyPart(formBodyPartName("default_chest"), BodyPart::Torso); break;
		case UnitDefines::EquipSlot::Feet: setBodyPart(formBodyPartName("default_feet"), BodyPart::Feet); break;
		case UnitDefines::EquipSlot::Hands: setBodyPart(formBodyPartName("default_hands"), BodyPart::Hands); break;
		case UnitDefines::EquipSlot::Legs: setBodyPart(formBodyPartName("default_legs"), BodyPart::Legs); break;
		case UnitDefines::EquipSlot::Helm: setBodyPart(m_defaultHead, BodyPart::Head); break;
		case UnitDefines::EquipSlot::Weapon1: clearBodyPart(BodyPart::Weapon); break;
		case UnitDefines::EquipSlot::Offhand: clearBodyPart(BodyPart::Offhand); break;
		case UnitDefines::EquipSlot::Ranged: clearBodyPart(BodyPart::WeaponRanged); break;
	}
}

void ClientPlayer::clearEquipment(const UnitDefines::EquipSlot slot)
{
	auto itr = m_equipment.find(slot);

	if (itr != m_equipment.end())
		m_equipment.erase(itr);

	clearEquipmentModel(slot);
	setSheath(BodyPart::Weapon);
}

bool ClientPlayer::hasBrokenEquipment() const
{
	for (auto& itr : m_equipment)
	{
		if (itr.second.m_durability == 0)
		{
			auto& it = sContentMgr->db("item_template");
			const int maxDurability = atoi(it.data(itr.second.m_itemId, "durability").c_str());

			if (maxDurability > 0)
				return true;
		}
	}

	return false;
}

void ClientPlayer::resetEquipment()
{
	auto itr = m_equipment.begin();

	while (itr != m_equipment.end())
	{
		clearEquipment(itr->first);
		itr = m_equipment.begin();
	}
}

void ClientPlayer::setGender(const PlayerDefines::Gender gender)
{
	m_gender = gender;

	if (m_gender == PlayerDefines::Gender::Male)
		m_defaultHead = "player_male_head_short";
	else
		m_defaultHead = "player_female_head_long";
}

UnitDefines::Faction ClientPlayer::faction() const
{
	if (getWorld()->myself() == this && getVariable(ObjDefines::Variable::ZoneType) == GameDefines::ZoneTypes::Contested)
	{
		if (getVariable(ObjDefines::PvP) != 0)
			return UnitDefines::Faction::PvP;
	}	

	return UnitDefines::Faction::PlayerDefault;
}

string ClientPlayer::formBodyPartName(const string& str) const
{
	if (m_gender == PlayerDefines::Gender::Male)
		return "player_male_" + str;

	return "player_female_" + str;
}

sf::Color ClientPlayer::getNameColor() const
{
	if (getWorld() != nullptr && getWorld()->myself() != nullptr)
	{
		if (getWorld()->myself()->seesAsHostile(*this))
			return __super::getFactionColor(UnitDefines::Faction::PvP);
	}

	// Party members are a light blue
	if (getWorld() != nullptr && getWorld()->isPartyMember(getGuid()))
		return sf::Color(0xfcb4ffff);

	return __super::getNameColor();
}

