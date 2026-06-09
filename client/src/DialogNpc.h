#pragma once

#include "DialogPanel.h"

class TextLines;
class Sprite;
class ScrollBar;
class SpriteRo;

class DialogNpc : public DialogPanel
{
	public:
		enum Interface
		{
			NpcName = 1,
			GossipText,
			GossipOptions,
			GossipOptionsTitle,
			GoodbyeButton,
			DescriptionScroll,
			DescriptionScrollBg
		};

		enum GossipType
		{
			GossipDialog,
			GossipQuestAccept,
			GossipQuestComplete,
			GossipVendor,
		};

	public:
		DialogNpc(World& owner, const int id);
		virtual ~DialogNpc();

		void applyText(const int entry, const string& npcName);
		void addGossip_Dialog(const int entry);
		void addGossip_QuestAccept(const int questId);
		void addGossip_QuestComplete(const int questId);
		void addVendorButton();

		void createGossipOptions();
		void clearGossip();

		// ---- Lua view: read the populated dialog + drive an option select (the same per-type flows the
		//      C++ click dispatch runs; extracted into selectGossip so both paths share it). ----
		const string& npcName() const { return m_npcName; }
		const string& gossipText() const { return m_gossipText; }
		int  gossipOptionCount() const { return static_cast<int>(m_serverGossipMenu.size()); }
		bool gossipOptionAt(const int idx, int& type, int& entry, string& label) const;
		void selectGossip(const int lineIdx);
		
	protected:
		virtual void input() override;
		virtual void render() override;
		virtual void onClose() override;

	private:
		void adjustGossipY();

		sf::Color getGossipOptionColor() const { return sf::Color(189, 166, 145, 255); }

		int m_textId{0};

		bool m_readyForInput{false};

		string m_npcName;      // formatted dialog content captured for the Lua view
		string m_gossipText;
		
		shared_ptr<Sprite> m_questDone;
		shared_ptr<Sprite> m_questAvail;
		shared_ptr<Sprite> m_gossipOptionSpr;
		shared_ptr<Sprite> m_vendorGold;

		shared_ptr<SpriteRo> m_descScrBarBg;
		shared_ptr<ScrollBar> m_descScrollBar;
		
		shared_ptr<TextBoxRo> m_description;
		shared_ptr<TextLines> m_gossipOptions;

		struct GossipField
		{
			GossipType type;
			int entry{0};
		};

		// lineIndex, data about it
		map<int, GossipField> m_serverGossipMenu;
};

