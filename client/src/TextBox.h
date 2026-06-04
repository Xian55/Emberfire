#pragma once

class Text;

class TextBox
{
	public:
		enum Align
		{
			AlignLeft,
			//AlignRight, todo
			AlignCenterBounds, // Center of the box
			AlignCenterAbsolute, // Center of itself
		};

		enum StringKeys
		{
			Key_ReplaceSpaceToMaxWidth = 1,
		};

	public:
		TextBox(const shared_ptr<sf::Font> font, const int characterSize);
		virtual ~TextBox();

		void setColor(const sf::Color color, const sf::Color outlineColor = sf::Color::Black);
		void draw();
		void moveTo(const sf::Vector2i& pos);
		void syncPosition(sf::Vector2i& textBoxPos);
		void clear();

		// Not fast at all, so don't set new data every frame if it can be avoided
		void setData(const int x, const int y, string str, const int maxWidth, const Align algn,
			const bool centerHeight = false, const float thickness = 0.0f, const sf::Color outlineColor = sf::Color::Black, const int maxLines = -1);

		// Will not re-calculate alignments and maxWidth logic, that has to be done via setData
		void resizeFontSize(const int size);

		auto getFont() const { return m_font; }
		auto getHeight() const { return m_height; }
		auto getWidth() const { return m_width; }
		auto getPosition() const { return m_position; }
		auto getColor() const { return m_color; }
		auto getCharacterSize() const { return m_characterSize; }
		auto getNumLines() const { return m_numLines; }
		auto getLastLineWidth() const { return m_lastLineWidth; }

		const string& getOriginalString() const { return m_originalString; }
		
		static string getStringKey(const StringKeys key);

		Text& getTextRef() { return *m_text; }

	private:
		int m_width;
		int m_height;
		int m_characterSize;
		int m_numLines;
		int m_lastLineWidth{0};

		sf::Color m_color;

		Align m_align;
		string m_originalString;
		sf::Vector2i m_position;

		shared_ptr<sf::Font> m_font;
		unique_ptr<Text> m_text;
};
