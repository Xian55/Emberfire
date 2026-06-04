#pragma once

#include "RenderObjectHolder.h"

#include "..\Shared\ChatDefines.h"
#include "..\Shared\ItemDefines.h"

class PromptBox;
class Button;
class SpriteRo;
class ItemIcon;

class GameChat : public RenderObjectHolder
{
	enum CtxMenuOptions
	{
		DdoWhisper,
		DdoInvite,
		DdoCancel
	};

	enum Defines
	{
		CharacterSize = 12,
	};

	enum Interface
	{
		RoFullUp = 1,
		RoFullDown,
		RoPromptBox,
		RoCloseTooltipBtn,
	};

	public:
		GameChat(RenderObjectHolder& owner, const int id, const sf::Vector2i& pos = sf::Vector2i{});
		virtual ~GameChat();

public:
		void notifyCtxMenuClicked(const int id, const string& lineClicked) final;

public:
		void setWhispering(const string& name);
		void recvMsg(const string& msg, const string& from, const ChatDefines::Channels c, const ItemDefines::ItemDefinition* linkedItem = nullptr);
		void addLine(const string& msg, const ChatDefines::Channels c, const ItemDefines::ItemDefinition* linkedItem = nullptr);
		void addLineColor(const string& msg, const sf::Color color, const ItemDefines::ItemDefinition* linkedItem = nullptr);
		void registerFromSlot(const string& from, const string& msg);
		void promptLinkAnItem(const ItemDefines::ItemDefinition& itemDef);
		void setInUse(const bool v);

		bool isMuteAllChat() const { return m_muteAllChate; }

		static sf::Color getChatColor(const ChatDefines::Channels c);

	protected:
		void input() final;
		void render() final;

		string getChannelPrefix(const ChatDefines::Channels c, const string& whisperName = "unknown");

	private:
		void setChannel(const ChatDefines::Channels c);
		void printHelp();
		
		bool parseChannelSwap(const string& inputstr, const bool extraSpace, string& out_extractedStr);
		bool processServerCommand(string enteredTxt);
		bool hasAndExtract(const string& prefx, string& output) const;

		string getMsgSenderName(const string& msg) const;

		static string ctxMenuOptionStr(const CtxMenuOptions id);

		bool m_muteAllChate = false;
		bool m_inUse{ false };

		string m_rightClickedName;
		string m_whisperTarget;
		string m_linkedItemNameCache;

		ChatDefines::Channels m_channel;
		ChatDefines::Channels m_channelPrevious;

		shared_ptr<PromptBox> m_promptBox;
		shared_ptr<Button> m_promptEnterButton;
		shared_ptr<Button> m_fullDownButton;
		shared_ptr<Button> m_fullUpButton;
		shared_ptr<Button> m_closeLinkTooltipBtn;
		shared_ptr<SpriteRo> m_bgSprite;
		
		unordered_map<string, string> m_msgNames;
		unordered_map<uintptr_t, shared_ptr<ItemIcon>> m_itemLinks;

		shared_ptr<ItemIcon> m_clickedLinkedItem;
		ItemDefines::ItemDefinition m_linkedItem;
};

