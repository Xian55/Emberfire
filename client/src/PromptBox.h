#pragma once

#include "TextLines.h"
#include "Text.h"

class Button;
class ScrollBar;

class PromptBox : public TextLines
{
	enum Defines
	{
		MaxPromptString = 128,
		MaxPrefixString = 128,
	};

	public:
		PromptBox(RenderObject& owner, const int id, const string& fontName, shared_ptr<Button> enterButton, const sf::Vector2i& renderPos, const int maxWidth, const sf::Color fontColor, const bool stringBeyondRender = true);
		PromptBox(RenderObject& owner, const int id, const Assets::FontId fontId, shared_ptr<Button> enterButton, const sf::Vector2i& renderPos, const int maxWidth, const sf::Color fontColor, const bool stringBeyondRender = true)
			: PromptBox(owner, id, Assets::fontFile(fontId), enterButton, renderPos, maxWidth, fontColor, stringBeyondRender) {}
		virtual ~PromptBox();

		void setMaxPromptString(const int val) { m_maxPromptStringLen = val; }
		void setColor(const sf::Color color) { m_fontColor = color; }
		void setMaxWidth(const int val);
		void setContent(const string& str);
		void setAllowStringBeyondRender(const bool bParam) { m_stringBeyondRender = bParam; }
		void setSafetyRender(const bool bParam) { m_safetyRender = bParam; }
		void setPromptCharacterSize(const int characterSize);
		void setPromptMaxStrLen(const int len);
		void setAsciiOnly(const bool v) { m_asciiOnly = v; }
		void setNumbersOnly(const bool v) { m_numbersOnly = v; }
		void setAllowZero(const bool v) { m_allowZero = v; }
		void setPrefix(const string& str);
		void setTextPrompt(const string& str);

		bool isCurrentPrompt() const;
		bool validLetter(const char letter) const;
		bool popEmptyEnter();

		// If this prompt box has a button associated with it then when the enter button is pressed, you get m_queuedStatement
		//		Returns an empty string if is none
		string popQueuedStatement();

		const string& getContent() const { return m_textPrompt; }

	protected:
		void input() final;
		void render() final;

	private:
		// This will be the source material to build a render string from
		string decideStringToRender() const;

		bool m_forcedDrawUpdateOnce;

		// If numbers are being used, 0 is blocked unless this is true
		bool m_allowZero{false};

		// If we hit enter, yet there was nothing input
		bool m_emptyEnter;

		// If true then we only add characters that isalpha(letter) != 0
		bool m_asciiOnly;

		// If true then we only add characters that are integers
		bool m_numbersOnly;

		// Toggles whether or not m_textPrompt can contain letters that exceed the render width
		bool m_stringBeyondRender;

		// If true then will render the text like "*******"
		bool m_safetyRender;

		int m_maxWidth;
		sf::Color m_fontColor;

		float m_promptTimer;

		size_t m_maxPromptStringLen;

		Text m_sfTextPrompt;

		string m_prefix;
		string m_textPrompt;

		// Temporary prefix that will be cleared when rendering is done. Constantly supply this with a value for it to also be rendered.
		string m_textPromptPrefix;

		// When the enter button is pressed, this is filled for the purpose of something else using it
		string m_queuedStatement;

		shared_ptr<Button> m_enterButton;
};

