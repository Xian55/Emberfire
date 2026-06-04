#pragma once

#include "MouseableNode.h"

class ClientObject;
class Text;
class Sprite;
class ChatBubble;

class ClientObjectOverhead : public MouseableNode
{
	public:
		ClientObjectOverhead(ClientObject const& owner);
		~ClientObjectOverhead();

		void draw();
		void refreshName();
		void setChatBubble(const string& str, const sf::Color colorInt);
		void setAlpha(const uint8_t a) { m_alpha = a; }
		void setDrawOptions(const bool hideHP, const bool hideNameAndChat);

		int getNameHeight() const;

	private:
		void drawHp(int& upwardY);
		void drawName(Text& obj, int& upwardY);
		void drawChatBubble(int& chat);

		bool shouldDrawName() const;
		bool shouldDrawNameplate() const;

		bool m_hideHP{ false };
		bool m_hideNameAndChat{ false };

		ClientObject const& m_owner;

		uint8_t m_alpha;

		unique_ptr<Text> m_nameDraw;
		unique_ptr<Text> m_subNameDraw;

		unique_ptr<ChatBubble> m_chatBubble;

		shared_ptr<Sprite> m_barBg;
		shared_ptr<Sprite> m_barHp;
		shared_ptr<Sprite> m_barParty;
		shared_ptr<Sprite> m_killMarker;

		time_t m_chatBubbleEnd;
};

