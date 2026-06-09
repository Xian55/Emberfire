#pragma once

#include "Button.h"

#include "..\Shared\ItemDefines.h"
#include "..\Shared\SpellDefines.h"

class Tooltip;
class Text;
class World;
class ClientUnit;

class GameIcon : public Button, public enable_shared_from_this<GameIcon>
{
	public:
		enum Type
		{
			Item,
			Spell,
		};

	public:
		virtual ~GameIcon();
		
		void popTooltipRefresh() final;

		void setRenderDispelType(const bool b);
		void setTooltipOffset(const sf::Vector2i& pos) { m_tooltipOffset = pos; }
		void release(World& source);
		void setDarken(const bool v) { m_darken = v; }
		void setCooldown(const __time64_t startDate, const __time64_t endDate);
		void setExpirationTimer(const __time64_t startDate, const __time64_t endDate);
		void setBindColor(const sf::Color col) { m_bindColor = col; }
		void setTooltipHorizontalAdju(const bool v) { m_allowHorizontalTooltipAdjust = v; }
		void setAlwaysLeftSide(const bool v) { m_alwaysLeftSide = v; }
		void setStackCount(const int stack) { m_stackAmount = stack; }
		void setOom(const bool oom) { m_oom = oom; }
		void refreshTooltip();
		void updateTooltipPosition();
		void setRenderStackAtBottom(const bool v) { m_stackRenderBottom = v; }
		void setShowStackAmtThreshold(const int greaterThanThisValue) { m_minStackShowAmount = greaterThanThisValue; }
		void setHideIfNull(const bool v) { m_hideIfNull = v; }

		// Returns true if there was an entry change
		bool changeEntry(const int entry);

		// Returns false if there was a problem
		bool grab(World& giveTo);

		// Returns true if there was a change
		bool setBasePoints(const int effIdx, const int minp, const int maxp);
		bool setBasePoints(const vector<pair<int16_t, int16_t>>& vec);
		bool setLevel(const int level);

		// Build (and return) this icon's tooltip from its current entry/level/bpoints. Reuses each subclass's
		// virtual fillTooltip (SpellIcon: name/rank/mana/range/cast/cooldown/desc; ItemIcon: item card), so the
		// Lua action-bar view gets the exact same tooltip the C++ icon would draw. nullptr if empty.
		shared_ptr<Tooltip> rebuildTooltip() { if (m_entry == 0) return nullptr; fillTooltip(); return getTooltip(); }

		auto getEntry() const { return m_entry; }
		auto getType() const { return m_type; }
		auto getQuality() const { return m_quality; }
		auto getStackCount() const { return m_stackAmount; }
		bool isErrorIcon() const { return m_red; }   // red "can't use" overlay state (level/class/req not met)

		virtual int getSpellManaCost(ClientUnit& myself) const { return 0; }
		virtual int getSpellRange() const { return m_cachedSpellRange; }
		virtual int getSpellRangeMin() const { return m_cachedSpellRangeMin; }
		virtual int getCastedSpellId() const { return getEntry(); }
		virtual int getCastedSpellCategoryCooldown() const { return 0; }

		const auto& getEffBasePts() const { return m_effectPoints; }

		virtual string deduceDescription() const { return ""; }
		virtual string deduceTitle() const { return ""; }

	protected:
		GameIcon(RenderObject& owner, const int id, const string& frame, const int entry = 0);

		void input() final;
		void render() final;

		void cacheCasterLevel();
		void drawCooldown();
		void drawCooldownText();
		void drawCooldownFlash();
		void drawClock(const float degree);
		void drawCountdown();
		void parseFormula(const string& baseValue, string& inputOutput) const;	
		
		virtual void drawLabels();
		virtual void fillTooltip() {}
		virtual void pickIcon() {}
		virtual void onEntryChange() {}

		World* getWorld() const;

		string formSpellDescription(const int spellId) const;
		string formTimeString(int timeMs, const vector<string>& dictionary, const bool allowFloat = true) const;
		string formTimeSuffix(int timeMs, const vector<string>& dictionary) const;
		string formTimePrefix(int timeMs) const;
		string tooltipFont() const { return Assets::fontFile(Assets::FontId::Trebuchet); }

		bool m_darken{0};
		bool m_grabbed{0};
		bool m_renderDispelType{0};
		bool m_allowHorizontalTooltipAdjust{true};
		bool m_alwaysLeftSide{false};
		bool m_needTooltipRefresh{false};
		bool m_oom{false};
		bool m_red{false};
		bool m_stackRenderBottom{false};
		bool m_hideIfNull{false};

		int m_entry{0};
		int m_cachedSpellRange{0};
		int m_cachedSpellRangeMin{0};
		int m_cachedActivatedByOut{0};
		int m_cachedActivatedByIn{0};
		int m_spellLevel{1};
		int m_stackAmount{0};
		int m_casterLevel{0};
		int m_minStackShowAmount{1};

		vector<pair<int16_t, int16_t>> m_effectPoints;

		float m_cooldownFlash{0};
		float m_redrawTooltipTimer{0};

		Type m_type{Type::Spell};
		ItemDefines::Quality m_quality{ItemDefines::Quality::QualityLv1};

		__time64_t m_cooldownExpireDate{0};
		__time64_t m_cooldownStartDate{0};

		__time64_t m_durationExpireDate{0};
		__time64_t m_durationStartDate{0};
		
		unique_ptr<Text> m_cooldownTxt;
		unique_ptr<Text> m_bindLabel;
		unique_ptr<Text> m_stackLabel;
		shared_ptr<Sprite> m_icon;
		sf::Vector2i m_tooltipOffset;

		sf::Color m_renderDispelTypeColor;
		sf::Color m_bindColor{sf::Color::White};
};