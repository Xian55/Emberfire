#pragma once

#include "ExpandableWindow.h"

class TextBox;
class TooltipGlyph;

class Tooltip : public ExpandableWindow
{
	public:
		enum Defines
		{
			MaxTooltipWidth = 255
		};

	public:
		Tooltip(RenderObject& owner, const sf::Vector2i topLeftCorner);
		~Tooltip();

		void draw();
		void moveTo(const sf::Vector2i& pos);
		void addGlyph(const shared_ptr<TooltipGlyph> glyph);
		void setShadowOffset(const int val) { m_shadowOffset = val; }
		void setMaxWidth(const int val) { m_maxWidth = val; }

		int addLine(const string& fontname, const int characterSize, const string& str, const sf::Color color = sf::Color::White, const bool incrementHeight = true);
		int addLine(const Assets::FontId fontId, const int characterSize, const string& str, const sf::Color color = sf::Color::White, const bool incrementHeight = true)
			{ return addLine(Assets::fontFile(fontId), characterSize, str, color, incrementHeight); }

		void setAllowMoreWidthIfLastLineIsUnderThisValue(const int val) { m_allowMoreWidthIfLastLineIsUnderThisValue = val; }

		const auto getHeight() const { return m_height; }
		const auto getWidth() const { return m_width; }
		const auto getTextOffset() const { return m_textOffset; }

	private:
		void drawGlyph(const shared_ptr<TooltipGlyph> action, const sf::Vector2i pos) const;

		shared_ptr<TooltipGlyph> getLineGlyph(const size_t idx) const;

		int m_width;
		int m_height;
		int m_shadowOffset{0};
		int m_allowMoreWidthIfLastLineIsUnderThisValue{0};
		int m_maxWidth{Defines::MaxTooltipWidth};
		
		// Allows for multiple "lines" on the same line (multi-colored lines)
		bool m_hasMultiLineLine{false};

		const sf::Vector2i m_textOffset;
		
		vector<unique_ptr<TextBox>> m_lines;
		map<size_t, shared_ptr<TooltipGlyph>> m_glyphs;
};

