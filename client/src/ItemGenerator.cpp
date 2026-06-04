#include "stdafx.h"
#include "ItemGenerator.h"
#include "Application.h"

#include "..\SqlConnector\SqlConnector.h"
#include "..\Shared\Config.h"
#include "..\StringHelpers.h"
#include "..\Shared\UnitDefines.h"
#include "..\Rand.h"

// Bunch of nonsense code for building initial database

void ItemGenerator::assignGenCosts()
{
	sConfig->setSource("config.ini");
	shared_ptr<SqlConnector> db = SqlConnector::create(SqlType::SQLite, sConfig->getString("GameDb", "Name"), sConfig->getString("GameDb", "Path"));

	db->query("update item_template set durability = quality*20 where (armor_type NOTNULL and armor_type > 0 and armor_type != "") or (weapon_type NOTNULL and weapon_type > 0 and weapon_type != "") or equip_type = 10");

	db->query("update item_template set sell_price = required_level * 1 * quality where armor_type = 1;	");
	db->query("update item_template set sell_price = required_level * 2 * quality where armor_type = 2;	");
	db->query("update item_template set sell_price = required_level * 3 * quality where armor_type = 3;	");
	db->query("update item_template set sell_price = required_level * 4 * quality where armor_type = 4;	");
	db->query("update item_template set sell_price = required_level * 5 * quality where armor_type = 5;	");
	db->query("update item_template set sell_price = required_level * 6 * quality where armor_type = 6;	");
	db->query("update item_template set sell_price = required_level * 7 * quality where armor_type = 7;	");
	db->query("update item_template set sell_price = required_level * 8 * quality where armor_type = 8;	");
	db->query("update item_template set sell_price = required_level * 9 * quality where armor_type = 9;	");
	db->query("update item_template set sell_price = required_level * 10 * quality where armor_type = 10;");
	db->query("update item_template set sell_price = required_level * 11 * quality where armor_type = 11;");
	db->query("update item_template set sell_price = required_level * 2 * quality where armor_type = 12;	");
	db->query("update item_template set sell_price = required_level * 4 * quality where armor_type = 13;	");
	db->query("update item_template set sell_price = required_level * 6 * quality where armor_type = 14;	");
	db->query("update item_template set sell_price = required_level * 8 * quality where armor_type = 15;	");

	db->query("update item_template set sell_price = required_level * 5 * quality where weapon_material = 1;	");
	db->query("update item_template set sell_price = required_level * 6 * quality where weapon_material = 2;	");
	db->query("update item_template set sell_price = required_level * 7 * quality where weapon_material = 3;	");
	db->query("update item_template set sell_price = required_level * 8 * quality where weapon_material = 4;	");
	db->query("update item_template set sell_price = required_level * 9 * quality where weapon_material = 5;	");
	db->query("update item_template set sell_price = required_level * 10 * quality where weapon_material = 6;");
	db->query("update item_template set sell_price = required_level * 11 * quality where weapon_material = 7;");

	db->query("update item_template set sell_price = required_level * 2 * quality where weapon_material = 8;	");
	db->query("update item_template set sell_price = required_level * 3 * quality where weapon_material = 9;	");
	db->query("update item_template set sell_price = required_level * 4 * quality where weapon_material = 10;");
	db->query("update item_template set sell_price = required_level * 5 * quality where weapon_material = 11;");
	db->query("update item_template set sell_price = required_level * 6 * quality where weapon_material = 12;");
	db->query("update item_template set sell_price = required_level * 7 * quality where weapon_material = 13;");
	db->query("update item_template set sell_price = required_level * 8 * quality where weapon_material = 14;");

	db->query("update item_template set sell_price = required_level * quality * 7 where equip_type = 2; ");
	db->query("update item_template set sell_price = required_level * quality * 5 where equip_type = 8; ");
	db->query("update item_template set sell_price = required_level * quality * 9 where equip_type = 10;");
	
	db->query("update item_template set num_sockets = 1 where quality = 3 and generated = 1;");
	db->query("update item_template set num_sockets = 2 where quality = 4 and generated = 1;");
	db->query("update item_template set num_sockets = 3 where quality = 5 and generated = 1;");
	db->query("update item_template set num_sockets = 4 where quality = 6 and generated = 1;");
	db->query("update item_template set num_sockets = 0 where quality = 3 and generated = 1 and equip_type in (2, 4, 8);");
	db->query("update item_template set num_sockets = 0 where quality = 4 and generated = 1 and equip_type in (2, 4, 8);");
	db->query("update item_template set num_sockets = 1 where quality = 5 and generated = 1 and equip_type in (2, 4, 8);");
	db->query("update item_template set num_sockets = 2 where quality = 6 and generated = 1 and equip_type in (2, 4, 8);");

	db->query("update item_template set armor_type = 4 where equip_type = 10 and quality = 2;");
	db->query("update item_template set armor_type = 5 where equip_type = 10 and quality = 3;");
	db->query("update item_template set armor_type = 6 where equip_type = 10 and quality = 4;");
	db->query("update item_template set armor_type = 7 where equip_type = 10 and quality = 5;");
	db->query("update item_template set armor_type = 8 where equip_type = 10 and quality = 6;");

	// After intentionally
	db->query("update item_template set sell_price = sell_price * 1.15 where equip_type = 1;");
	db->query("update item_template set sell_price = sell_price * 1.25 where equip_type = 3;");
	db->query("update item_template set sell_price = sell_price * 1.20 where equip_type = 5;");
}

void ItemGenerator::genBaseItems()
{
	sConfig->setSource("config.ini");
	shared_ptr<SqlConnector> db = SqlConnector::create(SqlType::SQLite, sConfig->getString("GameDb", "Name"), sConfig->getString("GameDb", "Path"));

	struct Key
	{
		int quality;
		int weapon_type;
		int armor_type;
		int equip_type; 
	};

	struct Dat
	{
		Key key;
		string icon;
		string sound;
		string model;
		string itemname;
	};

	vector<Dat> dats;

	if (auto result = db->query(Util::fmtStr("SELECT icon, sound, model, quality, weapon_type, armor_type, equip_type FROM item_dictionary;")))
	{
		for (auto& itr : result->fetchData())
		{
			Dat dat;
			dat.icon = itr[0].getString();
			dat.sound = itr[1].getString();
			dat.model = itr[2].getString();
			dat.key.quality = itr[3].getInt32();
			dat.key.weapon_type = itr[4].getInt32();
			dat.key.armor_type = itr[5].getInt32();
			dat.key.equip_type = itr[6].getInt32();

			bool duplicated = false;

			for (auto& itrDat : dats)
			{
				if (itrDat.key.quality == dat.key.quality &&
					itrDat.key.weapon_type == dat.key.weapon_type &&
					itrDat.key.armor_type == dat.key.armor_type &&
					itrDat.key.equip_type == dat.key.equip_type)
				{
					duplicated = true;
					break;
				}
			}

			if (!duplicated)
				dats.push_back(dat);
		}
	}

	// Have our unique list
	// --

	for (auto& itr : dats)
	{
		string itemName;

		if (itr.key.weapon_type != 0)
		{
			switch (itr.key.weapon_type)
			{
				case ItemDefines::WeaponType::Axe:
				{
					itemName = Util::randomChoice(string("Axe"), string("Cleaver"), string("Reaper"));
					break;
				}
				case ItemDefines::WeaponType::Bow:
				{
					switch (itr.key.quality)
					{
						case ItemDefines::Quality::QualityLv1:
							itemName = "Shortbow";
							break;
						case ItemDefines::Quality::QualityLv2: 
							itemName = "Longbow";
							break;
						case ItemDefines::Quality::QualityLv3: 
							itemName = "Greatbow";
							break;
						case ItemDefines::Quality::QualityLv4: 
							itemName = "Dragonslayer";
							break;
						case ItemDefines::Quality::QualityLv5:
							itemName = "Dragonslayer";
							break;
					}

					break;
				}
				case ItemDefines::WeaponType::Mace:
				{
					itemName = Util::randomChoice(string("Mace"), string("Hammer"));
					break;
				}
				case ItemDefines::WeaponType::Sword:
				{
					itemName = Util::randomChoice(string("Blade"), string("Slicer"));
					break;
				}
				case ItemDefines::WeaponType::Staff:
				{
					itemName = Util::randomChoice(string("Staff"), string("Stave"));
					break;
				}
				case ItemDefines::WeaponType::Dagger:
				{
					itemName = Util::randomChoice(string("Dagger"), string("Shank"), string("Shiv"));
					break;
				}
				case ItemDefines::WeaponType::Wand:
				{
					itemName = Util::randomChoice(string("Magic Rod"), string("Magic Wand"));
					break;
				}
			}
		}
		else if (itr.key.equip_type != 0)
		{
			switch (itr.key.equip_type)
			{
				case ItemDefines::EquipType::Necklace:
				{
					itemName = Util::randomChoice(string("Collar"), string("Choker"), string("Necklace"), string("Opera"), string("Pendant"));
					break;
				}
				case ItemDefines::EquipType::Belt:
				{
					itemName = Util::randomChoice(string("Sash"), string("Belt"), string("Girdle"));
					break;
				}
				case ItemDefines::EquipType::Ring:
				{
					itemName = Util::randomChoice(string("Ring"), string("Signet"), string("Band"));
					break;
				}
				case ItemDefines::EquipType::Shield:
				{
					itemName = Util::randomChoice(string("Barricade"), string("Defender"));
					break;
				}
				default:
				{
					switch (itr.key.armor_type)
					{					
						case ItemDefines::ArmorModels::ModelMage:
						case ItemDefines::ArmorModels::ModelMageAlt1:
						case ItemDefines::ArmorModels::ModelMageAlt2:
						{
							switch (itr.key.equip_type)
							{
								case ItemDefines::EquipType::Chest:
									itemName = "Vestments";
									break;
								case ItemDefines::EquipType::Legs:
									itemName = "Skirt";
									break;
								case ItemDefines::EquipType::Feet:
									itemName = "Slippers";
									break;
								case ItemDefines::EquipType::Hands:
									itemName = "Handwraps";
									break;
								case ItemDefines::EquipType::Helm:
									itemName = "Hood";
									break;
							}

							break;
						}
						case ItemDefines::ArmorModels::ModelCloth:
						{
							switch (itr.key.equip_type)
							{
								case ItemDefines::EquipType::Chest:
									itemName = "Shirt";
									break;
								case ItemDefines::EquipType::Legs:
									itemName = "Pants";
									break;
								case ItemDefines::EquipType::Feet:
									itemName = "Shoes";
									break;
								case ItemDefines::EquipType::Hands:
									itemName = "Handwraps";
									break;
								case ItemDefines::EquipType::Helm:
									itemName = "Hat";
									break;
							}

							break;
						}
						case ItemDefines::ArmorModels::ModelLeather:
						{
							switch (itr.key.equip_type)
							{
								case ItemDefines::EquipType::Chest:
									itemName = "Jerkin";
									break;
								case ItemDefines::EquipType::Legs:
									itemName = "Trousers";
									break;
								case ItemDefines::EquipType::Feet:
									itemName = "Boots";
									break;
								case ItemDefines::EquipType::Hands:
									itemName = "Gloves";
									break;
								case ItemDefines::EquipType::Helm:
									itemName = "Cap";
									break;
							}

							break;
						}
						case ItemDefines::ArmorModels::ModelMail:
						{
							switch (itr.key.equip_type)
							{
								case ItemDefines::EquipType::Chest:
									itemName = "Chainmail";
									break;
								case ItemDefines::EquipType::Legs:
									itemName = "Leggings";
									break;
								case ItemDefines::EquipType::Feet:
									itemName = "Greaves";
									break;
								case ItemDefines::EquipType::Hands:
									itemName = "Grips";
									break;
								case ItemDefines::EquipType::Helm:
									itemName = "Coif";
									break;
							}

							break;
						}
						case ItemDefines::ArmorModels::ModelPlate:
						{
							switch (itr.key.equip_type)
							{
								case ItemDefines::EquipType::Chest:
									itemName = "Beastplate";
									break;
								case ItemDefines::EquipType::Legs:
									itemName = "Legguards";
									break;
								case ItemDefines::EquipType::Feet:
									itemName = "Sabatons";
									break;
								case ItemDefines::EquipType::Hands:
									itemName = "Gauntlets";
									break;
								case ItemDefines::EquipType::Helm:
									itemName = "Helmet";
									break;
							}

							break;
						}
					}

					break;
				}
			}
		}

		itr.itemname = itemName;
	}

	for (auto& itr : dats)
	{
		if (itr.itemname.empty())
			continue;

		if (itr.icon.empty())
			itr.icon = " ";

		if (itr.sound.empty())
			itr.sound = " ";
		
		vector<ItemDefines::ArmorType> atypes = { (ItemDefines::ArmorType)itr.key.armor_type };
		vector<ItemDefines::WeaponMaterial> wmats = { };

		if (itr.key.armor_type == ItemDefines::ArmorModels::ModelMail)
		{
			atypes = { ItemDefines::ArmorType::Bronze,  ItemDefines::ArmorType::Steel, ItemDefines::ArmorType::Mythril, ItemDefines::ArmorType::Emerald };
		}
		else if (itr.key.armor_type == ItemDefines::ArmorModels::ModelPlate)
		{
			atypes = { ItemDefines::ArmorType::Crystal, ItemDefines::ArmorType::Diamond, ItemDefines::ArmorType::Titanium };
		}
		else if (itr.key.armor_type == ItemDefines::ArmorModels::ModelCloth)
		{
			atypes = {  ItemDefines::ArmorType::Cotton };
		}
		else if (itr.key.armor_type == ItemDefines::ArmorModels::ModelLeather)
		{
			atypes = { ItemDefines::ArmorType::Leather, ItemDefines::ArmorType::Brigandine, ItemDefines::ArmorType::Gambeson };
		}
		else if (itr.key.armor_type == ItemDefines::ArmorModels::ModelMage)
		{
			atypes = { ItemDefines::ArmorType::Linen, ItemDefines::ArmorType::Wool };
		}
		else if (itr.key.armor_type == ItemDefines::ArmorModels::ModelMageAlt1)
		{
			atypes = { ItemDefines::ArmorType::Silk };
		}
		else if (itr.key.armor_type == ItemDefines::ArmorModels::ModelMageAlt2)
		{
			atypes = { ItemDefines::ArmorType::Spiritsilk };
		}
		else if (itr.key.armor_type)
		{

		}

		switch (itr.key.weapon_type)
		{
			case ItemDefines::WeaponType::Axe:
			case ItemDefines::WeaponType::Mace:
			case ItemDefines::WeaponType::Sword:
			case ItemDefines::WeaponType::Dagger:
			{				
				wmats = {  ItemDefines::WeaponMaterial::Wep_Bronze, ItemDefines::WeaponMaterial::Wep_Steel, ItemDefines::WeaponMaterial::Wep_Mythril, ItemDefines::WeaponMaterial::Wep_Emerald,
					ItemDefines::WeaponMaterial::Wep_Crystal, ItemDefines::WeaponMaterial::Wep_Diamond, ItemDefines::WeaponMaterial::Wep_Titanium};
				break;
			}
			case ItemDefines::WeaponType::Staff:
			case ItemDefines::WeaponType::Wand:
			case ItemDefines::WeaponType::Bow:
			{
				wmats = {  ItemDefines::WeaponMaterial::Wep_Aspen, ItemDefines::WeaponMaterial::Wep_Elm, ItemDefines::WeaponMaterial::Wep_Cherry, ItemDefines::WeaponMaterial::Wep_BlackWalnut,
					ItemDefines::WeaponMaterial::Wep_WhiteOak, ItemDefines::WeaponMaterial::Wep_HardMaple, ItemDefines::WeaponMaterial::Wep_Hickory};
				break;
			}
		}

		int entry = 5000;

		if (!wmats.empty())
		{
			for (auto& wmat : wmats)
			{
				for (int i = 1; i <= 25; ++i)
				{
					string query = Util::fmtStr("INSERT INTO 'item_template' ('entry', 'name', 'icon', 'icon_sound', 'model', 'quality', 'weapon_type', 'weapon_material', 'equip_type', 'generated', 'required_level') VALUES ('%d', '%s', '%s', '%s', '%s', '%d', '%d', '%d', '%d', '1', '%d');",
						entry++, itr.itemname.c_str(), itr.icon.c_str(), itr.sound.c_str(), itr.model.c_str(), itr.key.quality, itr.key.weapon_type, wmat, itr.key.equip_type, i);

					db->query(query);
				}
			}
		}
		else
		{
			for (auto& atype : atypes)
			{
				for (int i = 1; i <= 25; ++i)
				{
					string query = Util::fmtStr("INSERT INTO 'item_template' ('entry', 'name', 'icon', 'icon_sound', 'model', 'quality', 'weapon_type', 'armor_type', 'equip_type', 'generated', 'required_level') VALUES ('%d', '%s', '%s', '%s', '%s', '%d', '%d', '%d', '%d', '1', '%d');",
						entry++, itr.itemname.c_str(), itr.icon.c_str(), itr.sound.c_str(), itr.model.c_str(), itr.key.quality, itr.key.weapon_type, atype, itr.key.equip_type, i);

					db->query(query);
				}
			}
		}
	}
}

void ItemGenerator::genAffixes()
{
	sConfig->setSource("config.ini");
	shared_ptr<SqlConnector> db = SqlConnector::create(SqlType::SQLite, sConfig->getString("GameDb", "Name"), sConfig->getString("GameDb", "Path"));

	struct WordType
	{
		vector<string> adjective;
		vector<string> noun;
	};
		
	map<pair<UnitDefines::Stat, UnitDefines::Stat>, WordType> dictionary;
	
	dictionary[{UnitDefines::Stat::ResistFrost, {UnitDefines::Stat::NullStat}}] = 
	{ 
		// Adjectives
		{ "Frozen", "Frostbitten","Polar","Crystalline","Glacial" }, 

		// Nouns
		{ "Frost", "Tundra","Avalanche","Blizzard","Vortex" }
	};
	
	dictionary[{UnitDefines::Stat::ResistFire, {UnitDefines::Stat::NullStat}}] = 
	{ 
		// Adjectives
		{ "Warm", "Burning","Scalding","Scorching","Volcanic" }, 

		// Nouns
		{ "Flame", "Fire","Inferno","Magma","Cataclysm" }
	};
	

	dictionary[{UnitDefines::Stat::ResistShadow, {UnitDefines::Stat::NullStat}}] = 
	{ 
		// Adjectives
		{ "Dark", "Gloomy","Dreadful","Twisted","Chaotic" }, 

		// Nouns
		{ "Shades", "Shadows","Dusk","Nightfall","Nether" }
	};
	
	dictionary[{UnitDefines::Stat::ResistHoly, {UnitDefines::Stat::NullStat}}] = 
	{ 
		// Adjectives
		{ "Sparkling", "Shining","Consecrating","Blinding","Divine" }, 

		// Nouns
		{ "Glim", "Light","Illumination","Incandescence","Luminescence" }
	};
	
	dictionary[{UnitDefines::Stat::Mana, {UnitDefines::Stat::NullStat}}] = 
	{ 
		// Adjectives
		{ "Magical", "Fabled","Mystical","Mythical","Clairvoyant" }, 

		// Nouns
		{ "Novice", "Initiate","Adept","Elder","Wizard" }
	};
	
	dictionary[{UnitDefines::Stat::Health, {UnitDefines::Stat::NullStat}}] = 
	{ 
		// Adjectives
		{ "Tough", "Resilient","Unfading","Imperishable","Undying" }, 

		// Nouns
		{ "Vim", "Vigor","Vitality","Vivacity","Essence" }
	};
	
	dictionary[{UnitDefines::Stat::ArmorValue, {UnitDefines::Stat::NullStat}}] = 
	{ 
		// Adjectives
		{ "Stout", "Solid","Sturdy","Protective","Reliant" }, 

		// Nouns
		{ "Guard", "Barricade","Tower","Castle","Fortress" }
	};
	
	dictionary[{UnitDefines::Stat::Strength, {UnitDefines::Stat::NullStat}}] = 
	{ 
		// Adjectives
		{ "Strong", "Robust","Powerful","Mighty","Herculean" }, 

		// Nouns
		{ "Fist", "Ogre","Giant","Titan","Atlas" }
	};
	
	dictionary[{UnitDefines::Stat::Agility, {UnitDefines::Stat::NullStat}}] = 
	{ 
		// Adjectives
		{ "Agile", "Nimble","Deft","Cunning","Masterful" }, 

		// Nouns
		{ "Dexterity", "Exactness","Precision","Excellence","Perfection" }
	};
	
	dictionary[{UnitDefines::Stat::Willpower, {UnitDefines::Stat::NullStat}}] = 
	{ 
		// Adjectives
		{ "Ready", "Willing","Dilligent","Unrelenting","Unyielding" }, 

		// Nouns
		{ "Volunteering", "Dedication","Resolve","Unyielding","Perseverance" }
	};
	
	dictionary[{UnitDefines::Stat::Intelligence, {UnitDefines::Stat::NullStat}}] = 
	{ 
		// Adjectives
		{ "Smart", "Crafty","Mindful","Intuitive","Transcendent" }, 

		// Nouns
		{ "Brain", "Mind","Brilliance","Wisdom","Omnipotence" }
	};
	
	dictionary[{UnitDefines::Stat::Courage, {UnitDefines::Stat::NullStat}}] = 
	{ 
		// Adjectives
		{ "Courageous", "Bold","Gallant","Brave","Heroic" }, 

		// Nouns
		{ "Grit", "Fortitude","Honor","Valor","Martyr" }
	};
	
	dictionary[{UnitDefines::Stat::Regeneration, {UnitDefines::Stat::NullStat}}] = 
	{ 
		// Adjectives
		{ "Regenerating", "Healing","Hearty","Curative","Rejuvenating" }, 

		// Nouns
		{ "Relxation", "Rest","Health","Constitution","Blood" }
	};
	
	dictionary[{UnitDefines::Stat::Meditate, {UnitDefines::Stat::NullStat}}] = 
	{ 
		// Adjectives
		{ "Meditative", "Calming","Contemplative","Cerebral","Concentrating", }, 

		// Nouns
		{ "Rationale", "Thought","Reason","Rumination","Respose" }
	};
	
	dictionary[{UnitDefines::Stat::WeaponValue, {UnitDefines::Stat::NullStat}}] = 
	{ 
		// Adjectives
		{ "Slashing", "Slaying","Slaughtering","Dispatching","Butchering" }, 

		// Nouns
		{ "Attack", "Offense","Infiltration","Assault","Onslaught" }
	};
	
	dictionary[{UnitDefines::Stat::RangedWeaponValue, {UnitDefines::Stat::NullStat}}] = 
	{ 
		// Adjectives
		{ "Skillful", "Accurate","Superior","Piercing","Guiding" }, 

		// Nouns
		{ "Bowyer", "Fletcher","Archer","Expert","Marksman" }
	};
	
	dictionary[{UnitDefines::Stat::MeleeCritical, {UnitDefines::Stat::NullStat}}] = 
	{ 
		// Adjectives
		{ "Smashing", "Lacerating","Flaying","Mutilating","Ravaging" }, 

		// Nouns
		{ "Brute", "Slayer","Savage","Berserker","Disembowler" }
	};
	
	dictionary[{UnitDefines::Stat::RangedCritical, {UnitDefines::Stat::NullStat}}] = 
	{ 
		// Adjectives
		{ "Vollying", "Bombarding","Remorseless","Ruthless","Merciless" }, 

		// Nouns
		{ "Mess", "Gore","Carnage","Agony","Doom" }
	};
	
	dictionary[{UnitDefines::Stat::SpellCritical, {UnitDefines::Stat::NullStat}}] = 
	{ 
		// Adjectives
		{ "Potent", "Banishing","Shocking","Consuming","Diabolic" }, 

		// Nouns
		{ "Evoker", "Abjurer","Sorceror","Witch","Magus" }
	};
	
	dictionary[{UnitDefines::Stat::DodgeRating, {UnitDefines::Stat::NullStat}}] = 
	{ 
		// Adjectives
		{ "Ducking", "Diving","Darting","Eluding","Evading" }, 

		// Nouns
		{ "Dodge", "Avoidance","Sidestep","Diversion","Trickery" }
	};
	
	dictionary[{UnitDefines::Stat::BlockRating, {UnitDefines::Stat::NullStat}}] = 
	{ 
		// Adjectives
		{ "Blocking", "Encompassing","Shielding","Protecting","Guarding" }, 

		// Nouns
		{ "Barrier", "Formation","Bulwark","Tetsudo","Phalanx" }
	};
	
	/*dictionary[{UnitDefines::Stat::ParryRating, {UnitDefines::Stat::NullStat}}] = 
	{ 
		// Adjectives
		{ "Parrying", "Countering","Engaging","Matching","Dueling" }, 

		// Nouns
		{ "Epee", "Foil","Sabre","Feint","Riposte" }
	};*/
	
	dictionary[{UnitDefines::Stat::StaffSkill, {UnitDefines::Stat::NullStat}}] = 
	{ 
		// Adjectives
		{ "Swinging", "Thrusting","Sweeping","Striking","Dominating" }, 

		// Nouns
		{ "Balance", "Stability","Equilibrium","Recovery","Harmony" }
	};
	
	dictionary[{UnitDefines::Stat::MaceSkill, {UnitDefines::Stat::NullStat}}] = 
	{ 
		// Adjectives
		{ "Pounding", "Bashing","Crushing","Mauling","Devastating" }, 

		// Nouns
		{ "Rock", "Stone","Boulder","Mountain","Collosus" }
	};
	
	dictionary[{UnitDefines::Stat::AxesSkill, {UnitDefines::Stat::NullStat}}] = 
	{ 
		// Adjectives
		{ "Chopping", "Splitting","Cleaving","Severing","Rupturing" }, 

		// Nouns
		{ "Conflict", "Combat","Battle","War","Bloodshed" }
	};
	
	dictionary[{UnitDefines::Stat::SwordSkill, {UnitDefines::Stat::NullStat}}] = 
	{ 
		// Adjectives
		{ "Injuring", "Fierce","Severe","Sadistic","Cruel" }, 

		// Nouns
		{ "Squire", "Soldier","Crusader","Knight","King" }
	};
	
	dictionary[{UnitDefines::Stat::RangedSkill, {UnitDefines::Stat::NullStat}}] = 
	{ 
		// Adjectives
		{ "Stabilizing", "Sighting","Targeting","Deadly","Maiming" }, 

		// Nouns
		{ "Bowman", "Hunter","Sniper","Sharpshooter","Bullseye" }
	};
	
	dictionary[{UnitDefines::Stat::DaggerSkill, {UnitDefines::Stat::NullStat}}] = 
	{ 
		// Adjectives
		{ "Penetrating","Wounding","Stabbing","Puncturing","Eviscerating" }, 

		// Nouns
		{ "Thief", "Bandit","Brigand","Cutthroat","Assassin"}
	};
	
	dictionary[{UnitDefines::Stat::WandSkill, {UnitDefines::Stat::NullStat}}] = 
	{ 
		// Adjectives
		{ "Charming", "Enchanting","Alluring","Tempting","Beguiling" }, 

		// Nouns
		{ "Incantation", "Voodoo","Occultism","Spellcraft","Witchcraft" }
	};
	
	dictionary[{UnitDefines::Stat::ShieldSkill, {UnitDefines::Stat::NullStat}}] = 
	{ 
		// Adjectives
		{ "Covering", "Fending","Negating","Armored","Defiant" }, 

		// Nouns
		{ "Absorption", "Salvation","Deflection","Impregnability","Invulnerability" }
	};
	
	dictionary[{UnitDefines::Stat::Health, UnitDefines::Stat::Mana}] = 
	{ 
		// Adjectives
		{ "Ample", "Copious","Plentiful","Abundant","Bountiful" },

		// Nouns
		{ }
	};
	
	dictionary[{UnitDefines::Stat::Strength, UnitDefines::Stat::Agility}] = 
	{ 
		// Adjectives
		{ },

		// Nouns
		{ "Tamarin", "Baboon","Chimpanzee","Gibbon","Gorilla" }
	};
	
	dictionary[{UnitDefines::Stat::Strength, UnitDefines::Stat::Willpower}] = 
	{ 
		// Adjectives
		{ },

		// Nouns
		{ "Crab", "Turtle","Octopus","Shark","Whale" }
	};
	
	dictionary[{UnitDefines::Stat::Strength, UnitDefines::Stat::Intelligence}] = 
	{ 
		// Adjectives
		{ },

		// Nouns
		{ "Duck", "Owl","Fox","Pig","Bear" }
	};
	
	dictionary[{UnitDefines::Stat::Strength, UnitDefines::Stat::Courage}] = 
	{ 
		// Adjectives
		{ },

		// Nouns
		{ "Badger","Ostrich","Eagle","Lion","Elephant" }
	};
	
	dictionary[{UnitDefines::Stat::Agility, UnitDefines::Stat::Willpower}] = 
	{ 
		// Adjectives
		{ },

		// Nouns
		{ "Mouse","Squirrel","Raccoon","Cougar","Tiger" }
	};
	
	dictionary[{UnitDefines::Stat::Agility, UnitDefines::Stat::Willpower}] = 
	{ 
		// Adjectives
		{ },

		// Nouns
		{ "Rabbit","Beaver","Crow","Coyote","Snake" }
	};
	
	dictionary[{UnitDefines::Stat::Agility, UnitDefines::Stat::Courage}] = 
	{ 
		// Adjectives
		{ },

		// Nouns
		{ "Bat","Opossum","Weasel","Deer","Wolf" }
	};
	
	dictionary[{UnitDefines::Stat::Willpower, UnitDefines::Stat::Intelligence}] = 
	{ 
		// Adjectives
		{ },

		// Nouns
		{ "Shrew","Muskrat","Mink","Otter","Dolphin" }
	};
	
	dictionary[{UnitDefines::Stat::Willpower, UnitDefines::Stat::Courage}] = 
	{ 
		// Adjectives
		{ },

		// Nouns
		{ "Rat","Dog","Mule","Donkey","Horse" }
	};
	
	dictionary[{UnitDefines::Stat::Intelligence, UnitDefines::Stat::Courage}] = 
	{ 
		// Adjectives
		{ },

		// Nouns
		{ "Spider","Lizard","Raven","Goat","Squid" }
	};

	set<pair<pair<UnitDefines::Stat, UnitDefines::Stat>, pair<UnitDefines::Stat, UnitDefines::Stat>>> types;

	for (auto& group1 : dictionary)
	{
		if (group1.first.second == 0)
			types.insert({ group1.first, { UnitDefines::Stat::NullStat, UnitDefines::Stat::NullStat } });

		for (auto& group2 : dictionary)
		{
			// Duplicates
			if (group1.first == group2.first)
				continue;

			// Check if we already have a define for when these two stats are merged 
			if (group1.first.second == 0 && group2.first.second == 0 &&
				dictionary.find({ group1.first.first, group2.first.first }) != dictionary.end())
				continue;

			/*map<UnitDefines::Stat, int> count;
			
			count[group1.first.first]++;
			count[group1.first.second]++;
			count[group2.first.first]++;
			count[group2.first.second]++;

			bool hasDupes = false;

			for (auto& itr : count)
			{
				if (itr.first != 0 && itr.second > 1)
					hasDupes = true;
			}*/

			// Can't pair two groups together if they both only have nouns, or they both only have adjectives
			if ((dictionary[group1.first].noun.empty() && dictionary[group2.first].noun.empty()) ||
				(dictionary[group1.first].adjective.empty() && dictionary[group2.first].adjective.empty()))
			{
				continue;
			}

			//if (!hasDupes)
				types.insert({ group1.first, group2.first });
		}
	}

	set<string> singleNouns = 
	{
		"Novice",
		"Initiate",
		"Adept",
		"Elder",
		"Wizard",
		"Guard",
		"Barricade",
		"Tower",
		"Castle",
		"Fortress",
		"Fist",
		"Ogre",
		"Giant",
		"Titan",
		"Atlas",
		"Brain",
		"Mind",
		"Sky",
		"Clouds",
		"Moon",
		"Stars",
		"Heavens",
		"Tamarin",
		"Baboon",
		"Chimpanzee",
		"Gibbon",
		"Gorilla",
		"Crab",
		"Turtle",
		"Octopus",
		"Shark",
		"Whale",
		"Duck",
		"Owl",
		"Fox",
		"Pig",
		"Bear",
		"Badger",
		"Ostrich",
		"Eagle",
		"Lion",
		"Elephant",
		"Mouse",
		"Squirrel",
		"Raccoon",
		"Cougar",
		"Tiger",
		"Rabbit",
		"Beaver",
		"Crow",
		"Coyote",
		"Snake",
		"Bat",
		"Opossum",
		"Weasel",
		"Deer",
		"Wolf",
		"Shrew",
		"Muskrat",
		"Mink",
		"Otter",
		"Dolphin",
		"Rat",
		"Dog",
		"Mule",
		"Donkey",
		"Horse",
		"Spider",
		"Lizard",
		"Raven",
		"Goat",
		"Squid",
		"Bowyer",
		"Fletcher",
		"Archer",
		"Expert",
		"Marksman",
		"Brute",
		"Slayer",
		"Savage",
		"Berserker",
		"Disembowler",
		"Invoker",
		"Abjurer",
		"Sorceror",
		"Witch",
		"Magus",
		"Barrier",
		"Formation",
		"Bulwark",
		"Tetsudo",
		"Phalanx",
		"Epee",
		"Foil",
		"Sabre",
		"Feint",
		"Riposte",
		"Rock",
		"Stone",
		"Boulder",
		"Mountain",
		"Collosus",
		"Bowman",
		"Hunter",
		"Sniper",
		"Sharpshooter",
		"Bullseye",
		"Thief",
		"Bandit",
		"Brigand",
		"Cuthroat",
		"Assassin",
	};

	struct Template
	{
		pair<int, int> lvlRange;
		vector<pair<UnitDefines::Stat, float>> stats;
		string name;
		bool name_single_noun{false};
	};

	vector<Template> templates;
	vector<float> modifiers = { 1.0, 1.125, 1.250, 1.375, 1.5 };
	vector<pair<int, int>> lvlRanges = { {1,5}, {6,13}, {14,19}, {20,25}, {25,25} };

	for (const pair<pair<UnitDefines::Stat, UnitDefines::Stat>, pair<UnitDefines::Stat, UnitDefines::Stat>>& itr : types)
	{
		for (size_t lvl = 0; lvl < modifiers.size(); ++lvl)
		{
			Template t;
			t.lvlRange = lvlRanges[lvl];
			
			pair<UnitDefines::Stat, UnitDefines::Stat> firstType = itr.first;
			pair<UnitDefines::Stat, UnitDefines::Stat> secondType = itr.second;

			float baseMod = modifiers[lvl];

			if (firstType.second == 0 && secondType.first == 0)
			{
				string noun = dictionary[firstType].noun[lvl];

				// A single stat and nothing else
				UnitDefines::Stat stat1 = firstType.first;
				t.name = noun.empty() ? UnitFunctions::statName(stat1) : noun;
				t.stats = { { stat1, 2.f * baseMod } };

				if (!noun.empty())
					t.name_single_noun = true;
			}
			else
			{
				if (firstType.first != 0 && secondType.first != 0 && Util::roll_chance_i(50))
					swap(firstType, secondType);

				if (dictionary[firstType].noun.empty() || dictionary[secondType].adjective.empty())
					swap(firstType, secondType);
			
				string noun = dictionary[firstType].noun[lvl];
				string adjective = dictionary[secondType].adjective[lvl];

				if (singleNouns.find(noun) != singleNouns.end())
					t.name_single_noun = true;

				int totalStats = 0;

				if (firstType.first != 0)
					++totalStats;

				if (firstType.second != 0)
					++totalStats;

				if (secondType.first != 0)
					++totalStats;

				if (secondType.second != 0)
					++totalStats;

				if (totalStats == 3)
					baseMod *= 0.85f;

				if (totalStats == 4)
					baseMod *= 0.75f;
			
				t.name = adjective + " " + noun;

				if (secondType.second == 0)
					t.stats.push_back( { secondType.first, 2.f * baseMod } );
				else
				{
					t.stats.push_back( { secondType.first, 2.f * baseMod } );
					t.stats.push_back( { secondType.second, 2.f * baseMod } );
				}

				// 2.f stats if its solo
				if (firstType.second == 0)
					t.stats.push_back( { firstType.first, 2.f * baseMod } );
				else
				{
					t.stats.push_back( { firstType.first, 2.f * baseMod } );
					t.stats.push_back( { firstType.second, 2.f * baseMod } );
				}
			}

			templates.push_back(t);
		}
	}

	for (size_t i = 0 ; i < templates.size(); ++i)
	{
		auto& templ = templates[i];

		if (templ.stats.size() < 4)
			templ.stats.resize(4, { UnitDefines::Stat::NullStat, 0.f });

		string query = Util::fmtStr("REPLACE INTO 'affix_template' ('name', 'name_single_noun', 'min_level', 'max_level', \
			'stat_type1', 'stat_value1', \
			'stat_type2', 'stat_value2', \
			'stat_type3', 'stat_value3', \
			'stat_type4', 'stat_value4') VALUES ('%s', '%d', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s');",	

			// 'name', 'name_single_noun', 'min_level', 'max_level'
			templ.name.c_str(), 
			templ.name_single_noun, 
			to_string(templ.lvlRange.first).c_str(), 
			to_string(templ.lvlRange.second).c_str(), 
			
			// 'stat_type1', 'stat_value1', 
			to_string(templ.stats[0].first).c_str(), 
			to_string(templ.stats[0].second).c_str(),
			
			// 'stat_type2', 'stat_value2', 
			to_string(templ.stats[1].first).c_str(), 
			to_string(templ.stats[1].second).c_str(),
			
			// 'stat_type3', 'stat_value3', 
			to_string(templ.stats[2].first).c_str(), 
			to_string(templ.stats[2].second).c_str(),
			
			// 'stat_type4', 'stat_value4', 
			to_string(templ.stats[3].first).c_str(), 
			to_string(templ.stats[3].second).c_str());
		
		db->query(query);
	}
}

/*
vector<pair<vector<string>, vector<string>>> vec =

	// Large Drake
	{
		{
			{	"Large Fang",
				"Frayed Drake Talons",
				"Broken Animal Bone",
				"Ancient Fossil",
				"Animal Leg",
				"Chipped Tooth",
				"Strange Coin",
				"Large Animal Claw",
			},

			{
				"wyvern_adult"
			}
		},

		{
			{
				"Small Broken Claw",
				"Frayed Drake Talons",
				"Broken Animal Bone",
				"Ancient Fossil",
				"Animal Leg",
				"Chipped Tooth",
				"Strange Coin",
			},

			{
				"wyvern",
				"wyvern_air",
				"wyvern_fire",
				"wyvern_water"
			}
		},

		{
			{
				"Small Broken Claw",
				"Broken Insect Claw",
				"Insect Scale"
			},

			{
				"antlion_small",
				"spider"
			}
		},

		{
			{
				"Small Broken Claw",
				"Broken Insect Claw",
				"Insect Scale",
				"Insect Leg",
				"Insect Arm",
				"Large Animal Claw"
			},

			{
				"antlion",
				"fire_ant",
				"ice_ant",
				"spider_giant",
				"spider_large"
			}
		},

		{
			{
				"Small Broken Claw",
				"Animal Hair",
				"Wolf Fang",
				"Large Fang",
				"Chipped Tooth",
				"Large Animal Claw",
				"Animal Leg",
				"Patch of Fur"
			},

			{
				"werewolf"
			},
		},

		{
			{
				"Destroyed Jewelry",
				"Shiny Gem",
				"Large Mineral",
				"Strange Mineral",
				"Shiny Mineral"
			},

			{
				"goblin",
				"goblin_elite",
				"goblin_elite_jumper",
				"goblin_jumper",
				"hobgoblin",
				"hobgoblin_archer",
				"goblin_spearman",
				"goblin_spearman_elite",
				"goblin_spearman_elite_jumper",
				"goblin_spearman_jumper"
			}
		},

		{
			{
				"Scraps of Metal",
				"Chunk of Metal",
				"Destroyed Merchandise",
				"Piece of Metal",
				"Metal Scraps",
				"Piece of Metal",
				"Broken Sword",
				"Strange Coin",
			},
			
			{
				"scathelocke",
				"orc_archer",
				"orc_elite",
				"orc_heavy",
				"orc_regular",
				"vesuvvio",
				"grisbon"
			}
		},
			
		{
			{
				"Broken Skeleton Bone",
				"Fractured Skull",
				"Large Mineral",
				"Strange Mineral",
				"Shiny Mineral",
				"Broken Sword",
				"Strange Coin"
			},

			{
				"zombie",
				"zombie_dark",
				"pirate_ghost_captain",
				"pirate_ghost_crewman",
				"cursed_grave",
				"skeleton",
				"skeleton_archer",
				"skeleton_knight",
				"skeleton_knight_boss",
				"skeleton_mage",
				"skeleton_mage_boss",
				"skeleton_mage_high",
				"skeleton_mage_high_boss",
				"skeleton_occultist",
				"skeleton_weak",
			}
		},

		{
			{
				"Hoof",
				"Large Mineral",
				"Strange Mineral",
				"Shiny Mineral"
			},

			{
				"minotaur",
				"minotaur_armor"
			}
		}
	};

	for (auto& itr : vec)
	{
		const auto& itemNames = itr.first;
		const auto& modelNames = itr.second;

		for (auto& modelName : modelNames)
		{
			for (auto& itemName : itemNames)
			{
				if (auto modelResult = db->query(Util::fmtStr("SELECT id FROM npc_models WHERE name = '%s';", modelName.c_str())))
				{
					int modelId = modelResult->fetchData()[0][0].getInt32();

					if (auto itemsResult = db->query(Util::fmtStr("SELECT entry FROM item_template WHERE name LIKE '%%%s%%';", itemName.c_str())))
					{
						for (auto& itr : itemsResult->fetchData())
						{
							int itemEntry = itr[0].getInt32();

							string query = Util::fmtStr("INSERT INTO `npc_models_junkloot` (`model_id`, `item_entry`) VALUES ('%d', '%d');", modelId, itemEntry);
							db->query(query);

							if (!db->error().empty())
								printf("");
						}
					}
				}
			}
		}
	}

*/