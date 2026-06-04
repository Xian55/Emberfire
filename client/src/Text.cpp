#include "stdafx.h"
#include "Text.h"
#include "Application.h"

#include <assert.h>

Text::Text(const shared_ptr<sf::Font> font) :
	m_font(font)
{
	ASSERT(font != nullptr);
	setFont(*m_font);
	m_originalColor = getFillColor();
}

Text::~Text()
{

}

void Text::draw(const int x, const int y)
{
	if (m_shadowOffset != 0)
	{
		// Shadow goes first (under)
		sf::Color col = getFillColor();
		sf::Color borderColor = getOutlineColor();
		
		setPosition(sf::Vector2f{ static_cast<float>(x + m_shadowOffset), static_cast<float>(y + m_shadowOffset) });
		setFillColor(sf::Color(0, 0, 0, col.a));
		setOutlineColor(sf::Color(0, 0, 0, 0));

		sApplication->canvas().draw(*this);
		setFillColor(col);
		setOutlineColor(borderColor);
	}

	setPosition(sf::Vector2f{ static_cast<float>(x), static_cast<float>(y) });
	sApplication->canvas().draw(*this);
}

void Text::setColorIfNot(const sf::Color colorToBe, const sf::Color outlineColor)
{
	if (getFillColor() != colorToBe)
		setFillColor(colorToBe);

	if (getOutlineColor() != outlineColor)
		setOutlineColor(outlineColor);
}

void Text::setOriginalColor(const sf::Color color)
{
	if (m_originalColor == color)
		return;

	m_originalColor = color;
	setFillColor(color);
}

void Text::restoreColor()
{
	if (getFillColor() != m_originalColor)
		setFillColor(m_originalColor);
}

void Text::setStringMaxWidth(string str, const int maxWidth, const bool backwards /*= false*/, const bool percise /*= false*/, const string extra /*= ""*/, const bool ellipses /*= false*/)
{
	if (maxWidth < 0)
		return;

	bool reduced = false;

	if (percise)
	{
		// Slow
		while (!str.empty() && getStringWidth(*getFont(), getCharacterSize(), getStyle() & sf::Text::Bold, str, percise) >= maxWidth)
		{
			if (backwards)
				str.erase(str.begin());
			else
				str.erase(str.end() - 1);

			reduced = true;
		}
	}
	else
	{
		int strWidth = 0;

		auto itr = m_widths.find(str);

		if (itr == m_widths.end())
		{
			strWidth = getStringWidth(*getFont(), getCharacterSize(), getStyle() & sf::Text::Bold, str, percise);
			m_widths[str] = strWidth;
		}
		else
		{
			strWidth = itr->second;
		}

		const float dividen = static_cast<float>(maxWidth) / static_cast<float>(strWidth);
		const size_t newMaxSize = size_t(static_cast<float>(str.size()) * dividen);

		if (newMaxSize == 0)
		{
			str.clear();
		}
		else if (newMaxSize < str.size())
		{
			if (backwards)
				str.erase(0, str.size() - newMaxSize);
			else
				str.erase(newMaxSize, str.size() - 1);

			reduced = true;
		}
	}

	str += extra;

	if (reduced && ellipses)
		str += "...";

	if (getString() != str)
		setString(str);
}

void Text::setOriginalString(const string& str)
{
	m_originalString = str;

	if (getString() != str)
		setString(str);
}

/*static*/
int Text::getStringWidth(const sf::Font& font, const int characterSize, const bool bold, const string& str, const bool real /*= false*/)
{
	if (real)
	{
		sf::Text text(str, font, characterSize);
		return static_cast<int>(text.getGlobalBounds().width);
	}

	float result = 0;

	for (int i = 0; i < static_cast<int>(str.size()); ++i)
	{
		const auto thisLetter = str[i];
		const auto& gly = font.getGlyph(thisLetter, characterSize, bold);
		result += gly.bounds.width;

		if (i < static_cast<int>(str.size()) - 1)
		{
			const auto nextLetter = str[i + 1];
			auto derp = font.getKerning(thisLetter, nextLetter, characterSize);
			result += derp;
		}
	}

	return static_cast<int>(ceil(result));
}

Text& Text::setStringf(const char* format, ...)
{
	va_list ap;
	char txt[4096];
	va_start(ap, format);
	vsnprintf(txt, 4096, format, ap);
	va_end(ap);
	setString(txt);
	return *this;
}

Text& Text::setOriginalStringf(const char* format, ...)
{
	va_list ap;
	char txt[4096];
	va_start(ap, format);
	vsnprintf(txt, 4096, format, ap);
	va_end(ap);
	setOriginalString(txt);
	return *this;
}