#include "stdafx.h"
#include "CooldownPie.h"
#include "Application.h"

#include <SFML/Graphics.hpp>
#include <vector>

// Verbatim port of GameIcon::drawClock: builds a polygon fanning clockwise from the top-middle of the rect,
// then x-mirrors it within the rect so the dark wedge represents the time still remaining.
void drawCooldownPie(const sf::Vector2i& tl, const sf::Vector2i& br, const float degree, const sf::Color& col)
{
	std::vector<sf::Vector2f> points;

	const sf::Vector2f topLeftCorner(tl);
	const sf::Vector2f bottomRightCorner(br);

	// Center
	points.push_back(sf::Vector2f((topLeftCorner.x + bottomRightCorner.x) / 2, (topLeftCorner.y + bottomRightCorner.y) / 2));
	// Top middle
	points.push_back(sf::Vector2f((topLeftCorner.x + bottomRightCorner.x) / 2, topLeftCorner.y));

	if (degree >= 45)
	{
		points.push_back(sf::Vector2f(bottomRightCorner.x, topLeftCorner.y));   // Top right

		if (degree >= 135)
		{
			points.push_back(sf::Vector2f(bottomRightCorner.x, bottomRightCorner.y));   // Bottom right

			if (degree >= 225)
			{
				points.push_back(sf::Vector2f(topLeftCorner.x, bottomRightCorner.y));   // Bottom left

				if (degree >= 315)
				{
					points.push_back(sf::Vector2f(topLeftCorner.x, topLeftCorner.y));   // Top left

					if (degree > 315)
					{
						const float topMiddleX = (topLeftCorner.x + bottomRightCorner.x) / 2;
						const float remaining = 360 - degree;
						const float remainingPct = 1.f - (remaining / 45.f);
						const float fullWidth = topMiddleX - topLeftCorner.x;
						const float addedWidth = fullWidth * remainingPct;
						points.push_back(sf::Vector2f(topLeftCorner.x + addedWidth, topLeftCorner.y));
					}
				}
				else
				{
					const float remaining = 315 - degree;
					const float remainingPct = 1.f - (remaining / 90.f);
					const float fullHeight = bottomRightCorner.y - topLeftCorner.y;
					const float subtractedHeight = fullHeight * remainingPct;
					points.push_back(sf::Vector2f(topLeftCorner.x, bottomRightCorner.y - subtractedHeight));
				}
			}
			else
			{
				const float remaining = 225 - degree;
				const float remainingPct = 1.f - float(remaining) / 90.f;
				const float fullWidth = bottomRightCorner.x - topLeftCorner.x;
				const float subtractedWidth = fullWidth * remainingPct;
				points.push_back(sf::Vector2f(bottomRightCorner.x - subtractedWidth, bottomRightCorner.y));
			}
		}
		else
		{
			const float remaining = 135 - degree;
			const float remainingPct = 1.f - (remaining / 90.f);
			const float fullHeight = bottomRightCorner.y - topLeftCorner.y;
			const float addedHeight = fullHeight * remainingPct;
			points.push_back(sf::Vector2f(bottomRightCorner.x, topLeftCorner.y + addedHeight));
		}
	}
	else
	{
		const float topMiddleX = (topLeftCorner.x + bottomRightCorner.x) / 2;
		const float remaining = 45 - degree;
		const float remainingPct = 1.f - (float(remaining) / 45.f);
		const float fullWidth = bottomRightCorner.x - topMiddleX;
		const float addedWidth = fullWidth * remainingPct;
		points.push_back(sf::Vector2f(topMiddleX + addedWidth, topLeftCorner.y));
	}

	sf::ConvexShape convex;
	convex.setPointCount(points.size());
	convex.setFillColor(col);

	const float width = bottomRightCorner.x - topLeftCorner.x;
	for (size_t i = 0; i < points.size(); ++i)
	{
		sf::Vector2f relativepos = points[i] - topLeftCorner;
		relativepos.x = width - relativepos.x + topLeftCorner.x;
		relativepos.y = points[i].y;
		convex.setPoint(i, relativepos);
	}

	sApplication->canvas().draw(convex);
}
