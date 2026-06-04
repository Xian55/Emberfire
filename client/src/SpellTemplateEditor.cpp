#include "stdafx.h"
#include "SpellTemplateEditor.h"
#include "ContentMgr.h"
#include "TextBoxRo.h"
#include "PromptBox.h"
#include "Application.h"
#include "ConfirmMessageBox.h"
#include "Sprite.h"
#include "SpellIcon.h"
#include "ClientMap.h"
#include "ClientNpc.h"
#include "WorldSpellAnimation.h"
#include "SpellVisualKit.h"
#include "Abilities.h"

#include "..\Files.h"
#include "..\Vector.h"
#include "..\StringHelpers.h"

#include "..\Shared\Config.h"
#include "..\Shared\UnitDefines.h"
#include "..\Shared\Config.h"

#include "..\SqlConnector\SqlConnector.h"
#include "..\SqlConnector\QueryResult.h"

SpellTemplateEditor::SpellTemplateEditor(RenderObjectHolder& owner, const int id) :
	TableTemplateEditor(owner, id)
{
	m_map = make_unique<ClientMap>(*this, 0);
	m_map->setRenderEmptyCells(true);
	m_map->setAllowCameraDrag(true);
	m_map->setShowClouds(false);
	m_map->loadFromDisk("debug");
	m_map->getCameraRef() = { 1553.f, 319.f };

	// Caster
	m_unitCaster = make_shared<ClientNpc>(1, m_map.get());
	m_unitCaster->setEntry(1);
	m_unitCaster->setVariable(ObjDefines::Variable::ModelId, 49);
	m_unitCaster->setWorldPosition(sf::Vector2f(static_cast<float>(1.5f), static_cast<float>(23.5f)));
	m_map->addWorldRenderable(m_unitCaster);
	
	// Target
	m_unitVictim = make_shared<ClientNpc>(2, m_map.get());
	m_unitVictim->setEntry(1);
	m_unitVictim->setVariable(ObjDefines::Variable::ModelId, 49);
	m_unitVictim->setWorldPosition(sf::Vector2f(static_cast<float>(8.5f), static_cast<float>(15.5f)));
	m_map->addWorldRenderable(m_unitVictim);

	m_numFields = Field::NumFields;
	m_entryFieldId = Field::STF_entry;

	setMultiInput(true);
	initDictionary();

	setEntry(sConfig->getInt("SpellTemplateEditor", "Entry"));
}

SpellTemplateEditor::~SpellTemplateEditor()
{

}

void SpellTemplateEditor::input() /*final*/
{
	m_map->attemptInput();
	
	if (sApplication->popKeyUp(sf::Keyboard::P))
	{		
		// Start casting
		m_animCasting = make_shared<WorldSpellAnimation>(m_map.get(), m_entry, true);
		m_animCasting->setSource(m_unitCaster);
		m_animCasting->setTarget(m_unitVictim);
		m_animCasting->setTimer(atoi(fieldValue(STF_cast_time).c_str()));
		m_map->addWorldRenderable(m_animCasting);
	}

	__super::input();

	// Confirmation boxes	
	if (auto confirmBox = sApplication->popConfirmBox({ ConfirmMessageBox::ConfirmCode_SpellEditorNewKit }))
	{
		if (confirmBox->getResult() == ConfirmMessageBox::ConfirmBox_Yes)
		{
			shared_ptr<SqlConnector> db = SqlConnector::create(SqlType::SQLite, sConfig->getString("GameDb", "Name"), sConfig->getString("GameDb", "Path"));

			if (auto result = db->query("SELECT MAX(id) FROM `spell_visual_kit`;"))
			{
				m_kitEntry = result->fetchData()[0][0].getInt32() + 1;
				db->query(Util::fmtStr("INSERT INTO `spell_visual_kit` ('id') VALUES ('%d');", m_kitEntry));

				string err = db->error();

				if (!err.empty())
				{
					m_kitEntry = 0;
					sApplication->spawnPopup(err, ConfirmMessageBox::ConfirmBox_Ok, ConfirmMessageBox::ConfirmCode_Null);
				}
				else
				{
					setFieldValue(m_newKitField, to_string(m_kitEntry));
				}

				setEntry(m_entry);
				sContentMgr->loadDbTable(db, "spell_visual_kit");
			}
		}
	}
}

void SpellTemplateEditor::render() /*final*/
{
	// Finished casting
	if (m_animCasting != nullptr && m_animCasting->done())
	{
		m_animCasting = nullptr;

		// Traveling
		m_animGo = make_shared<WorldSpellAnimation>(m_map.get(), m_entry);
		m_animGo->setSource(m_unitCaster);
		m_animGo->setTarget(m_unitVictim);
		m_map->addWorldRenderable(m_animGo);
		
		// Go
		auto sa = make_shared<WorldSpellAnimation>(m_map.get(), m_entry, false, true);
		sa->setSource(m_unitCaster);
		sa->setTarget(m_unitCaster);
		m_map->addWorldRenderable(sa);

		int animId = atoi(fieldValue(SVF_unit_go_animation).c_str());
		m_unitCaster->playAnimation(static_cast<UnitDefines::AnimId>(animId));
	}

	// Finished traveling
	if (m_animGo != nullptr && m_animGo->done())
	{
		m_animGo = nullptr;

		// Apply aura
		m_unitVictim->clearAuras();
		m_unitVictim->auraAnimationIncrement(m_entry);
	}

	m_map->attemptRender();	
	__super::render();
}

void SpellTemplateEditor::initEffectDictionary(const int eff, const int data1, const int data2, const int data3)
{
	m_conditionalLabels.push_back(
	{
		{
			{ eff, SpellDefines::Effects::WeaponDamage }
		},

		{
			{ data1, "Magic School", &m_strMagicSchool, false },
			{ data2, "Damage%", nullptr, false },
			{ data3, "Weapon Slot", &m_strEquipSlots, false },
		}
	});

	m_conditionalLabels.push_back(
	{
		{
			{ eff, SpellDefines::Effects::ScriptEffect }
		},

		{
			{ data1, "Script ID", nullptr, false }
		}
	});	

	m_conditionalLabels.push_back(
	{
		{
			{ eff, SpellDefines::Effects::Resurrect }
		},

		{
			{ data1, "Health%", nullptr, false },
			{ data2, "Mana%", nullptr, false },
		}
	});	

	m_conditionalLabels.push_back(
	{
		{
			{ eff, SpellDefines::Effects::Threat }
		},

		{
			{ data1, "Amount Flat", nullptr, false },
			{ data2, "Amount Pct", nullptr, false },
		}
	});	

	m_conditionalLabels.push_back(
	{
		{
			{ eff, SpellDefines::Effects::LearnSpell }
		},

		{
			{ data1, "Spell", &m_strEntries, false },
		}
	});	
	
	m_conditionalLabels.push_back(
	{
		{
			{ eff, SpellDefines::Effects::Heal }
		},

		{
			{ data1, "Amount", nullptr, false },
			{ data2, "Variance%", nullptr, false },
		}
	});
	
	m_conditionalLabels.push_back(
	{
		{
			{ eff, SpellDefines::Effects::RestoreMana }
		},

		{
			{ data1, "Amount", nullptr, false },
		}
	});
	
	m_conditionalLabels.push_back(
	{
		{
			{ eff, SpellDefines::Effects::RestoreManaPct }
		},

		{
			{ data1, "Amount%", nullptr, false },
		}
	});
	
	m_conditionalLabels.push_back(
	{
		{
			{ eff, SpellDefines::Effects::HealPct }
		},

		{
			{ data1, "Amount", nullptr, false },
		}
	});

	m_conditionalLabels.push_back(
	{
		{
			{ eff, SpellDefines::Effects::ManaBurn }
		},

		{
			{ data1, "Amount",  nullptr, false },
			{ data2, "Variance%", nullptr, false },
		}
	});

	m_conditionalLabels.push_back(
	{
		{
			{ eff, SpellDefines::Effects::SchoolDamage }
		},

		{
			{ data1, "Magic School", &m_strMagicSchool, false },
			{ data2, "Damage", nullptr, false },
			{ data3, "Dmg Variance%", nullptr, false },
		}
	});

	m_conditionalLabels.push_back(
	{
		{
			{ eff, SpellDefines::Effects::Dispel }
		},

		{
			{ data1, "Dispel Types", &m_strDispelType, true },
			{ data2, "Count", nullptr, false },
		}
	});

	m_conditionalLabels.push_back(
	{
		{
			{ eff, SpellDefines::Effects::KnockBack }
		},

		{
			{ data1, "Distance", nullptr, false },
		}
	});

	m_conditionalLabels.push_back(
	{
		{
			{ eff, SpellDefines::Effects::SlideFrom }
		},

		{
			{ data1, "Distance", nullptr, false },
		}
	});

	m_conditionalLabels.push_back(
	{
		{
			{ eff, SpellDefines::Effects::LootEffect }
		},

		{
			{ data1, "Lockpicking", &m_strBoolean, false },
		}
	});

	m_conditionalLabels.push_back(
	{
		{
			{ eff, SpellDefines::Effects::ApplyGemSocket }
		},

		{
			{ data1, "Gem", &m_strGems, false },
		}
	});

	m_conditionalLabels.push_back(
	{
		{
			{ eff, SpellDefines::Effects::ApplyOrbEnchant }
		},

		{
			{ data1, "Quality", &m_strQualities, false },
			{ data2, "Equip Slots", &m_strEquipTypes, true },
		}
	});

	auto configApplyAura = [&](const SpellDefines::Effects applyAura)
	{
		m_conditionalLabels.push_back(
		{
			{
				{ eff, applyAura }
			},

			{
				{ data1, "Aura Type", &m_strAuraType, false }
			}
		});

		m_conditionalLabels.push_back(
		{
			{
				{ eff, applyAura },
				{ data1, SpellDefines::AuraType::PeriodicDamage }
			},

			{
				{ data2, "Magic School", &m_strMagicSchool, false },
				{ data3, "Amount", nullptr, false }
			}
		});

		m_conditionalLabels.push_back(
		{
			{
				{ eff, applyAura },
				{ data1, SpellDefines::AuraType::PeriodicMeleeDamage }
			},

			{
				{ data2, "Magic School", &m_strMagicSchool, false },
				{ data3, "Amount%", nullptr, false }
			}
		});

		m_conditionalLabels.push_back(
		{
			{
				{ eff, applyAura },
				{ data1, SpellDefines::AuraType::Model }
			},

			{
				{ data2, "ModelId", &m_strNpcModels, false },
			}
		});

		m_conditionalLabels.push_back(
		{
			{
				{ eff, applyAura },
				{ data1, SpellDefines::AuraType::PeriodicHeal }
			},

			{
				{ data2, "Amount", nullptr, false }
			}
		});

		m_conditionalLabels.push_back(
		{
			{
				{ eff, applyAura },
				{ data1, SpellDefines::AuraType::PeriodicHealPct }
			},

			{
				{ data2, "Amount%", nullptr, false }
			}
		});

		m_conditionalLabels.push_back(
		{
			{
				{ eff, applyAura },
				{ data1, SpellDefines::AuraType::PeriodicRestoreManaPct }
			},

			{
				{ data2, "Amount%", nullptr, false }
			}
		});

		m_conditionalLabels.push_back(
		{
			{
				{ eff, applyAura },
				{ data1, SpellDefines::AuraType::InflictMechanic }
			},

			{
				{ data2, "Mechanic", &m_strMechanics, false }
			}
		});

		m_conditionalLabels.push_back(
		{
			{
				{ eff, applyAura },
				{ data1, SpellDefines::AuraType::InflictMechanic },
				{ data2, SpellDefines::Mechanics::Snare }
			},

			{
				{ data3, "AmountPct", nullptr, false }
			}
		});

		m_conditionalLabels.push_back(
		{
			{
				{ eff, applyAura },
				{ data1, SpellDefines::AuraType::ModifyStat }
			},
		
			{
				{ data2, "Stats", &m_strStats, true },
				{ data3, "Amount", nullptr, false }
			}
		});

		m_conditionalLabels.push_back(
		{
			{
				{ eff, applyAura },
				{ data1, SpellDefines::AuraType::ModifyStatPct }
			},
		
			{
				{ data2, "Stats", &m_strStats, true },
				{ data3, "Amount%", nullptr, false }
			}
		});

		m_conditionalLabels.push_back(
		{
			{
				{ eff, applyAura },
				{ data1, SpellDefines::AuraType::AbsorbDamage }
			},
		
			{
				{ data2, "Amount", nullptr, false },
				{ data3, "Magic Schools", &m_strMagicSchool, true },
			}
		});

		m_conditionalLabels.push_back(
		{
			{
				{ eff, applyAura },
				{ data1, SpellDefines::AuraType::ModifyResistance }
			},
		
			{
				{ data2, "Magic Schools", &m_strMagicSchool, true },
				{ data3, "Amount", nullptr, false }
			}
		});

		m_conditionalLabels.push_back(
		{
			{
				{ eff, applyAura },
				{ data1, SpellDefines::AuraType::ModifyHealingDealtPct }
			},
		
			{
				{ data2, "AmountPct", nullptr, false },
			}
		});

		m_conditionalLabels.push_back(
		{
			{
				{ eff, applyAura },
				{ data1, SpellDefines::AuraType::ModifyHealingRecvPct }
			},
		
			{
				{ data2, "AmountPct", nullptr, false },
			}
		});

		m_conditionalLabels.push_back(
		{
			{
				{ eff, applyAura },
				{ data1, SpellDefines::AuraType::PeriodicTriggerSpell }
			},
		
			{
				{ data2, "Spell Id", nullptr, false },
			}
		});

		m_conditionalLabels.push_back(
		{
			{
				{ eff, applyAura },
				{ data1, SpellDefines::AuraType::PeriodicRestoreMana }
			},
		
			{
				{ data2, "Amount", nullptr, false },
			}
		});

		m_conditionalLabels.push_back(
		{
			{
				{ eff, applyAura },
				{ data1, SpellDefines::AuraType::PeriodicBurnMana }
			},
		
			{
				{ data2, "Amount", nullptr, false },
			}
		});

		m_conditionalLabels.push_back(
		{
			{
				{ eff, applyAura },
				{ data1, SpellDefines::AuraType::ModifyMoveSpeedPct }
			},
		
			{
				{ data2, "Amount%", nullptr, false },
			}
		});

		m_conditionalLabels.push_back(
		{
			{
				{ eff, applyAura },
				{ data1, SpellDefines::AuraType::ModifyMeleeSpeedPct }
			},
		
			{
				{ data2, "Amount%", nullptr, false },
			}
		});

		m_conditionalLabels.push_back(
		{
			{
				{ eff, applyAura },
				{ data1, SpellDefines::AuraType::ModifyRangedSpeedPct }
			},
		
			{
				{ data2, "Amount%", nullptr, false },
			}
		});

		m_conditionalLabels.push_back(
		{
			{
				{ eff, applyAura },
				{ data1, SpellDefines::AuraType::MechanicImmunity }
			},
		
			{
				{ data2, "Mechanics", &m_strMechanics, true },
				{ data3, "Remove Debuffs", &m_strBoolean, false }
			}
		});

		m_conditionalLabels.push_back(
		{
			{
				{ eff, applyAura },
				{ data1, SpellDefines::AuraType::SchoolImmunity }
			},
		
			{
				{ data2, "Magic Schools", &m_strMagicSchool, true },
				{ data3, "Remove Debuffs", &m_strBoolean, false }
			}
		});

		m_conditionalLabels.push_back(
		{
			{
				{ eff, applyAura },
				{ data1, SpellDefines::AuraType::ModifyDamageDealtPct }
			},
		
			{
				{ data2, "Magic Schools", &m_strMagicSchool, true },
				{ data3, "Amount%", nullptr, false }
			}
		});

		m_conditionalLabels.push_back(
		{
			{
				{ eff, applyAura },
				{ data1, SpellDefines::AuraType::ModifyDamageReceivedPct }
			},
		
			{
				{ data2, "Magic Schools", &m_strMagicSchool, true },
				{ data3, "Amount%", nullptr, false }
			}
		});

		m_conditionalLabels.push_back(
		{
			{
				{ eff, applyAura },
				{ data1, SpellDefines::AuraType::Proc }
			},
		
			{
				{ data2, "ProcFlags", &m_strProcFlags, true },
				{ data3, "ProcType", &m_strProcType, false }
			}
		});
	};
	
	configApplyAura(SpellDefines::Effects::ApplyAura);
	configApplyAura(SpellDefines::Effects::ApplyAreaAura);
}

void SpellTemplateEditor::initDictionary()
{
	vector<string> files;
	Util::getFileList("scripts\\animation", files);

	for (auto& itr : files)
	{
		Util::strReplaceAll(itr, "scripts\\animation//", "");
		m_strSpriteSheets[itr] = itr;
	}

	files.clear();
	Util::getFileList("scripts\\particles", files);

	for (auto& itr : files)
	{
		Util::strReplaceAll(itr, "scripts\\particles//", "");
		m_strPsystems[itr] = itr;
	}

	m_strGems.clear();
	auto& ig = sContentMgr->db("item_gems");
	
	for (auto& itr : ig.fetchIndexData())
	{
		string entry = itr.first.c_str();		
		auto nameitr = itr.second.find("name");
				
		if (nameitr != itr.second.end())
		{
			string name = nameitr->second.getString();
			m_strGems[entry] = name;
		}
	}

	__super::initDictionary();
	
	m_strQualities[to_string(ItemDefines::Quality::QualityLv0)] = ItemFunctions::qualityName(ItemDefines::Quality::QualityLv0);
	m_strQualities[to_string(ItemDefines::Quality::QualityLv1)] = ItemFunctions::qualityName(ItemDefines::Quality::QualityLv1);
	m_strQualities[to_string(ItemDefines::Quality::QualityLv2)] = ItemFunctions::qualityName(ItemDefines::Quality::QualityLv2);
	m_strQualities[to_string(ItemDefines::Quality::QualityLv3)] = ItemFunctions::qualityName(ItemDefines::Quality::QualityLv3);
	m_strQualities[to_string(ItemDefines::Quality::QualityLv4)] = ItemFunctions::qualityName(ItemDefines::Quality::QualityLv4);
	m_strQualities[to_string(ItemDefines::Quality::QualityLv5)] = ItemFunctions::qualityName(ItemDefines::Quality::QualityLv5);
	
	m_strEquipTypes[to_string(ItemDefines::EquipType::Helm)] = ItemFunctions::equipTypeName(ItemDefines::EquipType::Helm);
	m_strEquipTypes[to_string(ItemDefines::EquipType::Necklace)] = ItemFunctions::equipTypeName(ItemDefines::EquipType::Necklace);
	m_strEquipTypes[to_string(ItemDefines::EquipType::Chest)] = ItemFunctions::equipTypeName(ItemDefines::EquipType::Chest);
	m_strEquipTypes[to_string(ItemDefines::EquipType::Belt)] = ItemFunctions::equipTypeName(ItemDefines::EquipType::Belt);
	m_strEquipTypes[to_string(ItemDefines::EquipType::Legs)] = ItemFunctions::equipTypeName(ItemDefines::EquipType::Legs);
	m_strEquipTypes[to_string(ItemDefines::EquipType::Feet)] = ItemFunctions::equipTypeName(ItemDefines::EquipType::Feet);
	m_strEquipTypes[to_string(ItemDefines::EquipType::Hands)] = ItemFunctions::equipTypeName(ItemDefines::EquipType::Hands);
	m_strEquipTypes[to_string(ItemDefines::EquipType::Ring)] = ItemFunctions::equipTypeName(ItemDefines::EquipType::Ring);
	m_strEquipTypes[to_string(ItemDefines::EquipType::Weapon)] = ItemFunctions::equipTypeName(ItemDefines::EquipType::Weapon);
	m_strEquipTypes[to_string(ItemDefines::EquipType::Shield)] = ItemFunctions::equipTypeName(ItemDefines::EquipType::Shield);
	m_strEquipTypes[to_string(ItemDefines::EquipType::Ranged)] = ItemFunctions::equipTypeName(ItemDefines::EquipType::Ranged);

	m_strProcFlags[to_string(SpellDefines::ProcFlags::ProcFlag_HolderTookDamage)] = "HolderTookDamage";
	m_strProcFlags[to_string(SpellDefines::ProcFlags::ProcFlag_HolderDealtDamage)] = "HolderDealtDamage";
	m_strProcFlags[to_string(SpellDefines::ProcFlags::ProcFlag_HolderWasImmune)] = "HolderWasImmune";
	m_strProcFlags[to_string(SpellDefines::ProcFlags::ProcFlag_HolderDodged)] = "HolderDodged";
	
	m_strProcType[to_string(SpellDefines::ProcType::ProcType_RemoveCharge)] = "RemoveCharge";

	initEffectDictionary(STF_effect1, STF_effect1_data1, STF_effect1_data2, STF_effect1_data3);
	initEffectDictionary(STF_effect2, STF_effect2_data1, STF_effect2_data2, STF_effect2_data3);
	initEffectDictionary(STF_effect3, STF_effect3_data1, STF_effect3_data2, STF_effect3_data3);
}

void SpellTemplateEditor::setEntry(const int spellId)
{
	if (m_entry != spellId)
		m_kitEntry = 0;

	m_entry = spellId;

	sConfig->setInt("SpellTemplateEditor", "Entry", m_entry);

	m_maskedValues.clear();
	m_maskedValues.insert(STF_attributes);
	m_maskedValues.insert(STF_aura_interrupt_flags);
	m_maskedValues.insert(STF_cast_interrupt_flags);
	m_maskedValues.insert(STF_prevention_type);
	m_maskedValues.insert(STF_activated_by_in);
	m_maskedValues.insert(STF_activated_by_out);
	m_maskedValues.insert(STF_req_caster_mechanic); 
	m_maskedValues.insert(STF_req_tar_mechanic);

	m_kitValues.clear();
	m_kitValues.insert(SVF_traveling_kit);
	m_kitValues.insert(SVF_impact_kit);
	m_kitValues.insert(SVF_casting_kit);
	m_kitValues.insert(SVF_go_kit);
	m_kitValues.insert(SVF_aura_kit_ontop);
	m_kitValues.insert(SVF_aura_kit_below);

	m_fieldDescriptions.clear();
	m_fieldDescriptions[STF_mana_formula] = Util::readTextFile("scripts\\text\\STF_mana_formula.txt");
	m_fieldDescriptions[STF_description] = Util::readTextFile("scripts\\text\\STF_description.txt");
	m_fieldDescriptions[STF_effect1_scale_formula] = Util::readTextFile("scripts\\text\\STF_effect1_scale_formula.txt");
	m_fieldDescriptions[STF_effect2_scale_formula] = m_fieldDescriptions[STF_effect1_scale_formula];
	m_fieldDescriptions[STF_effect3_scale_formula] = m_fieldDescriptions[STF_effect1_scale_formula];
	m_fieldDescriptions[STF_duration_formula] = Util::readTextFile("scripts\\text\\STF_duration_formula.txt");

	m_longTxtValues.clear();
	m_longTxtValues.insert(STF_description);
	m_longTxtValues.insert(STF_effect1_scale_formula);
	m_longTxtValues.insert(STF_effect2_scale_formula);
	m_longTxtValues.insert(STF_effect3_scale_formula);

	m_fieldEnumOptions.clear();
	m_fieldEnumOptions[STF_aura_interrupt_flags] = &m_strInterruptCauses;
	m_fieldEnumOptions[STF_cast_interrupt_flags] = &m_strInterruptCauses;	
	m_fieldEnumOptions[STF_dispel] = &m_strDispelType;	
	m_fieldEnumOptions[STF_attributes] = &m_strAttributes;
	m_fieldEnumOptions[STF_activated_by_in] = &m_strHitResults;
	m_fieldEnumOptions[STF_activated_by_out] = &m_strHitResults;
	m_fieldEnumOptions[STF_abilities_tab] = &m_strAbilitiesTabs;
	m_fieldEnumOptions[STF_effect1] = &m_strEffects;
	m_fieldEnumOptions[STF_effect2] = &m_strEffects;
	m_fieldEnumOptions[STF_effect3] = &m_strEffects;
	m_fieldEnumOptions[STF_prevention_type] = &m_strMechanics;
	m_fieldEnumOptions[STF_cast_school] = &m_strMagicSchool;
	m_fieldEnumOptions[STF_magic_roll_school] = &m_strMagicSchool;
	m_fieldEnumOptions[STF_effect1_targetType] = &m_strTargetType;
	m_fieldEnumOptions[STF_effect2_targetType] = &m_strTargetType;
	m_fieldEnumOptions[STF_effect3_targetType] = &m_strTargetType;
	m_fieldEnumOptions[STF_entry] = &m_strEntries;
	m_fieldEnumOptions[STF_required_equipslot] = &m_strEquipSlots;
	m_fieldEnumOptions[SVF_unit_go_animation] = &m_strUnitAnims;
	m_fieldEnumOptions[SVF_unit_cast_animation] = &m_strUnitAnims;
	m_fieldEnumOptions[SVKF_spranim] = &m_strSpriteSheets;
	m_fieldEnumOptions[SVKF_spranim_2] = &m_strSpriteSheets;
	m_fieldEnumOptions[SVKF_sprblend] = &m_strBlendModes;
	m_fieldEnumOptions[SVKF_sprblend_2] = &m_strBlendModes;
	m_fieldEnumOptions[SVKF_ground_glow_color] = &m_strColors;
	m_fieldEnumOptions[SVKF_unit_glow_color] = &m_strColors;
	m_fieldEnumOptions[SVKF_psystem] = &m_strPsystems;
	m_fieldEnumOptions[STF_req_caster_mechanic] = &m_strMechanics;
	m_fieldEnumOptions[STF_req_tar_mechanic] = &m_strMechanics;
	m_fieldEnumOptions[STF_req_caster_aura] = &m_strEntries;
	m_fieldEnumOptions[STF_req_tar_aura] = &m_strEntries;
	m_fieldEnumOptions[STF_stat_scale_1] = &m_strStats;
	m_fieldEnumOptions[STF_stat_scale_2] = &m_strStats;
	m_fieldEnumOptions[STF_can_level_up] = &m_strBoolean;
	
	m_ignoreEnumForReadable.clear();
	m_ignoreEnumForReadable.insert(STF_entry);

	auto populate = [&](int start, int end, int xPos, int yPos, int width, int space)
	{
		for (int i = start, count = 0; i < end; ++i, ++count)
		{
			const int charactersize = 18;
			const int yVal = yPos + (count * 20);
			const auto field = int(i);

			auto text = make_shared<TextBoxRo>(*this, TableTemplateEditor::LabelsBegin + i, "Friz Quadrata Regular.ttf", 400, charactersize);
			text->setString(fieldName(field), getLabelColor(field));
			text->updateTopLeftCorner(sf::Vector2i(xPos, yVal));
			addRenderObject(text);

			auto promptBox = make_shared<PromptBox>(*this, TableTemplateEditor::PromptBoxBegin + i, "arial.ttf", nullptr, sf::Vector2i(xPos + space, yVal), width, sf::Color::White);
			promptBox->setPromptCharacterSize(charactersize);
			promptBox->setMaxPromptString(1024);
			addRenderObject(promptBox);
		}
	};
	
	populate(0, Field::EndFields_Main, 100, 35, 400, 190);
	populate(Field::STF_effect1, Field::EndFields_Effect1, 700, 40, 200, 190);
	populate(Field::STF_effect2, Field::EndFields_Effect2, 700, 240, 200, 190);
	populate(Field::STF_effect3, Field::EndFields_Effect3, 1100, 40, 200, 190);
	populate(Field::SVF_traveling_kit, Field::EndSpellVisualFields, 100, 800, 90, 190);
	populate(Field::SVKF_entry, Field::EndSpellVisualKitFields, 485, 500, 400, 220);

	for (auto& conditionalLabel : m_conditionalLabels)
	{
		bool met = true;

		// Check if the conditions are met
		for (auto& condition : conditionalLabel.condition)
		{
			if (atoi(fieldValue(condition.first).c_str()) != condition.second)
			{
				met = false;
				break;
			}
		}
		
		if (met)
		{
			for (auto& newData : conditionalLabel.newData)
			{			
				auto label = dynamic_pointer_cast<TextBoxRo>(getRenderObject(TableTemplateEditor::LabelsBegin + newData.field));
			
				// Apply new label
				label->setString(newData.value, getLabelColor(newData.field));
				m_fieldEnumOptions[newData.field] = newData.dictionary;

				if (newData.mask)
					m_maskedValues.insert(newData.field);
			}
		}
	}
	
	for (int i = 0; i < Field::NumFields; ++i)
	{
		auto promptBox = dynamic_pointer_cast<PromptBox>(getRenderObject(TableTemplateEditor::PromptBoxBegin + i));
		auto label = dynamic_pointer_cast<TextBoxRo>(getRenderObject(TableTemplateEditor::LabelsBegin + i));

		if (promptBox == nullptr || label == nullptr)
			continue;

		if (!fieldVisible(int(i)))
		{
			promptBox->setHidden(true);
			label->setHidden(true);
		}

		auto value = fieldValue(int(i));
		auto content = fieldValueToHumandReadable(int(i), value);
		promptBox->setContent(content.empty() ? value : content);
	}

	auto icon = make_shared<SpellIcon>(*this, Interface::SpellEntryIcon, "gameicon40");
	icon->getTopLeftCornerRef() = { 35, 44 };
	icon->changeEntry(m_entry);
	addRenderObject(icon);
		
	m_strEntries.clear();

	auto& sv = sContentMgr->db("spell_template");
	
	for (auto& itr : sv.fetchIndexData())
	{
		string entry = itr.first.c_str();		
		auto nameitr = itr.second.find("name");

		if (nameitr != itr.second.end())
		{
			string name = nameitr->second.getString();
			m_strEntries[entry] = Util::fmtStr("%s - %s", entry.c_str(), name.c_str());
		}
	}
	
	if (sContentMgr->db("spell_visual").data(m_entry, "entry").empty())
	{
		// Must be a matching spell_visual entry (todo: merge these tables probably, was mistake to seperate this data)
		shared_ptr<SqlConnector> db = SqlConnector::create(SqlType::SQLite, sConfig->getString("GameDb", "Name"), sConfig->getString("GameDb", "Path"));
		db->query(Util::fmtStr("INSERT INTO `spell_visual` ('entry') VALUES ('%d');", m_entry));
	}
}

bool SpellTemplateEditor::fieldVisible(const int field) const
{
	if (field < Field::EndSpellVisualFields)
		return true;
	
	if (field < Field::EndSpellVisualKitFields)
		return m_kitEntry != 0;

	return __super::fieldVisible(field);
}

string SpellTemplateEditor::fieldDbName(const int field) const
{
	switch (field)
	{
		case STF_entry:					return "entry";
		case STF_name:					return "name";
		case STF_icon:					return "icon";
		case STF_description:			return "description";
		case STF_aura_description:		return "aura_description";
		case STF_mana_formula:			return "mana_formula";
		case STF_mana_pct:				return "mana_pct";
		case STF_health_cost:			return "health_cost";
		case STF_health_pct_cost:		return "health_pct_cost";
		case STF_maxTargets:			return "maxTargets";
		case STF_dispel:				return "dispel";
		case STF_abilities_tab:			return "abilities_tab";
		case STF_can_level_up:			return "can_level_up";
		case STF_attributes:			return "attributes";
		case STF_activated_by_in:		return "activated_by_in";
		case STF_cast_time:				return "cast_time";
		case STF_cooldown:				return "cooldown";
		case STF_cooldown_category:		return "cooldown_category";
		case STF_aura_interrupt_flags:	return "aura_interrupt_flags";
		case STF_cast_interrupt_flags:	return "cast_interrupt_flags";
		case STF_duration:				return "duration";
		case STF_duration_formula:		return "duration_formula";
		case STF_interval:				return "interval";
		case STF_speed:					return "speed";
		case STF_prevention_type:		return "prevention_type";
		case STF_cast_school:			return "cast_school";
		case STF_magic_roll_school:		return "magic_roll_school";
		case STF_range:					return "range";
		case STF_range_min:				return "range_min";
		case STF_stack_amount:			return "stack_amount";
		case STF_required_equipslot:	return "required_equipment";
		case STF_req_caster_mechanic:	return "req_caster_mechanic";
		case STF_req_tar_mechanic:		return "req_tar_mechanic";
		case STF_req_caster_aura:		return "req_caster_aura";
		case STF_req_tar_aura:			return "req_tar_aura";
		case STF_stat_scale_1:			return "stat_scale_1";
		case STF_stat_scale_2:			return "stat_scale_2";

		case STF_effect1:				return "effect1";
		case STF_effect1_data1:			return "effect1_data1";
		case STF_effect1_data2:			return "effect1_data2";
		case STF_effect1_data3:			return "effect1_data3";
		case STF_effect1_targetType:	return "effect1_targetType";
		case STF_effect1_radius:		return "effect1_radius";
		case STF_effect1_positive:		return "effect1_positive";
		case STF_effect1_scale_formula:	return "effect1_scale_formula";
		
		case STF_effect2:				return "effect2";
		case STF_effect2_data1:			return "effect2_data1";
		case STF_effect2_data2:			return "effect2_data2";
		case STF_effect2_data3:			return "effect2_data3";
		case STF_effect2_targetType:	return "effect2_targetType";
		case STF_effect2_radius:		return "effect2_radius";
		case STF_effect2_positive:		return "effect2_positive";
		case STF_effect2_scale_formula:	return "effect2_scale_formula";
		
		case STF_effect3:				return "effect3";
		case STF_effect3_data1:			return "effect3_data1";
		case STF_effect3_data2:			return "effect3_data2";
		case STF_effect3_data3:			return "effect3_data3";
		case STF_effect3_targetType:	return "effect3_targetType";
		case STF_effect3_radius:		return "effect3_radius";
		case STF_effect3_positive:		return "effect3_positive";
		case STF_effect3_scale_formula:	return "effect3_scale_formula";
			
		case SVF_traveling_kit:			return "traveling_kit";
		case SVF_impact_kit:			return "impact_kit";
		case SVF_casting_kit:			return "casting_kit";
		case SVF_go_kit:				return "go_kit";
		case SVF_aura_kit_ontop:		return "aura_kit_ontop";
		case SVF_aura_kit_below:		return "aura_kit_below";
		case SVF_unit_go_animation:		return "unit_go_animation";
		case SVF_unit_cast_animation:	return "unit_cast_animation";			
		case SVF_uca_speed:				return "uca_speed";
		case SVF_uca_freeze_frame:		return "uca_freeze_frame";

		case SVKF_entry:				return "id";
		case SVKF_psystem:				return "psystem";
		case SVKF_psystem_x:			return "psystem_x";
		case SVKF_psystem_y:			return "psystem_y";
		case SVKF_spranim:				return "spranim";
		case SVKF_spranim_x:			return "spranim_x";
		case SVKF_spranim_y:			return "spranim_y";
		case SVKF_sprcolor:				return "sprcolor";
		case SVKF_sprblend:				return "spranim_blend";
		case SVKF_spranim_topmost:		return "spranim_topmost";
		case SVKF_spranim2_topmost:		return "spranim2_topmost";
		case SVKF_spranim_2:			return "spranim_2";
		case SVKF_spranim_x_2:			return "spranim_x_2";
		case SVKF_spranim_y_2:			return "spranim_y_2";
		case SVKF_sprcolor_2:			return "sprcolor_2";
		case SVKF_sprblend_2:			return "spranim2_blend";
		case SVKF_sound:				return "sound";
		case SVKF_directional_spranim:	return "directional_spranim";
		case SVKF_directional_spranim_x:return "directional_spranim_x";
		case SVKF_directional_spranim_y:return "directional_spranim_y";
		case SVKF_ground_glow_color:	return "ground_glow_color";
		case SVKF_unit_glow_color:		return "unit_glow_color";
	}

	return "unk";
}

int SpellTemplateEditor::fieldEntry(const int field) const
{	
	if (field < EndSpellVisualFields)
		return m_entry;
	
	if (field < EndSpellVisualKitFields)
		return m_kitEntry;

	return -1;
}

string SpellTemplateEditor::fieldKeyName(const int field) const
{
	if (field < EndFields_Effect3)
		return fieldDbName(STF_entry);
	
	if (field < EndSpellVisualFields)
		return fieldDbName(STF_entry);
	
	if (field < EndSpellVisualKitFields)
		return fieldDbName(SVKF_entry);

	return "unk";
}

string SpellTemplateEditor::fieldTable(const int field) const
{
	if (field < EndFields_Effect3)
		return "spell_template";
	
	if (field < EndSpellVisualFields)
		return "spell_visual";
	
	if (field < EndSpellVisualKitFields)
		return "spell_visual_kit";

	return "unk";
}

string SpellTemplateEditor::fieldName(const int field) const
{
	switch (field)
	{
		case STF_entry:					return "Entry";
		case STF_name:					return "Name";
		case STF_icon:					return "Icon";
		case STF_description:			return "Description";
		case STF_aura_description:		return "Aura Description";
		case STF_mana_formula:			return "Mana Formula";
		case STF_mana_pct:				return "Mana% Cost";
		case STF_maxTargets:			return "Max Targets";
		case STF_dispel:				return "Dispel Type";
		case STF_abilities_tab:			return "Abilities Tab";
		case STF_can_level_up:			return "Can Level Up";
		case STF_attributes:			return "Attributes";
		case STF_cast_time:				return "Cast Time";
		case STF_cooldown:				return "Cooldown";
		case STF_cooldown_category:		return "Cooldown Category";
		case STF_aura_interrupt_flags:	return "Aura InterruptFlags";
		case STF_cast_interrupt_flags:	return "Cast InterruptFlags";
		case STF_duration:				return "Duration";
		case STF_duration_formula:		return "Duration Formula";
		case STF_interval:				return "Interval";
		case STF_speed:					return "Speed";
		case STF_prevention_type:		return "Prevented By";
		case STF_cast_school:			return "Cast School";
		case STF_magic_roll_school:		return "Magic Roll School";
		case STF_range:					return "Range";
		case STF_range_min:				return "Range Min";
		case STF_stack_amount:			return "Stack Limit";
		case STF_required_equipslot:	return "Required Equipment";
		case STF_health_cost:			return "Health Cost";
		case STF_health_pct_cost:		return "Health% Cost";
		case STF_activated_by_in:		return "Activated By (In)";
		case STF_activated_by_out:		return "Activated By (Out)";
		case STF_req_caster_mechanic:	return "Req Caster Mechanic";
		case STF_req_tar_mechanic:		return "Req Target Mechanic";
		case STF_req_caster_aura:		return "Req Caster Aura";
		case STF_req_tar_aura:			return "Req Target Aura";
		case STF_stat_scale_1:			return "Stat Scale #1";
		case STF_stat_scale_2:			return "Stat Scale #2";

		case STF_effect1:				return "Effect 1";
		case STF_effect1_data1:			return "Data1";
		case STF_effect1_data2:			return "Data2";
		case STF_effect1_data3:			return "Data3";
		case STF_effect1_targetType:	return "Target Type";
		case STF_effect1_radius:		return "Radius";
		case STF_effect1_positive:		return "Positivity";	
		case STF_effect1_scale_formula: return "Scale Formula";
		
		case STF_effect2:				return "Effect 2";
		case STF_effect2_data1:			return "Data1";
		case STF_effect2_data2:			return "Data2";
		case STF_effect2_data3:			return "Data3";
		case STF_effect2_targetType:	return "Target Type";
		case STF_effect2_radius:		return "Radius";
		case STF_effect2_positive:		return "Positivity";	
		case STF_effect2_scale_formula: return "Scale Formula";
		
		case STF_effect3:				return "Effect 3";
		case STF_effect3_data1:			return "Data1";
		case STF_effect3_data2:			return "Data2";
		case STF_effect3_data3:			return "Data3";
		case STF_effect3_targetType:	return "Target Type";
		case STF_effect3_radius:		return "Radius";
		case STF_effect3_positive:		return "Positivity";			
		case STF_effect3_scale_formula: return "Scale Formula";

		case SVF_traveling_kit:			return "Traveling Kit";
		case SVF_impact_kit:			return "Impact Kit";
		case SVF_casting_kit:			return "Casting Kit";
		case SVF_go_kit:				return "Go Kit";
		case SVF_aura_kit_ontop:		return "Aura Kit (Top)";
		case SVF_aura_kit_below:		return "Aura Kit (Under)";
		case SVF_unit_go_animation:		return "Unit GO Anim";
		case SVF_unit_cast_animation:	return "Unit Cast Anim";
		case SVF_uca_speed:				return "CastAnim Speed";
		case SVF_uca_freeze_frame:		return "CastAnim FreezeFr";
					
		case SVKF_entry:				return "Selected Kit";
		case SVKF_psystem:				return "Partical System";
		case SVKF_psystem_x:			return "Partical System X";
		case SVKF_psystem_y:			return "Partical System Y";
		case SVKF_spranim:				return "Sprite Animation";
		case SVKF_spranim_x:			return "Sprite Animation X";
		case SVKF_spranim_y:			return "Sprite Animation Y";
		case SVKF_sprblend:				return "Sprite BlendMode";
		case SVKF_sprcolor:				return "Sprite (2) Color";
		case SVKF_spranim_2:			return "Sprite (2) Animation";
		case SVKF_spranim_x_2:			return "Sprite (2) Animation X";
		case SVKF_spranim_y_2:			return "Sprite (2) Animation Y";
		case SVKF_sprcolor_2:			return "Sprite (2) Color";
		case SVKF_sprblend_2:			return "Sprite (2) BlendMode";
		case SVKF_sound:				return "Sound";
		case SVKF_directional_spranim:	return "Directional SprAnim";
		case SVKF_directional_spranim_x:return "Directional SprAnim X";
		case SVKF_directional_spranim_y:return "Directional SprAnim Y";
		case SVKF_ground_glow_color:	return "Ground Glow Color";
		case SVKF_unit_glow_color:		return "Unit Glow Color";
		case SVKF_spranim_topmost:		return "Always On Top (1)";
		case SVKF_spranim2_topmost:		return "Always On Top (2)";
	}

	return "unk";
}

string SpellTemplateEditor::huamnReadableToFieldValue(const int field, const string& readable) const
{
	if (readable == "null")
		return "";

	switch (field)
	{
		case STF_cast_time:
		case STF_cooldown:
		case STF_range:
		{
			string s = readable;
			s.erase(std::remove_if(s.begin(), s.end(), isalpha), s.end());
			return readable;
		}
	}
	
	return __super::huamnReadableToFieldValue(field, readable);
}

string SpellTemplateEditor::fieldValueToHumandReadable(const int field, string value) const
{	
	if (value.empty())
		value = "0";

	switch (field)
	{
		case STF_cast_time:
		case STF_cooldown:
		case STF_duration:
		case STF_interval:
		{
			return value + " ms";
		}
		case STF_range:
		case STF_range_min:
		{
			return value + " pixels";
		}
		case STF_effect1_radius:
		case STF_effect2_radius:
		case STF_effect3_radius:
		{
			return value + " cells";
		}
		case STF_speed:
		{
			return value + " pixel/sec.";
		}
	}
	
	return __super::fieldValueToHumandReadable(field, value);
}

SpellDefines::AuraType SpellTemplateEditor::getAuraTypeFromName(const string& str) const
{	
	for (auto& itr : m_strAuraType)
	{
		if (itr.second == str)
			return SpellDefines::AuraType(atoi(itr.first.c_str()));
	}

	return SpellDefines::AuraType(-1);
}

SpellDefines::Effects SpellTemplateEditor::getSpellEffectFromName(const string& str) const
{
	for (auto& itr : m_strEffects)
	{
		if (itr.second == str)
			return SpellDefines::Effects(atoi(itr.first.c_str()));
	}

	return SpellDefines::Effects(-1);
}