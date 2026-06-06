#pragma once

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Color.hpp>

// Draw a WoW-style radial "cooldown" pie inside the rect [topLeft, bottomRight], darkening the REMAINING
// portion clockwise from top-middle. `degree` is the remaining angle (0..360). Shared by the C++ game icons
// (GameIcon::drawClock) and the Lua aura widget (LuaCooldown). Renders to sApplication->canvas().
void drawCooldownPie(const sf::Vector2i& topLeft, const sf::Vector2i& bottomRight, float degree, const sf::Color& col);
