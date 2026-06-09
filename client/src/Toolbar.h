#pragma once

#include "WorldChild.h"

class SpriteRo;
class Inventory;
class Abilities;
class SpriteAnimation;
class Tooltip;

class Toolbar :	public WorldChild
{
	public:
		enum Interface
		{
			GameIcon1 = 1,
			GameIcon2,
			GameIcon3,
			GameIcon4,
			GameIcon5,
			GameIcon6,
			GameIcon7,
			GameIcon8,
			GameIcon9,
			GameIcon10,
			GameIcon11,
			GameIcon12,
		};

	public:
		Toolbar(World& owner, const int id, const sf::Vector2i topLeftCorner = sf::Vector2i{});
		virtual ~Toolbar();

		void setButtonBind(const Interface id, const SfKeyEvent ke);
		void createBaseIcon(const Interface id, const int type, const int entry);
		void refreshTooltips();
		void saveCache();

		bool loadCache(const string& playerName);

		int getIconEntry(const Interface id) const;

		shared_ptr<GameIcon> findIconByEntry(const int entry) const;

		const string& getPlayerName() const { return m_playerName; }

		// ---- Lua action-bar view: when set, render() draws nothing and input() only detects keybinds (the Lua
		//      ActionBar owns the visuals, clicks and drag; the C++ bar stays the data + cast + cooldown + cache
		//      engine). The 3 bars are flipped on via World::setActionBarsLuaView when the Lua addon loads. ----
		void setLuaView(const bool v) { m_luaView = v; }

		// Activate a slot (the same cast/use the keybind + click path runs). Public so the Lua view can drive it.
		void useIcon(const Interface id);
		// Assign a slot's icon (type 0=Item 1=Spell, entry 0 = clear), preserving its keybind, then save the cache.
		void assignIcon(const Interface id, const int type, const int entry);
		void clearIcon(const Interface id) { assignIcon(id, 1 /*GameIcon::Type::Spell*/, 0); }

		// ---- read accessors for the Lua view (per icon) ----
		bool   iconInfo(const Interface id, int& type, int& entry, string& texture) const;
		bool   iconCooldown(const Interface id, int& remainingMs, int& durationMs) const;
		int    iconStateFlags(const Interface id) const;   // bit0 has-entry, bit1 darken, bit2 out-of-range, bit3 oom
		int    iconStackCount(const Interface id) const;
		string iconKeybindText(const Interface id) const;
		shared_ptr<Tooltip> iconTooltip(const Interface id) const;

	private:
		void input() final;
		void render() final;
		void givenGabbedIcon(shared_ptr<GameIcon> myIcon, shared_ptr<GameIcon> heldIcon) final;

		void castSpell(int spellId);
		void updateBottomRight();
		void updateInvSpbookCheck(GameIcon& icon, Inventory& inv, Abilities& spells);

		bool shouldDarkenSpellId(const int spellId) const;
		bool expiredPreload(const pair<clock_t, vector<shared_ptr<SpriteAnimation>>>& dat) const;

		string getCacheFilename() const;

		string m_playerName;

		bool m_luaView{false};

		map<int, pair<clock_t, vector<shared_ptr<SpriteAnimation>>>> m_spellPreloadCache;
		map<Interface, shared_ptr<GameIcon>> m_icons;
};

