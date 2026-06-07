#pragma once

#include "WorldChild.h"
#include "GameIcon.h"

class Button;
class GameIcon;
class Text;
class TextBox;
class Tooltip;

class GameIconList : public WorldChild
{
	public:
		GameIconList(World& world, RenderObjectHolder& owner, const int id, const GameIcon::Type type, const int numVerticalElements, const int graphicalWidth,
			const sf::Vector2i& iconOffsetWithinSlot, const string& slotBg, const int slotHeight, const string& titleFont, const int titleFontSize, 
			const sf::Color& titleColor, const sf::Color& titleOutlineColor, const float titleBorderThickness, const sf::Vector2i& titleOffset);
		virtual ~GameIconList() {}

		void setAllowDraggingIcons(const bool v);
		void setLowercaseText(const bool v) { m_lowercaseTxt = v; }
		void addItemIcon(const ItemDefines::ItemDefinition entry, const int stack);
		void addSpellIcon(const int entry, const int lvl, const vector<pair<int16_t, int16_t>>& bpoints = {});
		void updateSpellIcon(const int entry, const int lvl, const vector<pair<int16_t, int16_t>>& bpoints = {});
		void scroll(const int amount);
		void setScroll(const int val);
		void hideIndex(const int idx);
		void clearEntries();
		void refreshTooltips();
		void registerDongle(const string& buttonName);
		void registerDongleTooltip(const string& buttonName, const int iconEntry, shared_ptr<Tooltip> tooltip);
		void setDongleOffset(const string& buttonName, const sf::Vector2i& off) { m_dongleOffset[buttonName] = off; }
		void pushDongleException(const string& buttonName, const int iconEntry) { m_dongleExceptions[buttonName].insert(iconEntry); }
		void clearDongleExceptions() { m_dongleExceptions.clear(); }
		void clearDarkenedEntries() { m_darkenedEntries.clear(); }
		void registerDarkenedEntry(const int entry) { m_darkenedEntries.insert(entry); }

		void enableDescriptions(const string& descriptionFont,
			const int descriptionFontSize, const sf::Color& descriptionColor, const sf::Color& descriptionOutlineColor, const float descriptionBorderThickness,
			const sf::Vector2i& descriptionOffset, const int descriptionWidth, const int descriptionMaxLines);

		bool isAllHidden() const;
		bool upMaxxed() const;
		bool downMaxxed() const;

		int numVerticalElements() const { return m_numVerticalElements; }
		int numEntries() const { return static_cast<int>(m_entries.size()); }

		// Read item entry `index`'s def + stack (false if oob / not an item entry). Lets a Lua view mirror the
		// list contents without the C++ viewport (m_iconBgFades is only the visible window, not 1:1 m_entries).
		bool itemEntryAt(int index, ItemDefines::ItemDefinition& def, int& stack) const;

		pair<shared_ptr<GameIcon>, int> popClickedGameIcon();
		pair<shared_ptr<GameIcon>, int> popClickedDongle(const string& dongleName);

	private:
		void input() final;
		void render() final;

		void refreshView();

		int m_scrollOffset;

		bool m_lowercaseTxt;
		bool m_needDescriptionRefresh{true};
		bool m_allowDraggingIcons{false};

		const int m_graphicalWidth;
		const int m_numVerticalElements;
		const sf::Vector2i m_titleOffset;
		const sf::Vector2i m_iconOffsetWithinSlot;

		int m_descriptionWidth;
		int m_descriptionMaxLines;
		float m_descriptionBorderThickness;

		sf::Vector2i m_descriptionOffset;
		sf::Color m_descriptionColor;
		sf::Color m_descriptionOutlineColor;

		set<int> m_darkenedEntries;

		vector<shared_ptr<Text>> m_labels;
		vector<shared_ptr<TextBox>> m_descriptions;
		vector<pair<shared_ptr<Button>, shared_ptr<GameIcon>>> m_iconBgFades;

		map<string, vector<pair<shared_ptr<Button>, shared_ptr<GameIcon>>>> m_dongles;
		map<string, sf::Vector2i> m_dongleOffset;
		map<string, set<int>> m_dongleExceptions;
		map<string, map<int, shared_ptr<Tooltip>>> m_dongleTooltips;

		struct IconEntry
		{
			virtual ~IconEntry() {}
			int id;
		};

		struct ItemEntry : public IconEntry
		{
			virtual ~ItemEntry() {}
			int stack;
			ItemDefines::ItemDefinition def;
		};

		struct SpellEntry : public IconEntry
		{
			virtual ~SpellEntry() {}
			int lvl;
			vector<pair<int16_t, int16_t>> bpoints;
		};

		// entry, count][visible
		vector<pair<shared_ptr<IconEntry>, bool>> m_entries;
};
