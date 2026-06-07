#include "stdafx.h"
#include "ItemIcon.h"
#include "ContentMgr.h"
#include "Sprite.h"
#include "Tooltip.h"
#include "Application.h"
#include "World.h"
#include "Text.h"
#include "Keybinds.h"
#include "WorldChild.h"
#include "ClientPlayer.h"
#include "Equipment.h"
#include "TextBox.h"
#include "TooltipGlyph.h"
#include "Abilities.h"

#include "..\SqlConnector\QueryResult.h"
#include "..\Shared\PlayerDefines.h"
#include "..\Shared\ItemDefiner.h"
#include "..\Shared\ItemTemplate.h"
#include "..\Shared\ItemAffix.h"
#include "..\Shared\Config.h"
#include "..\StringHelpers.h"
#include "..\Rand.h"

#include <assert.h>

ItemIcon::ItemIcon(RenderObject& owner, const int id, const string& frame) :
	GameIcon(owner, id, frame, 0)
{
	m_type = GameIcon::Type::Item;
	m_needTooltipRefresh = true;
	m_activationClearsExclaim = false;
	pickIcon();
}

void ItemIcon::setItemDef(const ItemDefines::ItemDefinition def)
{ 
	// The new itemdef is needed before changeEntry happens, because it will call pickIcon and other child member functions
	bool defChanged = !def.equalsExact(m_itemDef);   // exact: catch gem/enchant/durability changes (same entry)
	m_itemDef = def;

	bool entryChanged = changeEntry(def.m_itemId);

	if (defChanged)
	{
		m_needTooltipRefresh = true;

		if (!entryChanged)
			pickIcon();
	}
}

void ItemIcon::drawLabels() /*final*/
{
	__super::drawLabels();
}

sf::Color ItemIcon::itemColor(const ItemDefines::Quality quality)
{
	// VERIFIED original client FUN_0048e810: 1-based rarity colors (quality 3 = Radiant = green).
	switch (quality)
	{
		case ItemDefines::Quality::QualityLv1:	return sf::Color(0x8e8b92ff); // Junk    grey
		case ItemDefines::Quality::QualityLv2:	return sf::Color(0xf2f1f1ff); // Normal  white
		case ItemDefines::Quality::QualityLv3:	return sf::Color(0x1ef002ff); // Radiant green
		case ItemDefines::Quality::QualityLv4:	return sf::Color(0x3e9dffff); // Blessed blue
		case ItemDefines::Quality::QualityLv5:	return sf::Color(0xff73b3ff); // Holy    pink
		case ItemDefines::Quality::QualityLv6:	return sf::Color(0xb33ffaff); // Godly   purple
		default: break;
	}

	return sf::Color(0xffffffff); // quality 0 / out-of-range = white
}

void ItemIcon::onEntryChange() /*final*/
{
	auto& it = sContentMgr->db("item_template");
	m_itemDef.m_itemId = getEntry();
	m_quality = ItemDefines::Quality(atoi(it.data(getEntry(), "quality").c_str()));
}

void ItemIcon::pickIcon() /*final*/
{	
	// Spell data
	m_castedSpellId = 0;
	m_castedSpellCategoryCooldown = 0;

	setExclaimNotice(false);

	if (getEntry() == 0)
	{
		m_icon = nullptr;
		return;
	}

	auto& it = sContentMgr->db("item_template");
	auto& st = sContentMgr->db("spell_template");

	auto considerSpell = [&](string& str)
	{
		int spellId = atoi(str.c_str());

		if (spellId == 0)
			return;

		uint64_t spellAttr = _atoi64(st.data(spellId, "attributes").c_str());

		if (!Util::maskHas(spellAttr, SpellDefines::Attributes::Passive))
		{
			m_castedSpellId = spellId;
			m_castedSpellCategoryCooldown = atoi(st.data(spellId, "cooldown_category").c_str());
		}
	};	

	considerSpell(it.data(getEntry(), "spell_1"));
	considerSpell(it.data(getEntry(), "spell_2"));
	considerSpell(it.data(getEntry(), "spell_3"));
	considerSpell(it.data(getEntry(), "spell_4"));
	considerSpell(it.data(getEntry(), "spell_5"));

	// Icon
	const string iconName = it.data(getEntry(), "icon");
	ASSERT(m_icon = sContentMgr->spawnSprite(iconName));
	m_red = computeHasErrorIcon();
}
		
/*static*/
string ItemIcon::formItemTitle(const ItemDefines::ItemDefinition def)
{
	auto& it = sContentMgr->db("item_template");
	auto& at = sContentMgr->db("affix_template");

	string strName = it.data(def.m_itemId, "name");
	string strAffixName = at.data(def.m_affixId, "name");
	auto m_quality = ItemDefines::Quality(atoi(it.data(def.m_itemId, "quality").c_str()));
	string strQualityName;
	
	if (!strAffixName.empty())
		strQualityName = ItemFunctions::qualityName(ItemDefines::Quality(m_quality));

	int equipType = atoi(it.data(def.m_itemId, "equip_type").c_str());
	int armorType = atoi(it.data(def.m_itemId, "armor_type").c_str());
	int weaponType = atoi(it.data(def.m_itemId, "weapon_type").c_str());
	
	if (!strAffixName.empty())
	{
		bool name_single_noun = atoi(at.data(def.m_affixId, "name_single_noun").c_str());

		if (name_single_noun)
			strName += " of the " + strAffixName;
		else
			strName += " of " + strAffixName;
	}

	if (def.m_enchantLvl > 0)
		strName += " +" + to_string(def.m_enchantLvl);

	if (!strQualityName.empty() && equipType != 0)
		strName = strQualityName + " " + strName;

	return strName;
}
		
bool ItemIcon::computeHasErrorIcon()
{
	if (auto world = getWorld())
	{
		if (auto myself = world->myself())
		{
			auto& it = sContentMgr->db("item_template");
			int equipType = atoi(it.data(getEntry(), "equip_type").c_str());
			int weaponType = atoi(it.data(getEntry(), "weapon_type").c_str());
			int requiredLevel = atoi(it.data(getEntry(), "required_level").c_str());
			
			if (m_itemDef.m_durability == 0 && atoi(it.data(getEntry(), "durability").c_str()) > 0)
				return true;

			if (!myself->canEquipItem(myself->getClass(), ItemDefines::EquipType(equipType)) || 
				!myself->canEquipItem(myself->getClass(), ItemDefines::WeaponType(weaponType)))
			{
				return true;
			}
			else
			{
				if (int armorType = atoi(it.data(getEntry(), "armor_type").c_str()))
				{
					if (!myself->canEquipItem(myself->getClass(), ItemDefines::ArmorType(armorType), ItemDefines::EquipType(equipType)))
						return true;
				}

				if (int weaponMaterial = atoi(it.data(getEntry(), "weapon_material").c_str()))
				{
					if (!myself->canEquipItem(ItemDefines::WeaponType(weaponType), ItemDefines::WeaponMaterial(weaponMaterial)))
						return true;
				}
			}
			
			if (!m_ignoreSpellbookNotifyForIcon)
			{
				if (const int requiredClass = atoi(it.data(getEntry(), "required_class").c_str()))
				{
					if (myself->getClass() != requiredClass || alreadyKnownSkillbook(world) || myself->getLevel() < requiredLevel)
					{
						return true;
					}
					else
					{					
						if (Util::maskHas(atoi(it.data(getEntry(), "flags").c_str()), ItemDefines::ItemFlags::ItemFlag_Skillbook))
							setExclaimNotice(true);
					}
				}
			}

			if (!m_ignoreLevelErrorForIcon)
			{
				if (const int requiredLevel = atoi(it.data(getEntry(), "required_level").c_str()))
				{
					if (requiredLevel > myself->getLevel())
						return true;
				}
			}
		}

	}

	return false;
}

shared_ptr<Tooltip> ItemIcon::buildTooltip()
{
	if (getEntry() == 0)
		return nullptr;
	fillTooltip();              // rebuilds m_tooltip from the current item
	return getTooltip();
}

void ItemIcon::fillTooltip() /*final*/
{
	if (getEntry() == 0)
	{
		setTooltip(nullptr);
		return;
	}

	auto& it = sContentMgr->db("item_template");
	auto& st = sContentMgr->db("spell_template");
	auto& at = sContentMgr->db("affix_template");

	auto tooltip = make_shared<Tooltip>(*this, sf::Vector2i());
	tooltip->setShadowOffset(1);
	tooltip->setAllowMoreWidthIfLastLineIsUnderThisValue(60);

	// Name	
	string strName = formItemTitle(m_itemDef);
	int titleWidth = tooltip->addLine(tooltipFont(), 18, strName, itemColor(m_quality));
	tooltip->setAllowMoreWidthIfLastLineIsUnderThisValue(0);

	if (titleWidth > Tooltip::Defines::MaxTooltipWidth)
		tooltip->setMaxWidth(titleWidth);
	else
		tooltip->setMaxWidth(Tooltip::Defines::MaxTooltipWidth);

	int equipType = atoi(it.data(getEntry(), "equip_type").c_str());
	int armorType = atoi(it.data(getEntry(), "armor_type").c_str());
	int weaponType = atoi(it.data(getEntry(), "weapon_type").c_str());
	int weapon_material = 0;

	int shortestTrailingLine = 0;

	if (m_itemDef.m_soulbound)
		tooltip->addLine(tooltipFont(), 15, "Soulbound");

	if (Util::maskHas(atoi(it.data(getEntry(), "flags").c_str()), ItemDefines::ItemFlags::ItemFlag_QuestItem))
		tooltip->addLine(tooltipFont(), 15, "Quest Item");

	if (int startQuest = atoi(it.data(getEntry(), "quest_offer").c_str()))
		tooltip->addLine(tooltipFont(), 15, "This Item Begins a Quest");

	m_hasEquipErrors = false;

	// "Helm		Mail", for example
	// "Weapon		Axe", for example
	string equipText;

	string str1;
	string str2;

	if (armorType != 0 && equipType != 0)
	{
		str1 = ItemFunctions::armorTypeName(ItemDefines::ArmorType(armorType));
		str2 = ItemFunctions::equipTypeName(ItemDefines::EquipType(equipType));
	}
	else
	{
		if (equipType != 0)
			str1 = ItemFunctions::equipTypeName(ItemDefines::EquipType(equipType));

		if (armorType != 0)
			str2 = ItemFunctions::armorTypeName(ItemDefines::ArmorType(armorType));
	}

	if (weaponType != 0)
	{
		str2 = ItemFunctions::weaponTypeName(ItemDefines::WeaponType(weaponType));
		
		// Bronze Sword
		if (weapon_material = atoi(it.data(getEntry(), "weapon_material").c_str()))
			str1 = ItemFunctions::weaponMaterialType(ItemDefines::WeaponMaterial(weapon_material));
	}

	if (equipType == 0)
		m_hasEquipErrors = true;

	if (m_itemDef.m_affixScore > 1)
		str1 = ItemFunctions::affixScoreName(m_itemDef.m_affixScore) + " " + str1;

	if (!str1.empty() || !str2.empty())
		equipText = Util::fmtStr("%s%s%s", str1.c_str(), TextBox::getStringKey(TextBox::Key_ReplaceSpaceToMaxWidth).c_str(), str2.c_str());
	
	std::map<UnitDefines::Stat, int> failedStatReq;
	std::map<UnitDefines::Stat, int> allRequirements;

	if (!equipText.empty())
	{
		sf::Color col = sf::Color::White;

		if (auto world = getWorld())
		{
			if (auto myself = world->myself())
			{
				if (!myself->canEquipItem(myself->getClass(), ItemDefines::EquipType(equipType)) || 
					!myself->canEquipItem(myself->getClass(), ItemDefines::WeaponType(weaponType)))
				{
					// Can never use this type
					col = sf::Color::Red;
					m_hasEquipErrors = true;
				}
				else
				{
					myself->canEquipItem(myself->getClass(), ItemDefines::ArmorType(armorType), ItemDefines::EquipType(equipType), &failedStatReq);
					myself->canEquipItem(ItemDefines::WeaponType(weaponType), ItemDefines::WeaponMaterial(weapon_material), &failedStatReq);

					// The original client lists the base-stat requirements whenever the item has any
					// (red when unmet, white when met), not only when one is unmet.
					allRequirements = PlayerFunctions::getRequiredStats(myself->getClass(), ItemDefines::ArmorType(armorType), ItemDefines::EquipType(equipType));

					if (!failedStatReq.empty())
						m_hasEquipErrors = true; // unmet -> red icon cover
				}
			}
		}

		tooltip->addLine(tooltipFont(), 15, equipText, col);	
	}

	const int requiredLevel = atoi(it.data(getEntry(), "required_level").c_str());
	std::map<UnitDefines::Stat, int> baseItemStats = sItemDefiner->baseItemStats(getEntry(), requiredLevel, m_itemDef.m_enchantLvl);

	if (weaponType != 0)
	{
		// 33 Weapon Value		Speed 1.90
		if (int weapon_value = baseItemStats[UnitDefines::Stat::WeaponValue])
		{
			float attack_speed = static_cast<float>(baseItemStats[UnitDefines::Stat::MeleeCooldown]) / 1000.0f;
			tooltip->addLine(tooltipFont(), 15, Util::fmtStr("%d Weapon Value%sSpeed %.2f", weapon_value, TextBox::getStringKey(TextBox::Key_ReplaceSpaceToMaxWidth).c_str(), attack_speed));
		}

		if (int ranged_value = baseItemStats[UnitDefines::Stat::RangedWeaponValue])
		{
			float attack_speed = static_cast<float>(baseItemStats[UnitDefines::Stat::RangedCooldown]) / 1000.0f;
			tooltip->addLine(tooltipFont(), 15, Util::fmtStr("%d Ranged Value%sSpeed %.2f", ranged_value, TextBox::getStringKey(TextBox::Key_ReplaceSpaceToMaxWidth).c_str(), attack_speed));
		}
	}

	// Stats
	vector<string> intStats;
	vector<string> textStats;

	std::map<UnitDefines::Stat, int> stats = sItemDefiner->crunchItemStats(getEntry(), m_itemDef.m_affixId, m_itemDef.m_affixScore, requiredLevel, m_itemDef.m_enchantLvl);
	
	if (armorType != 0)
	{
		// 96 Armor Value
		tooltip->addLine(tooltipFont(), 15, Util::fmtStr("%d Armor Value", baseItemStats[UnitDefines::Stat::ArmorValue]));
	}
	else if (baseItemStats[UnitDefines::Stat::ArmorValue] > 0)
	{
		// +96 Armor Value
		stats[UnitDefines::Stat::ArmorValue] += baseItemStats[UnitDefines::Stat::ArmorValue];
	}

	auto applyStat = [&](const UnitDefines::Stat stat, const int value)
	{
		if (stat == 0 || value == 0)
			return;

		switch (stat)
		{
			case UnitDefines::Stat::MeleeCritical:
			case UnitDefines::Stat::RangedCritical:
			case UnitDefines::Stat::SpellCritical:
				textStats.push_back(Util::fmtStr("Equip: Increases your %s rating by %d.", Util::toLowerCase(UnitFunctions::statName(stat)).c_str(), value));
				break;
				
			case UnitDefines::Stat::WeaponValue:
			case UnitDefines::Stat::RangedWeaponValue:

			case UnitDefines::Stat::DodgeRating:
			case UnitDefines::Stat::BlockRating:
				textStats.push_back(Util::fmtStr("Equip: Increases your %s by %d.", Util::toLowerCase(UnitFunctions::statName(stat)).c_str(), value));
				break;

			case UnitDefines::Stat::Bartering:
			case UnitDefines::Stat::Lockpicking:
			//case UnitDefines::Stat::Perception:

			case UnitDefines::Stat::StaffSkill:
			case UnitDefines::Stat::MaceSkill:
			case UnitDefines::Stat::AxesSkill:
			case UnitDefines::Stat::SwordSkill:
			case UnitDefines::Stat::RangedSkill:
			case UnitDefines::Stat::DaggerSkill:
			case UnitDefines::Stat::WandSkill:
			case UnitDefines::Stat::ShieldSkill:
				textStats.push_back(Util::fmtStr("Equip: Increases your %s skill by %d.", Util::toLowerCase(UnitFunctions::statName(stat)).c_str(), value));
				break;

			default:					
				intStats.push_back("+" + to_string(value) + " " + UnitFunctions::statName(stat));
				break;
		}
	};

	shared_ptr<ItemAffix> affixStats = sItemDefiner->getAffix(m_itemDef.m_affixId);

	if (affixStats != nullptr)
	{
		// Affix stats first and in order too
		for (auto& stat : affixStats->statOrder())
		{
			applyStat(stat, stats[stat]);
			stats[stat] = 0;
		}		
	}

	for (auto& sb : stats)
	{
		if (sb.second != 0)
			applyStat(sb.first, sb.second);
	}

	for (auto& itr : intStats)
		tooltip->addLine(tooltipFont(), 15, itr);
		
	if (int num_sockets = atoi(it.data(getEntry(), "num_sockets").c_str()))
	{
		tooltip->addLine(FontId::FrizRegular, 12, "Magical gem sockets", sf::Color(240, 197, 2, 255));

		switch (num_sockets)
		{
			case 1:
				tooltip->addGlyph(make_shared<TooltipGlyph>(TooltipGlyph::Types::Gems_x1, m_itemDef.m_gem1, m_itemDef.m_gem2, tooltipFont()));
				break;
			case 2:
				tooltip->addGlyph(make_shared<TooltipGlyph>(TooltipGlyph::Types::Gems_x2, m_itemDef.m_gem1, m_itemDef.m_gem2, tooltipFont()));
				break;
			case 3:
				tooltip->addGlyph(make_shared<TooltipGlyph>(TooltipGlyph::Types::Gems_x2, m_itemDef.m_gem1, m_itemDef.m_gem2, tooltipFont()));
				tooltip->addGlyph(make_shared<TooltipGlyph>(TooltipGlyph::Types::Gems_x1, m_itemDef.m_gem3, m_itemDef.m_gem4, tooltipFont()));
				break;
			case 4:
				tooltip->addGlyph(make_shared<TooltipGlyph>(TooltipGlyph::Types::Gems_x2, m_itemDef.m_gem1, m_itemDef.m_gem2, tooltipFont()));
				tooltip->addGlyph(make_shared<TooltipGlyph>(TooltipGlyph::Types::Gems_x2, m_itemDef.m_gem3, m_itemDef.m_gem4, tooltipFont()));
				break;
		}
	}

	tooltip->setAllowMoreWidthIfLastLineIsUnderThisValue(55);

	for (auto& itr : textStats)
		tooltip->addLine(tooltipFont(), 15, itr, sf::Color(240, 197, 2, 255));

	tooltip->setAllowMoreWidthIfLastLineIsUnderThisValue(0);

	// Durability 90/90
	if (const int maxDurability = atoi(it.data(getEntry(), "durability").c_str()))
	{
		const bool broken = maxDurability > 0 && m_itemDef.m_durability <= 0;
		tooltip->addLine(tooltipFont(), 15, Util::fmtStr("Durability %d/%d", m_itemDef.m_durability, maxDurability).c_str(), broken ? sf::Color::Red : sf::Color::White);
	}

	// Requires level 7
	if (requiredLevel != 0)
	{
		tooltip->addLine(tooltipFont(), 15, "Requires level " + to_string(requiredLevel), m_casterLevel < requiredLevel ? sf::Color::Red : sf::Color::White);
	
		if (m_casterLevel < requiredLevel)
			m_hasEquipErrors = true;
	}

	// Requires class
	if (const int requiredClass = atoi(it.data(getEntry(), "required_class").c_str()))
	{
		int myClass = requiredClass;

		if (auto world = getWorld())
		{
			if (auto myself = world->myself())
				myClass = myself->getClass();
		}

		string className = PlayerFunctions::className(static_cast<PlayerDefines::Classes>(requiredClass));
		tooltip->addLine(tooltipFont(), 15, "Classes: ", myClass == requiredClass ? sf::Color::White : sf::Color::Red, false);
		tooltip->addLine(tooltipFont(), 15, className, sf::Color(PlayerFunctions::classColor(static_cast<PlayerDefines::Classes>(requiredClass))));
	}

	// Spells (Use: %s, or just a passive buff)
	auto addSpellTxt = [&](string& str)
	{
		int spellId = atoi(str.c_str());

		if (spellId == 0)
			return;

		string spellDescription = formSpellDescription(spellId);

		if (spellDescription.empty())
			return;

		uint64_t spellAttr = _atoi64(st.data(spellId, "attributes").c_str());

		// passive or active
		if (!Util::maskHas(spellAttr, SpellDefines::Attributes::Passive))
			spellDescription = "Use: " + spellDescription;

		tooltip->addLine(tooltipFont(), 15, spellDescription, sf::Color(240, 197, 2, 255));
	};
	
	addSpellTxt(it.data(getEntry(), "spell_1"));
	addSpellTxt(it.data(getEntry(), "spell_2"));
	addSpellTxt(it.data(getEntry(), "spell_3"));
	addSpellTxt(it.data(getEntry(), "spell_4"));
	addSpellTxt(it.data(getEntry(), "spell_5"));

	string descript = it.data(getEntry(), "description");

	if (!descript.empty())
		tooltip->addLine(tooltipFont(), 15, "\"" + descript + "\"", sf::Color(240, 197, 2, 255));

	if (m_showSellPrice)
	{
		// Sell Price: 95370g
		if (int sellPrice = atoi(it.data(getEntry(), "sell_price").c_str()))
		{
			string barteringStr;

			if (auto world = getWorld())
			{
				if (auto myself = world->myself())
				{
					float sell_offset = 0.f;
					PlayerFunctions::getBarteringPcts(nullptr, &sell_offset, myself->getStat(UnitDefines::Stat::Bartering));
					PlayerFunctions::applyItemGoldValueScales(&sellPrice, myself->getLevel(), atoi(it.data(getEntry(), "flags").c_str()));
					PlayerFunctions::applyBarterting(nullptr, &sellPrice, myself->getStat(UnitDefines::Stat::Bartering));

					if (sell_offset > 0)
						barteringStr = Util::fmtStr(" (+%.1f%%)", sell_offset * 100.f);
				}
			}

			tooltip->addLine(tooltipFont(), 15, "Sell Price: " + Util::formatMoneyCommas(sellPrice) + "g" + barteringStr);
		}
	}

	// You already know this ability
	if (m_castedSpellId != 0)
	{
		if (auto world = getWorld())
		{
			if (alreadyKnownSkillbook(world))
				tooltip->addLine(tooltipFont(), 15, "You already know this ability", sf::Color::Red);
		}
	}

	if (!allRequirements.empty())
	{
		// List every base-stat requirement; red = unmet, white = met (matches original client).
		for (auto& itr : allRequirements)
		{
			sf::Color col = sf::Color::White;

			if (failedStatReq.find(itr.first) != failedStatReq.end())
				col = sf::Color::Red;

			tooltip->addLine(tooltipFont(), 15, Util::fmtStr("Requires %d base %s", itr.second, UnitFunctions::statName(itr.first).c_str()), col);
		}
	}

	setTooltip(tooltip);
}

bool ItemIcon::alreadyKnownSkillbook(World* world) const
{
	auto& st = sContentMgr->db("spell_template");
	int effect1 = atoi(st.data(m_castedSpellId, "effect1").c_str());

	if (effect1 == SpellDefines::Effects::LearnSpell)
	{
		int effect1_data1 = atoi(st.data(m_castedSpellId, "effect1_data1").c_str());
		
		if (auto world = getWorld())
		{			
			if (auto abilities = dynamic_pointer_cast<Abilities>(world->getRenderObject(World::Interface::AbilitiesPanel)))
			{
				if (abilities->hasSpell(effect1_data1))
					return true;
			}
		}
	}

	return false;
}

string ItemIcon::deduceTitle() const /*final*/
{
	if (getEntry() == ItemDefines::StaticItems::GoldItem)
		return Util::formatMoneyCommas(m_stackAmount) + " Gold Pieces";

	return formItemTitle(m_itemDef);
}

string ItemIcon::deduceDescription() const /*final*/
{
	auto& sv = sContentMgr->db("item_template");
	return sv.data(getEntry(), "name");
}
