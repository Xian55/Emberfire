#pragma once

#include "WorldPanel.h"

#include "..\Shared\PlayerDefines.h"

class GameIcon;
class ItemIcon;
class ClientPlayer;
class TextBoxRo;
class SelectionButtons;
class Sprite;
class Tooltip;
class DraggableNode;

class Equipment : public WorldPanel
{
	public:
		enum Interface
		{
			//
			HelmIcon,
			WeaponIcon,
			ShieldIcon,
			RangedIcon,
			ChestIcon,
			NecklaceIcon,
			GlovesIcon,
			Ring1Icon,
			FeetIcon,
			LegsIcon,
			BeltIcon,
			Ring2Icon,

			//
			LevelUp,
			LevelUpExclaim,
			ConfirmLevelUp,
			CancelLevelUp,
			NameLabel,
			SubnameLabel,

			//
			StatsTabs,

			//
			SpendPointKey = 5000,
			SpendPointMinusKey = 6000,

			//
			LabelsKey = 10000,
			ValuesKey = LabelsKey + 5000,
		};

		enum StatView
		{
			SView_General,
			SView_Combat,
			SView_Skills,
		};

		enum CustomLabel
		{
			LevelupCost = -1,
			CombatRating = -2,
			CombatRatingText = -3,
		};

		enum Defines
		{
			MaxPlayerLevel = 25
		};

	public:
		Equipment(World& owner, const int id);
		virtual ~Equipment();

		void toggleSpendingPoints(const bool v);
		void updatePlayer(shared_ptr<ClientPlayer> plr);
		void clearPendingServerSend() { m_pendingServerSpend = false; }

		const auto& getPendingStatInvestments() const { return m_pendingStatInvestments; }

		// --- Lua-driven stat-point spending (stats only; no Abilities/panel coupling) ---
		// The Lua Equipment view drives the spend flow through these while the C++ window is force-hidden.
		// They reuse the same machinery as input()/toggleSpendingPoints but skip the panel/spellbook juggling.
		void beginStatSpend();
		void cancelStatSpend();
		bool addStatPoint(const int statVar);
		bool removeStatPoint(const int statVar);
		bool canAddStatVar(const int statVar) const;
		bool canMinusStatVar(const int statVar) const;
		int  pendingStatPoints(const int statVar) const;
		bool isSpendingPoints() const { return m_spendingPoints; }
		bool hasUnspentPoints() const;
		void confirmStatSpend();
		// Passthrough so the Lua sheet value mirrors the C++ value byte-for-byte (spend preview + speed fmt).
		string sheetValue(const int var) { return getEquipmentValue(var); }

		static UnitDefines::EquipSlot convertInterface(const Interface id);

		static shared_ptr<Tooltip> spawnUpgradeTooltip(const int totalInvest, const int cost, const string& itemName, const string& pointName);

	private:
		void input() final;
		void render() final;
		void givenGabbedIcon(shared_ptr<GameIcon> myIcon, shared_ptr<GameIcon> heldIcon) final;
		void onClose() final;

		void setStatView(const StatView view);
		void refreshPlusMinusStatSpending_Slow();
		void calculateBaseStats();

		bool isAllow_AddStat(const UnitDefines::Stat stat) const;
		bool isAllow_MinusStat(const UnitDefines::Stat stat) const;

		bool canUseSkill(const UnitDefines::Stat stat) const;
		bool overrideIconGrab(shared_ptr<GameIcon> targetIcon) final;

		string getEquipmentValue(const int var);

		sf::Color variableVaueColor(const int var);
		
		shared_ptr<ItemIcon>& configSlot(shared_ptr<ItemIcon>& ptr);

		shared_ptr<Button> attachSpendButton(const int var, const sf::Vector2i& offset);
		shared_ptr<Button> attachSpendMinButton(const int var, const sf::Vector2i& offset);
		shared_ptr<TextBoxRo> attachLabel(const int var, const sf::Vector2i& offset);
		shared_ptr<TextBoxRo> attachValue(const int var, const sf::Vector2i& offset, const int alignment, const bool spendButtons = true, const int spendVar = -1);

		bool m_pendingServerSpend{false};
		bool m_spendingPoints{false};
		bool m_wasOpenedRecently{false};

		int m_lastKnownLevelupCost{0};

		map<int, shared_ptr<TextBoxRo>> m_currentLabels;
		map<int, shared_ptr<TextBoxRo>> m_currentValues;
		map<int, shared_ptr<Button>> m_currentAddBtns;
		map<int, shared_ptr<Button>> m_currentMinusBtns;
		
		map<UnitDefines::Stat, int> m_pendingStatInvestments;
		map<UnitDefines::Stat, int> m_cachedBaseStats;

		StatView m_view;

		shared_ptr<ClientPlayer> m_player;
		shared_ptr<SelectionButtons> m_viewChoices;
		shared_ptr<Sprite> m_crLabel;
		shared_ptr<Sprite> m_levelupCostLabel;
		shared_ptr<Sprite> m_levelupBaseNotice;
		shared_ptr<Button> m_levelupButton;
		shared_ptr<Button> m_levelupPurpleButton;
		shared_ptr<Button> m_tab1;
		shared_ptr<Button> m_tab2;
		shared_ptr<Button> m_tab3;

		sf::Color m_labelColor;
		sf::Color m_labelShadow;

		unique_ptr<DraggableNode> m_dragNode;
};

