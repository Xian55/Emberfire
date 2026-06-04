#pragma once

class TextBox;

struct ScrollingMsg
{			
	sf::Color originalColor;
	float alphaPct{0};
	shared_ptr<TextBox> txt;
};