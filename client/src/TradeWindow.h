#pragma once

#include "DialogPanel.h"

#include "..\Shared\GamePacketServer.h"

class GameIcon;
class TextBoxRo;
class SpriteRo;
class Button;
class Sprite;
class ClientPlayer;

class TradeWindow : public DialogPanel
{
	public:
		enum Interface
		{
			// Left column (local player's items) - 5 slots
			IconSlotLocal1 = 1,
			IconSlotLocal5 = 5,

			// Right column (remote player's items) - 5 slots
			IconSlotRemote1 = 6,
			IconSlotRemote5 = 10,

			// Highlights for all 10 slots
			HighlightLocal1 = 11,
			HighlightLocal5 = 15,
			HighlightRemote1 = 16,
			HighlightRemote5 = 20,

			// Item titles for all 10 slots
			TitleLocal1 = 21,
			TitleLocal5 = 25,
			TitleRemote1 = 26,
			TitleRemote5 = 30,

			// Portraits
			PortraitLocalFrame,
			PortraitRemoteFrame,

			// Buttons
			TradeButton,

			// Name labels
			LocalNameText,
			RemoteNameText,

			// Gold display
			LocalGoldText,
			RemoteGoldText,
			LocalGoldHighlight,
		};

	public:
		TradeWindow(World& owner, const int id);
		virtual ~TradeWindow();

		void reset();
		
		// Set the trading partners
		void setLocalPlayer(shared_ptr<ClientPlayer> player);
		void setRemotePlayer(shared_ptr<ClientPlayer> player);

		// Item management - local side (what we're offering)
		void addLocalItem(const ItemDefines::ItemDefinition entry, const int itemGuid, const int stackSize, const int slot);
		void removeLocalItem(const int slot);
		void clearLocalItems();

		// Item management - remote side (what they're offering)
		void addRemoteItem(const ItemDefines::ItemDefinition entry, const int stackSize, const int slot);
		void removeRemoteItem(const int slot);
		void clearRemoteItems();

		// Gold management
		void setLocalGold(const int amount);
		void setRemoteGold(const int amount);

		// Ready state
		void setLocalReady(const bool ready);
		void setRemoteReady(const bool ready);

		bool isLocalIconId(const int id) { return id >= IconSlotLocal1 && id <= IconSlotLocal5; }
		bool isLocalReady() const { return m_localReady; }
		bool isRemoteReady() const { return m_remoteReady; }

		virtual void onClose() final;

	private:
		void input() final;
		void render() final;

		void updatePortrait(shared_ptr<ClientPlayer> player, bool isLocal);
		void renderPortrait(bool isLocal);

		void givenGabbedIcon(shared_ptr<GameIcon> myIcon, shared_ptr<GameIcon> heldIcon) final;

		shared_ptr<GameIcon> attachIcon(shared_ptr<GameIcon> gi, bool isLocal);
		shared_ptr<Button> attachHighlight(shared_ptr<Button> button, bool isLocal);
		shared_ptr<TextBoxRo> attachTitle(shared_ptr<TextBoxRo> txt, bool isLocal);

		// Portrait data
		shared_ptr<Sprite> m_localPortrait;
		shared_ptr<Sprite> m_remotePortrait;
		int m_localPortraitOffset{0};
		int m_remotePortraitOffset{0};
		sf::Vector2i m_localPortraitPos;
		sf::Vector2i m_remotePortraitPos;
		static constexpr int PORTRAIT_RADIUS = 32;

		// Player references
		shared_ptr<ClientPlayer> m_localPlayer;
		shared_ptr<ClientPlayer> m_remotePlayer;

		// Name text boxes
		shared_ptr<TextBoxRo> m_localNameTxt;
		shared_ptr<TextBoxRo> m_remoteNameTxt;

		// Gold display
		shared_ptr<TextBoxRo> m_localGoldTxt;
		shared_ptr<TextBoxRo> m_remoteGoldTxt;
		shared_ptr<Button> m_localGoldHighlight;

		// Ready state
		bool m_localReady{false};
		bool m_remoteReady{false};
		shared_ptr<Sprite> m_localReadySprite;
		shared_ptr<Sprite> m_remoteReadySprite;
};
