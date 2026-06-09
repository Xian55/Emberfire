#pragma once

#include "RenderObjectHolder.h"

#include "..\Shared\ChatDefines.h"
#include "..\Shared\ItemDefines.h"

#include <deque>

class PromptBox;
class Button;
class SpriteRo;
class ItemIcon;
class Tooltip;

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
		CombatLogMaxLines = 256,
	};

	public:
		// Combat-log line category (client-side only, for the Lua combat tab's filters).
		enum CombatCategory
		{
			CombatOutgoing = 1,   // my damage
			CombatIncoming,       // damage to me
			CombatHeal,           // heals (any direction)
			CombatMisc,           // misses/auras/other units
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
		void addLineColor(const string& msg, const sf::Color color, const ItemDefines::ItemDefinition* linkedItem = nullptr, const int channelTag = ChatDefines::Channels::System);
		// Combat-log feed (separate buffer so combat spam can't evict chat history; read by the Lua combat tab).
		void addCombatLogLine(const string& msg, const sf::Color color, const CombatCategory cat);
		void registerFromSlot(const string& from, const string& msg);
		void promptLinkAnItem(const ItemDefines::ItemDefinition& itemDef);
		void setInUse(const bool v);

		bool isMuteAllChat() const { return m_muteAllChate; }

		static sf::Color getChatColor(const ChatDefines::Channels c);

		// ---- Lua chat view: when set, render() draws nothing and input() only watches the open keys (Enter /
		//      slash -> CHAT_OPEN event); the C++ chat stays the engine (line buffer, channel state, command
		//      parsing, packets, link tooltips) and the Lua addon owns visuals, scroll and the edit box. ----
		void setLuaView(const bool v) { m_luaView = v; }

		// Send path (channel-swap / "/"-command / chat packet + channel restore), shared by the C++ prompt
		// and the Lua editbox (SubmitChat). Prepends any pending linked-item name.
		void submitText(const string& raw);
		// The live while-typing channel swap ("/s " etc); true = handled (the caller clears its input).
		bool trySwapChannel(const string& typed);

		// ---- read accessors for the Lua view ----
		int  lineCount() const;
		bool lineAt(const int idx, string& text, uint32_t& rgba, bool& hasLink, int& channel) const;
		string lineSender(const int idx) const;             // "" if the line has no known sender
		shared_ptr<Tooltip> lineLinkTooltip(const int idx); // the linked item's tooltip (nullptr if none)
		string prefixText();                                // channel prefix incl. a pending item link
		uint32_t prefixColor() const;                       // packed 0xRRGGBBAA channel color
		int  combatLineCount() const { return static_cast<int>(m_combatLines.size()); }
		bool combatLineAt(const int idx, string& text, uint32_t& rgba, int& category) const;

	protected:
		void input() final;
		void render() final;

		string getChannelPrefix(const ChatDefines::Channels c, const string& whisperName = "unknown");

	private:
		void setChannel(const ChatDefines::Channels c);
		void printHelp();

		bool parseChannelSwap(const string& inputstr, const bool extraSpace, string& out_extractedStr);
		bool processServerCommand(string enteredTxt);
		bool hasAndExtract(const string& prefx, const string& content, string& output) const;

		string getMsgSenderName(const string& msg) const;

		static string ctxMenuOptionStr(const CtxMenuOptions id);

		bool m_muteAllChate = false;
		bool m_inUse{ false };
		bool m_luaView{ false };

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
		unordered_map<uintptr_t, int> m_lineChannels;   // line ptr -> ChatDefines channel (Lua tab filters)

		struct CombatLine { string text; uint32_t rgba; int category; };
		deque<CombatLine> m_combatLines;

		shared_ptr<ItemIcon> m_clickedLinkedItem;
		ItemDefines::ItemDefinition m_linkedItem;
};

