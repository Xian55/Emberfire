#pragma once

#include "WorldChild.h"

class SpriteRo;
class Inventory;
class Abilities;
class SpriteAnimation;

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

		map<int, pair<clock_t, vector<shared_ptr<SpriteAnimation>>>> m_spellPreloadCache;
		map<Interface, shared_ptr<GameIcon>> m_icons;
};

