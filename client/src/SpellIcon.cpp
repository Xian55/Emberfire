#include "stdafx.h"
#include "SpellIcon.h"
#include "ContentMgr.h"
#include "Sprite.h"
#include "Tooltip.h"
#include "Application.h"
#include "Text.h"
#include "Keybinds.h"
#include "ClientPlayer.h"
#include "Equipment.h"
#include "TextBox.h"

#include "..\SqlConnector\QueryResult.h"
#include "..\Shared\PlayerDefines.h"
#include "..\StringHelpers.h"

#include <assert.h>

SpellIcon::SpellIcon(RenderObject& owner, const int id, const string& frame, const int entry /*= 0*/) :
	GameIcon(owner, id, frame, entry)
{
	m_type = GameIcon::Type::Spell;
	m_needTooltipRefresh = true;
	pickIcon();
}

void SpellIcon::drawLabels() /*final*/
{
	__super::drawLabels();
}
		
void SpellIcon::pickIcon() /*final*/
{
	if (getEntry() == 0)
	{
		m_icon = nullptr;
		return;
	}

	auto& st = sContentMgr->db("spell_template");
	ASSERT(m_icon = sContentMgr->spawnSprite(st.data(getEntry(), "icon")));

	m_castedSpellCategoryCooldown = atoi(st.data(getEntry(), "cooldown_category").c_str());
	m_cachedSpellRange = atoi(st.data(getEntry(), "range").c_str());		
	m_cachedSpellRangeMin = atoi(st.data(getEntry(), "range_min").c_str());		
	m_cachedActivatedByOut = atoi(st.data(getEntry(), "activated_by_out").c_str());
	m_cachedActivatedByIn = atoi(st.data(getEntry(), "activated_by_in").c_str());

	m_manaCost = 0;
	m_manaCostPct = 0;
	
	// The icon can fade out to blue when the player is out of mana, so we need to cache the mana cost when the icon is picked
	string manaFormula = st.data(getEntry(), "mana_formula").c_str();

	if (!manaFormula.empty())
	{
		parseFormula("", manaFormula);
		m_manaCost = atoi(manaFormula.c_str());
	}
	else
	{
		m_manaCostPct = atoi(st.data(getEntry(), "mana_pct").c_str());
	}
}
		
int SpellIcon::getSpellManaCost(ClientUnit& myself) const /*final*/
{ 
	if (m_manaCost > 0)
		return m_manaCost;

	if (m_manaCostPct > 0)
		return int(float(myself.getMaxMana()) * (float(m_manaCostPct) / 100.f));

	return 0;
}

/*static*/
bool SpellIcon::canSpellScale(const int entry) 
{
	auto& st = sContentMgr->db("spell_template");
	return atoi(st.data(entry, "can_level_up").c_str()) != 0;
}

void SpellIcon::fillTooltip() /*final*/
{
	if (getEntry() == 0)
	{
		setTooltip(nullptr);
		return;
	}

	auto tooltip = make_shared<Tooltip>(*this, sf::Vector2i());

	auto& it = sContentMgr->db("item_template");
	auto& st = sContentMgr->db("spell_template");

	/**
	* Spell Name
	* 601 Mana, 2.5 sec cast, 35 ft range
	* Description goes here and tells you stuff about the
	*  spell in question like what it does so forth.
	*/

	if (m_durationExpireDate == 0)
	{
		string title = st.data(getEntry(), "name");

		// If it can't scale then just hide the level req
		if (canSpellScale(getEntry()))
		{
			string statScale;
			
			int stat1 = atoi(st.data(getEntry(), "stat_scale_1").c_str());
			int stat2 = atoi(st.data(getEntry(), "stat_scale_2").c_str());
			
			if (stat1 && !stat2)
				statScale = Util::fmtStr(" (%s)", UnitFunctions::statAbbr(UnitDefines::Stat(stat1)).c_str());
			else if (stat2 && !stat1)
				statScale = Util::fmtStr(" (%s)", UnitFunctions::statAbbr(UnitDefines::Stat(stat2)).c_str());
			else if (stat1 && stat2)
				statScale = Util::fmtStr(" (%s/%s)", UnitFunctions::statAbbr(UnitDefines::Stat(stat1)).c_str(), UnitFunctions::statAbbr(UnitDefines::Stat(stat2)).c_str());

			title += TextBox::getStringKey(TextBox::Key_ReplaceSpaceToMaxWidth);
			
			if (m_spellLevel <= 1)
				title += "Basic";
			else
				title += Util::fmtStr("Lv. +%d", m_spellLevel - 1);

			title += statScale;
		}

		tooltip->addLine(tooltipFont(), 15, title);
		
		string manaFormula = st.data(getEntry(), "mana_formula").c_str();		
		const int manaPct = atoi(st.data(getEntry(), "mana_pct").c_str());
		const int castTime = atoi(st.data(getEntry(), "cast_time").c_str());
		const int cooldownMs = atoi(st.data(getEntry(), "cooldown").c_str());

		string manaStr;
		string rangeStr;
		string castTimeStr;
		string cooldownStr;

		// Mana 
		if (!manaFormula.empty())
		{
			parseFormula("", manaFormula);
			manaStr = Util::fmtStr("%s Mana", manaFormula.c_str());
		}
		else
		{
			// Mana Pct
			if (manaPct > 0)
				manaStr += to_string(manaPct) + "% Mana ";
		}

		// Range
		if (m_cachedSpellRange > 0)
		{
			if (m_cachedSpellRangeMin == 0)
				rangeStr = to_string(m_cachedSpellRange / 20) + " yd range";
			else
				rangeStr = to_string(m_cachedSpellRangeMin / 20) + "-" +  to_string(m_cachedSpellRange / 20) + " yd range";
		}
		else if (m_cachedSpellRangeMin > 0)
		{			
			rangeStr = to_string(m_cachedSpellRangeMin / 20) + " yd deadzone";
		}


		// Cast time
		if (castTime > 0)
			castTimeStr = Util::fmtStr("%.2f sec cast", static_cast<float>(castTime) / 1000.0f);
		else
			castTimeStr = "Instant";

		// Cooldown
		if (cooldownMs > 0)
			cooldownStr = formTimeString(cooldownMs, { "day cooldown", "hour cooldown", "minute cooldown", "sec cooldown" });

		vector<string> lineStrs;

		if (!manaStr.empty())
			lineStrs.push_back(manaStr);

		if (!rangeStr.empty())
			lineStrs.push_back(rangeStr);

		if (!castTimeStr.empty())
			lineStrs.push_back(castTimeStr);

		if (!cooldownStr.empty())
		{
			lineStrs.push_back(cooldownStr);

			// Keep cooldown on the right side when possible
			if (lineStrs.size() == 3)
			{
				lineStrs.push_back("");
				swap(lineStrs[3], lineStrs[1]);
				swap(lineStrs[3], lineStrs[2]);
			}
		}

		// Empty string signals end of list but as a lazy safety net for index checks make sure this is filled
		while (lineStrs.size() < 4)
			lineStrs.push_back(string{});

		if (!lineStrs[0].empty())
			tooltip->addLine(tooltipFont(), 15, Util::fmtStr("%s%s%s", lineStrs[0].c_str(), TextBox::getStringKey(TextBox::Key_ReplaceSpaceToMaxWidth).c_str(), lineStrs[1].c_str()));

		if (!lineStrs[2].empty())
			tooltip->addLine(tooltipFont(), 15, Util::fmtStr("%s%s%s", lineStrs[2].c_str(), TextBox::getStringKey(TextBox::Key_ReplaceSpaceToMaxWidth).c_str(), lineStrs[3].c_str()));
				
		// Requirements
		int req_caster_mechanic = atoi(st.data(getEntry(), "req_caster_mechanic").c_str());

		if (req_caster_mechanic != 0)
		{
			if (Util::maskHas(req_caster_mechanic, SpellDefines::Mechanics::Stealth))
				tooltip->addLine(tooltipFont(), 15, "Requires Stealth");
		}

		// Description last
		string descriptionStr = formSpellDescription(getEntry());

		if (!descriptionStr.empty())
			tooltip->addLine(tooltipFont(), 15, descriptionStr, sf::Color(240, 197, 2, 255));
	}

	/**
	* Spell Name		DispelType
	* Description goes here and tells you stuff about the
	*  spell in question like what it does so forth.
	* X seconds remaining
	*/

	else 
	{
		// Title
		const int dispelType = atoi(st.data(getEntry(), "dispel").c_str());
		const string title = Util::fmtStr("%s%s%s", st.data(getEntry(), "name").c_str(), TextBox::getStringKey(TextBox::Key_ReplaceSpaceToMaxWidth).c_str(), 
			SpellIcon::dispelTypeStr(static_cast<SpellDefines::DispelType>(dispelType)).c_str());
		tooltip->addLine(tooltipFont(), 15, title, sf::Color(240, 197, 2, 255));

		// Description
		const string descriptionStr = st.data(getEntry(), "aura_description");

		if (!descriptionStr.empty())
			tooltip->addLine(tooltipFont(), 15, descriptionStr);

		// Time remaining
		__time64_t timeRemainingMs = m_durationExpireDate - sApplication->timeNowMs();
		string timeRemainingStr = formTimeString(int(timeRemainingMs), { "days remaining", "hours remaining", "minutes remaining", "seconds remaining" }, false);
		tooltip->addLine(tooltipFont(), 15, timeRemainingStr, sf::Color(240, 197, 2, 255));
	}

	setTooltip(tooltip);
}

string SpellIcon::deduceDescription() const
{
	return formSpellDescription(getEntry());
}

string SpellIcon::deduceTitle() const
{
	if (m_type == Type::Spell)
	{
		auto& sv = sContentMgr->db("spell_template");
		string name = sv.data(getEntry(), "name");

		if (m_spellLevel > 1)
			name += " " + Util::toRoman(m_spellLevel - 1);

		return name;
	}

	return "";
}

string SpellIcon::dispelTypeStr(const SpellDefines::DispelType type)
{
	switch (type)
	{
		case SpellDefines::DispelType::Physical: return "Physical";
		case SpellDefines::DispelType::Curse: return "Curse";
		case SpellDefines::DispelType::Disease: return "Disease";
		case SpellDefines::DispelType::Poison: return "Poison";
		case SpellDefines::DispelType::Magic: return "Magic";

	}

	return "Unkown";
}
