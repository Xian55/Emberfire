#include "stdafx.h"
#include "TextBox.h"
#include "Application.h"
#include "Text.h"

#include "..\StringHelpers.h"

#include <assert.h>

TextBox::TextBox(const shared_ptr<sf::Font> font, const int characterSize) :
	m_width(0),
	m_height(0),
	m_align(AlignLeft),
	m_font(font),
	m_characterSize(characterSize),
	m_color(sf::Color::White)
{
	ASSERT(m_characterSize > 0);
	ASSERT(m_font != nullptr);
	m_text = make_unique<Text>(m_font);
}

TextBox::~TextBox()
{

}

void TextBox::draw()
{
	m_text->draw(int(m_text->getPosition().x), int(m_text->getPosition().y));
}

void TextBox::syncPosition(sf::Vector2i& textBoxPos)
{
	m_position = sf::Vector2i(m_text->getPosition());
	textBoxPos = m_position;
}

void TextBox::moveTo(const sf::Vector2i& pos)
{
	if (m_position == pos)
		return;

	const auto diff = pos - m_position;	
	m_text->setPosition(m_text->getPosition() + sf::Vector2f(diff));
	m_position = pos;
}

void TextBox::resizeFontSize(const int size)
{
	if (m_characterSize == size)
		return;

	m_characterSize = size;
	ASSERT(m_characterSize > 0);
	m_text->setCharacterSize(m_characterSize);
}

/*static*/
string TextBox::getStringKey(const StringKeys key)
{
	switch (key)
	{
		case StringKeys::Key_ReplaceSpaceToMaxWidth:
			return "{$MW}";
	}

	return "";
}

void TextBox::clear()
{
	m_text->setOriginalString("");
	m_originalString.clear();
	m_position = {};
}

// Very slow
void TextBox::setData(const int x, const int y, string str, const int maxWidth, const Align algn, 
	const bool centerHeight /*= false*/, const float thickness /*= 0.0f*/, const sf::Color outlineColor /*= sf::Color::Black*/, const int maxLines /*= -1*/)
{
	if (m_align == algn && m_originalString == str && m_position == sf::Vector2i(x, y))
		return;

	ASSERT(m_characterSize > 0);

	m_originalString = str;
	m_align = algn;
	m_position = sf::Vector2i(x, y);
	m_numLines = 1;

	auto maxWKey = str.find(getStringKey(Key_ReplaceSpaceToMaxWidth));
	
	// Implement Key_ReplaceSpaceToMaxWidth
	if (maxWKey != string::npos)
	{
		Util::strReplaceAll(str, getStringKey(Key_ReplaceSpaceToMaxWidth), "");

		int spaceWidth = Text::getStringWidth(*getFont(), m_characterSize, false, " ", true);
		int curWidth = Text::getStringWidth(*getFont(), m_characterSize, false, str, true);

		if (curWidth < maxWidth && spaceWidth > 0)
		{
			int diff = maxWidth - curWidth;
			int numNewSpaces = diff / spaceWidth;

			for (int i = 0; i < numNewSpaces; ++i)
				str.insert(maxWKey, " ");
		}
	}

	m_text->setColorIfNot(sf::Color(m_color));
	m_text->setOutlineThickness(thickness);
	m_text->setOutlineColor(outlineColor);
	m_text->setCharacterSize(m_characterSize);

	size_t prevSpace = 0;
	string currentWord;
	string result;
	string thisLine;

	bool endedTripleDot = false;

	for (size_t i = 0; i < str.size(); ++i)
	{
		const char letter = str[i];
		currentWord.push_back(letter);
		
		// Function
		auto checkWordLen = [&]()
		{
			prevSpace = i;
			thisLine += currentWord;

			auto len = Text::getStringWidth(*getFont(), m_characterSize, false, thisLine, true);

			if (len > maxWidth)
			{
				if (++m_numLines > maxLines && maxLines != -1)
				{
					result += "...";
					endedTripleDot = true;
					return true;
				}

				thisLine = currentWord;
				result += "\n";
			}

			result += currentWord;
			currentWord.clear();
			return false;
		};
		
		// Check which letter we're iterating at
		if (letter == '\n')
		{
			if (checkWordLen())
				break;

			thisLine.clear();
			result += currentWord;
			currentWord.clear();

			if (++m_numLines > maxLines && maxLines != -1)
			{
				// We just added a \n, undo that only
				result += "...";
				endedTripleDot = true;
				break;
			}
		}
		else if (letter == ' ' || i == str.size() - 1)
		{
			if (checkWordLen())
				break;
		}
	}

	if (endedTripleDot)
	{
		int it = int(result.size()) - 4;

		while (it > 0 && result[it] == '\n')
		{
			result.erase(result.begin() + it);
			it = int(result.size()) - 4;
		}
	}

	m_lastLineWidth = Text::getStringWidth(*getFont(), m_characterSize, false, thisLine, true);

	m_text->setOriginalString(result);

	m_width = static_cast<int>(m_text->getGlobalBounds().width);
	m_height = static_cast<int>(m_text->getGlobalBounds().height);

	int xOffset = 0;
	int yOffset = 0;

	if (algn == AlignCenterAbsolute)
		xOffset -= m_width / 2;
	else if (m_width < maxWidth)
		xOffset = (maxWidth - m_width) / 2;

	if (centerHeight)
		yOffset -= (m_numLines * m_characterSize) / 2;

	if (algn == AlignCenterBounds || algn == AlignCenterAbsolute)
		m_text->setPosition({ static_cast<float>(x + xOffset), static_cast<float>(y + yOffset) });
	else
		m_text->setPosition({ static_cast<float>(x), static_cast<float>(y + yOffset) });
}

void TextBox::setColor(const sf::Color color, const sf::Color outlineColor)
{
	m_color = color;
	m_text->setColorIfNot(color, outlineColor);
}
