#pragma once

#include "..\Geo2d.h"

class Text;
class ClientObject;

struct CombatMsg
{
	sf::Vector2f startPosf;
	float sizeDest{0};
	float sizeRestDest{0};
	float size{0};
	float startSize{0};
	float widthBefore{0};
	float heightBefore{0};
	float fadeInTimer{0};
	float fadeOutTimer{0};
	float solidTimer{0};
	bool sizeSwitch{0};
	bool crit{0};
	bool moveUpward{true};
	bool movedSideways{0};
	int sourceGuid;
	shared_ptr<Text> txt;
	string msg;
	sf::Color textColor;
	Geo2d::Vector2 objStartPos;
	Geo2d::Vector2 originalCamera;
	bool floatSideways{0};
	float sidewaysSwitch{0};
	float decayRate{1.f};
	clock_t startDate{0};
	bool ignoreCamera{false};
};