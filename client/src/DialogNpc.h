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
		
	protected:
		virtual void input() override;
		virtual void render() override;
		virtual void onClose() override;

	private:
		void adjustGossipY();

		sf::Color getGossipOptionColor() const { return sf::Color(189, 166, 145, 255); }

		int m_textId{0};

		bool m_readyForInput{false};
		
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

