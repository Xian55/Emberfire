#pragma once

// The font we use must not be destroyed as long as the text is still being used
class Text : public sf::Text
{
	public:
		Text(const shared_ptr<sf::Font> font);
		virtual ~Text();

		void draw(const int x, const int y);
		void setColorIfNot(const sf::Color color, const sf::Color outlineColor = sf::Color::Black);
		void setOriginalColor(const sf::Color color);
		void restoreColor();
		void setStringMaxWidth(string str, const int maxWidth, const bool backwards = false, const bool percise = false, const string extra = "", const bool ellipses = false);
		void setOriginalString(const string& str);
		void setShadowOffset(const int size) { m_shadowOffset = size; }
		void clearSizeCache() { m_widths.clear(); }

		static int getStringWidth(const sf::Font& font, const int characterSize, const bool bold, const string& str, const bool real = false);
		
		const auto& getOriginalColor() const { return m_originalColor; }
		const string& getOriginalString() const { return m_originalString; }

		Text& setStringf(const char* format, ...);
		Text& setOriginalStringf(const char* format, ...);

	private:
		int m_shadowOffset{0};
		string m_originalString;
		sf::Color m_originalColor;
		shared_ptr<sf::Font> m_font;

		// It's slow to calculate how wide a string is. Cache results.
		unordered_map<string, int> m_widths;
};