#pragma once

#include "ExpandableWindow.h"

class TextBox;

class ChatBubble :	public ExpandableWindow
{
	enum Defines
	{
		MaxBubbleWidth = 255
	};

	public:
		ChatBubble(RenderObject& owner, const sf::Vector2i topLeftCorner);
		virtual ~ChatBubble();

		void draw();
		void setText(const string& fontname, const int characterSize, const string& str, const sf::Color color = sf::Color::White);
		void moveTo(const sf::Vector2i pos);
		void refreshBounds();

		const auto getHeight() const { return m_height; }
		const auto getWidth() const { return m_width; }
		const auto getTextOffset() const { return m_textOffset; }

	private:
		int m_width;
		int m_height;

		const sf::Vector2i m_textOffset;
		
		unique_ptr<TextBox> m_text;
};

