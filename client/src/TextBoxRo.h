#pragma once

#include "RenderObject.h"
#include "TextBox.h"

class TextBoxRo :	public RenderObject
{
	public:
		TextBoxRo(RenderObject& owner, const int id, const string& fontName, const int w, const int characterSize,
			const TextBox::Align algn = TextBox::AlignLeft, const bool centerHeight = false, const float thickness = 0.0f);
		TextBoxRo(RenderObject& owner, const int id, const Assets::FontId fontId, const int w, const int characterSize,
			const TextBox::Align algn = TextBox::AlignLeft, const bool centerHeight = false, const float thickness = 0.0f)
			: TextBoxRo(owner, id, Assets::fontFile(fontId), w, characterSize, algn, centerHeight, thickness) {}
		virtual ~TextBoxRo();

		void setString(const string& str, const sf::Color color = sf::Color::White, const sf::Color outlineColor = sf::Color::White, const int maxLines = -1, const bool enableScroll = false);
		void pumpScrollOffset(const int val);

		auto getScrollMax() const { return m_scrollMax; }
		auto getScrollOffset() const { return m_scrollOffset; }
		auto getDrawnHeight() const { return m_text.getHeight(); }
		auto& getTextRef() { return m_text; }

	private:
		void input() final {}
		void render() final;

		void scrollUp();
		void scrollDown();

		const bool m_centerHeight;

		const int m_width;
		const int m_characterSize;
		
		const float m_thickness;

		const TextBox::Align m_align;

		int m_scrollMax{0};
		int m_scrollOffset{0};
		int m_scrollMaxLines{0};
		int m_scrollDownCap{0};

		TextBox m_text;
		
		
		string m_scrollTextFull;
};

