#pragma once

#include "RenderObject.h"
#include "Text.h"

#include "..\Math.h"

class ScrollBar;
class TextLines;
class Tooltip;

class TextLine
{
	friend class TextLines;

	public:
		TextLine(const int characterSize, shared_ptr<sf::Font> f, const string& s, const sf::Color colr = sf::Color::White);

		void setColor(const sf::Color color) { m_text.setOriginalColor(color); }
		void setTextStr(const string& str) { m_text.setOriginalString(str); }
		void setTooltipStr(const string& str) { m_tooltipStr = str; }

		const string& getTextStr() const { return m_text.getOriginalString(); }
		const string& getTooltipStr() const { return m_tooltipStr; }
		const sf::Vector2i getRenderPos() const { return m_renderPos; }

		const auto getTextBounds() const { return m_text.getGlobalBounds(); }
		const auto getCharacterSize() const { return m_text.getCharacterSize(); }

	private:
		int m_idxCache{0};

		Text m_text;
		string m_tooltipStr;
		sf::Vector2i m_renderPos;

		shared_ptr<Tooltip> m_tooltip;
};

class TextLines : public RenderObject
{
	public:
		enum Defines
		{
			DialogWindowMaxLines = 256
		};
	
	public:
		TextLines(RenderObject& owner, const int id, const string& fontName, const Util::GeoBox2d& box, const int fontSize = 12);
		virtual ~TextLines();
		
		void setBounds(const Util::GeoBox2d& box) { m_box = box; }
		void setClickableLines(const bool value) { m_clickableLines = value; }
		void setScrollObject(shared_ptr<ScrollBar> obj) { m_scrollObject = obj; }
		void setLeading(const int pixels) { m_leading = pixels; }
		void setMaxLines(const size_t val) { m_maxLines = val; }
		void clear() { m_lines.clear(); }
		void setExtraScrollOfset(const int v) { m_extraScrollOffset = v; }
		void setClickableMouseButton(const sf::Mouse::Button b) { m_clickableMouseButton = b; }
		void setSelectedLine(const string& txt);
		void setShadowOffset(const int idx) { m_shadowOffset = idx; }
		void addLine(string line, const sf::Color color = sf::Color::White, uintptr_t* prunedLineAddr = nullptr);

		void removeLine(const int idx);
		void addLines(vector<string>& lines);

		// Very slow if there are already lines
		void setDialogCharacterSize(const int size);		

		int popPendingClickedLineId();
		int getScrollOffset() const;
		
		auto getFont() const { return m_font; }
		auto getNumLines() const { return static_cast<int>(m_lines.size()); }
		auto getScrollObject() const { return m_scrollObject; }
		auto getSelectedLineId() const { return m_selectedLineId; }
		auto getLeading() const { return m_leading; }
		auto getCharacterSize() const { return m_characterSize; }
		auto getHoveringLineId() const { return m_lineHovering; }

		const Util::GeoBox2d& getBounds() const { return m_box; }
		const std::string& getSelectedLine() const { return m_selectedLine; }

		TextLine* getLine(const int idx);
		string popPendingClickedLine();

	protected:
		virtual void input() override;
		virtual void render() override;

		bool isMousedOver(const bool useRealPosition /* = false */) final;

		shared_ptr<sf::Font> m_font;
		shared_ptr<ScrollBar> m_scrollObject;
					
		Util::GeoBox2d m_box;

		// If set to true then lines being moused over will be highlighted and clickable
		bool m_clickableLines{0};

		int m_characterSize{0};
		int m_clickedLineId{0};
		int m_lineHovering{0};
		int m_shadowOffset{0};

		size_t m_maxLines{0};

	private:
		int m_leading{0};
		int m_extraScrollOffset{0};

		sf::Mouse::Button m_clickableMouseButton;
		string m_selectedLine;
		int m_selectedLineId{0};

		vector<shared_ptr<TextLine>> m_lines;
};

