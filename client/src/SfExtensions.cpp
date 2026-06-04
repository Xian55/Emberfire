#include "stdafx.h"
#include "SfExtensions.h"
#include "Application.h"
#include "ContentMgr.h"

#include "..\Shared\GameMap.h"

void SfSoundSafe::setScreenPosition(const sf::Vector2f& screenPos)
{
	auto centerOfScreen = sf::Vector2f(sApplication->centerOfScreen());

	const float maxDistDown = sApplication->sWf() - centerOfScreen.y;
	const float maxDistRight = sApplication->sHf() - centerOfScreen.x;

	float distDown = screenPos.y - centerOfScreen.y;
	float distRight = screenPos.x - centerOfScreen.x;

	float distDownPct = distDown / maxDistDown;
	float distRightPct = distRight / maxDistRight;

	if (abs(distDown) < GameMap::BaseCellHeight && abs(distRight) < GameMap::BaseCellWidth)
		setPosition({ 0.f, 0.f, 0.f });
	else
		setPosition({ distRightPct, distDownPct, 0.f });
}