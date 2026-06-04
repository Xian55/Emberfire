#include "stdafx.h"
#include "GameIcon.h"
#include "ContentMgr.h"
#include "Sprite.h"
#include "Tooltip.h"
#include "Application.h"
#include "World.h"
#include "Text.h"
#include "Keybinds.h"
#include "WorldChild.h"
#include "ClientPlayer.h"
#include "Equipment.h"
#include "TextBox.h"

#include "..\SqlConnector\QueryResult.h"
#include "..\Shared\PlayerDefines.h"
#include "..\StringHelpers.h"

#include <assert.h>
#include <sstream>
#include <locale>

GameIcon::GameIcon(RenderObject& owner, const int id, const string& frame, const int entry /*= 0*/) :
	Button(owner, frame, id),
	m_entry(entry)
{
	m_cooldownTxt = make_unique<Text>(sContentMgr->getFont("Cambria Regular.ttf"));
	m_cooldownTxt->setCharacterSize(15);
	m_cooldownTxt->setOutlineThickness(1.0f);
	m_cooldownTxt->setOriginalColor(sf::Color::Red);

	m_bindLabel = make_unique<Text>(sContentMgr->getFont("Helvetica 400.ttf"));
	m_bindLabel->setCharacterSize(12);
	m_bindLabel->setOutlineThickness(1.0f);

	m_stackLabel = make_unique<Text>(sContentMgr->getFont("Helvetica 400.ttf"));
	m_stackLabel->setCharacterSize(12);
	m_stackLabel->setOutlineThickness(1.0f);

	m_effectPoints.resize(SpellDefines::NumEffectIdx, { 0, 0 });
}

GameIcon::~GameIcon()
{

}

void GameIcon::input()
{
	__super::input();
}

void GameIcon::render()
{
	if (m_hideIfNull && getEntry() == 0)
		return;

	if (isMousedOver())
		popTooltipRefresh();

	if (m_icon != nullptr)
	{
		// Faded icon when grabbed
		if (m_grabbed)
			m_icon->setColor(sf::Color(255, 255, 255, 100));
		else
			m_icon->setColor(sf::Color().White);

		assignBottomLeftCorner();

		sf::Vector2f pos1(getTopLeftCornerRef());
		sf::Vector2f pos2(getBottomRightCornerRef());

		if (m_renderDispelType)
		{
			// Border around the icon
			sf::RectangleShape rectangle;
			rectangle.setPosition(sf::Vector2f(pos1.x - 1.25f, pos1.y - 1.25f));
			rectangle.setSize({ pos2.x - pos1.x + 2.5f, pos2.y - pos1.y + 2.5f });
			rectangle.setFillColor(m_renderDispelTypeColor);
			sApplication->canvas().draw(rectangle);
		}

		m_icon->renderStretch(pos1, pos2);

		if (m_darken)
		{
			// Darken -> over it in a bit of black
			sf::RectangleShape rectangle;
			rectangle.setPosition(pos1);
			rectangle.setSize({ pos2.x - pos1.x, pos2.y - pos1.y });
			rectangle.setFillColor(sf::Color(0, 0, 0, 150));
			sApplication->canvas().draw(rectangle);
		}

		if (m_oom)
		{
			// Out of mana -> blue cover
			sf::RectangleShape rectangle;
			rectangle.setPosition(pos1);
			rectangle.setSize({ pos2.x - pos1.x, pos2.y - pos1.y });
			rectangle.setFillColor(sf::Color(0, 0, 255, 65));
			sApplication->canvas().draw(rectangle);
		}

		if (m_red)
		{
			// Error -> red cover
			sf::RectangleShape rectangle;
			rectangle.setPosition(pos1);
			rectangle.setSize({ pos2.x - pos1.x, pos2.y - pos1.y });
			rectangle.setFillColor(sf::Color(125, 0, 0, 65));
			sApplication->canvas().draw(rectangle);
		}

		if (m_cachedActivatedByOut > 0 || m_cachedActivatedByIn > 0)
		{			
			bool darken = false;

			if (auto world = getWorld())
			{
				if (auto myself = world->myself())
				{
					if (m_cachedActivatedByOut > 0 && !(myself->getVariable(ObjDefines::Variable::ReactionMask_Out) & m_cachedActivatedByOut))
						darken = true;

					if (m_cachedActivatedByIn > 0 && !(myself->getVariable(ObjDefines::Variable::ReactionMask_In) & m_cachedActivatedByIn))
						darken = true;
				}
			}

			if (darken)
			{				
				sf::RectangleShape rectangle;
				rectangle.setPosition(pos1);
				rectangle.setSize({ pos2.x - pos1.x, pos2.y - pos1.y });
				rectangle.setFillColor(sf::Color(0, 0, 0, 150));
				sApplication->canvas().draw(rectangle);
			}
		}

		if (m_durationExpireDate != 0)
			drawCountdown();

		if (m_cooldownExpireDate != 0)
			drawCooldown();

		if (m_cooldownFlash > 0.f)
			drawCooldownFlash();

		// Last on purpose, since duration timer might change the tooltip
		updateTooltipPosition();
	}

	// The button portion of this child class is the gloss visual, but it has no visual if the button isn't at least moused over
	if (MouseableNode::isMousedOver(true) || isHighlight() || getKeyEvent().code != sf::Keyboard::Unknown)
	{
		__super::render();
	}
	else
	{
		__super::assignBottomLeftCorner();
		__super::pumpExclaimNotice();
		__super::updateDragLogic();
	}

	drawLabels();
}

bool GameIcon::setLevel(const int level)
{ 
	bool result = m_spellLevel != level;
	m_spellLevel = level; 
	return result;
}

bool GameIcon::setBasePoints(const vector<pair<int16_t, int16_t>>& vec)
{
	bool result = false;

	if (vec.size() == m_effectPoints.size())
	{
		result = m_effectPoints != vec;
		m_effectPoints = vec;
	}

	return result;
}

bool GameIcon::setBasePoints(const int effIdx, const int minp, const int maxp)
{
	bool result = false;

	if (effIdx >= 0 && effIdx < SpellDefines::NumEffectIdx)
	{
		result = m_effectPoints[effIdx] != pair<int16_t, int16_t>{ minp, maxp };
		m_effectPoints[effIdx] = { minp, maxp };
	}

	return result;
}

void GameIcon::drawLabels() /*final*/
{
	if (getKeyEvent().code != sf::Keyboard::Unknown)
	{
		const string labelStr = sKeybinds->deduceKeybindStr(getKeyEvent());

		if (m_bindLabel->getString() != labelStr)
			m_bindLabel->setString(labelStr);

		m_bindLabel->setColorIfNot(m_bindColor);
		m_bindLabel->draw(getTopLeftCornerRef().x + 2, getTopLeftCornerRef().y);
	}

	if (m_stackAmount > m_minStackShowAmount && (getType() != GameIcon::Type::Item || getEntry() != ItemDefines::StaticItems::GoldItem))
	{
		const string labelStr = "(" + to_string(m_stackAmount) + ")";

		if (m_stackLabel->getString() != labelStr)
			m_stackLabel->setString(labelStr);

		int yPos = 0;

		if (m_stackRenderBottom)
			yPos = getTopLeftCornerRef().y + mouseableHeight() - int(m_stackLabel->getGlobalBounds().height) - 5;
		else
			yPos = getTopLeftCornerRef().y;

		m_stackLabel->draw(getBottomRightCornerRef().x - 2 - int(m_stackLabel->getGlobalBounds().width), yPos);
	}
	
	if (m_cooldownExpireDate != 0)
		drawCooldownText();
}

void GameIcon::cacheCasterLevel()
{
	if (auto world = getWorld())
	{
		if (auto myself = world->myself())
			m_casterLevel = myself->getLevel();
	}
}

void GameIcon::refreshTooltip()
{
	cacheCasterLevel();
	pickIcon();
	m_needTooltipRefresh = true;
}

void GameIcon::popTooltipRefresh()
{
	if (!m_needTooltipRefresh)
		return;

	m_needTooltipRefresh = false;	
	fillTooltip();
}
		
void GameIcon::setRenderDispelType(const bool v)
{
	m_renderDispelType = v;
	
	if (m_renderDispelType)
	{
		auto& sv = sContentMgr->db("spell_template");
		int dispelType = atoi(sv.data(getEntry(), "dispel").c_str());

		switch (dispelType)
		{
		case SpellDefines::DispelType::Magic:
			m_renderDispelTypeColor = sf::Color(87, 142, 197, 255);
			break;
		case SpellDefines::DispelType::Disease:
			m_renderDispelTypeColor = sf::Color(122, 83, 31, 255);
			break;
		case SpellDefines::DispelType::Curse:
			m_renderDispelTypeColor = sf::Color(130, 16, 204, 255);
			break;
		case SpellDefines::DispelType::Poison:
			m_renderDispelTypeColor = sf::Color(80, 135, 74, 255);
			break;
		case SpellDefines::DispelType::Physical:
			m_renderDispelTypeColor = sf::Color(115, 12, 26, 255);
			break;
		default:
			m_renderDispelTypeColor = sf::Color(15, 15, 15, 255);
			break;
		}
	}
}
	
void GameIcon::setCooldown(const __time64_t startDate, const __time64_t endDate)
{
	// We were animating a cooldown, but it finished
	if (m_cooldownExpireDate != 0 && endDate == 0)
		m_cooldownFlash = 1.f;

	m_cooldownStartDate = startDate;
	m_cooldownExpireDate = endDate;
}

void GameIcon::drawCooldownFlash()
{
	float brightenPct = 0.f;

	if (m_cooldownFlash > 0.5f)
		brightenPct = 1.f - ((m_cooldownFlash - 0.5f) / 0.5f);
	else
		brightenPct = m_cooldownFlash / 0.5f;

	brightenPct /= 2.f;

	sf::Vector2f pos1(getTopLeftCornerRef());
	sf::Vector2f pos2(getBottomRightCornerRef());
		
	sf::RectangleShape rectangle;
	rectangle.setPosition(pos1);
	rectangle.setSize({ pos2.x - pos1.x, pos2.y - pos1.y });
	rectangle.setFillColor(sf::Color(255, 255, 255, uint8_t(255.f * brightenPct)));
	sApplication->canvas().draw(rectangle, sf::BlendAdd);

	m_cooldownFlash -= sApplication->delta() * 3.f;	
}
		
void GameIcon::drawCountdown()
{
	// Update the tooltip every 100ms so that the countdown timer is visible
	if (MouseableNode::isMousedOver(true))
	{
		m_redrawTooltipTimer += sApplication->timeNowMs();

		if (m_redrawTooltipTimer >= 0.1f)
		{
			fillTooltip();
			m_redrawTooltipTimer = 0.f;
		}
	}

	__time64_t maxDuration = m_durationExpireDate - m_durationStartDate;
	__time64_t elapsedTime = sApplication->timeNowMs() - m_durationStartDate;

	float pct = float(elapsedTime) / float(maxDuration);
	float degree = 360.f * min(max(pct, 0.f), 1.f);

	drawClock(degree);
}

void GameIcon::drawCooldown()
{
	__time64_t maxDuration = m_cooldownExpireDate - m_cooldownStartDate;
	__time64_t elapsedTime = sApplication->timeNowMs() - m_cooldownStartDate;

	// Just draw the icon as normal if timer is complete
	if (elapsedTime > maxDuration)
		return;

	float pct = 1.f - (float(elapsedTime) / float(maxDuration));
	float degree = 360.f * min(max(pct, 0.f), 1.f);

	drawClock(degree);
}

void GameIcon::drawCooldownText()
{
	__time64_t maxDuration = m_cooldownExpireDate - m_cooldownStartDate;
	__time64_t elapsedTime = sApplication->timeNowMs() - m_cooldownStartDate;
	
	if (elapsedTime > maxDuration)
		return;

	__time64_t timeLeft = m_cooldownExpireDate - sApplication->timeNowMs();
	
	float numSec = float(timeLeft) / 1000.f;

	string durationStr = formTimeSuffix(static_cast<int>(timeLeft), { "d", "h", "m", "s" });
	m_cooldownTxt->setOriginalString(formTimePrefix(static_cast<int>(timeLeft)) + durationStr);
	m_cooldownTxt->draw(getTopLeftCornerRef().x + (mouseableWidth() / 2) - int(m_cooldownTxt->getGlobalBounds().width / 2.f), getTopLeftCornerRef().y + (mouseableHeight() / 2) - (m_cooldownTxt->getCharacterSize() / 2));
}

void GameIcon::drawClock(const float degree)
{
	vector<sf::Vector2f> points;
	
	sf::Vector2f topLeftCorner(getTopLeftCornerRef());
	sf::Vector2f bottomRightCorner(getBottomRightCornerRef());

	// Center
	points.push_back(sf::Vector2f((topLeftCorner.x + bottomRightCorner.x) / 2, (topLeftCorner.y + bottomRightCorner.y) / 2));

	// Top middle
	points.push_back(sf::Vector2f((topLeftCorner.x + bottomRightCorner.x) / 2, topLeftCorner.y));

	if (degree >= 45)
	{
		// Top right
		points.push_back(sf::Vector2f(bottomRightCorner.x, topLeftCorner.y));

		if (degree >= 135)
		{
			// Bottom right
			points.push_back(sf::Vector2f(bottomRightCorner.x, bottomRightCorner.y));

			if (degree >= 225)
			{
				// Bottom left
				points.push_back(sf::Vector2f(topLeftCorner.x, bottomRightCorner.y));
				
				if (degree >= 315)
				{
					// Top left
					points.push_back(sf::Vector2f(topLeftCorner.x, topLeftCorner.y));

					// Getting closer to the top middle again
					if (degree > 315)
					{
						float topMiddleX = (topLeftCorner.x + bottomRightCorner.x) / 2;

						float remaining = 360 - degree;
						float remainingPct = 1.f -  (remaining / 45.f);

						float fullWidth = topMiddleX - topLeftCorner.x;
						float addedWidth = fullWidth * remainingPct;

						points.push_back(sf::Vector2f(topLeftCorner.x + addedWidth, topLeftCorner.y));
					}
				}
				else
				{
					float bottomRightY = bottomRightCorner.y;

					float remaining = 315 - degree;
					float remainingPct = 1.f -  (remaining / 90.f);

					float fullHeight = bottomRightCorner.y - topLeftCorner.y;
					float subtractedHeight = fullHeight * remainingPct;

					points.push_back(sf::Vector2f(topLeftCorner.x, bottomRightCorner.y - subtractedHeight));
				}
			}
			else
			{
				float bottomRightX = bottomRightCorner.x;

				float remaining = 225 - degree;
				float remainingPct = 1.f -  float(remaining) / 90.f;

				float fullWidth = bottomRightCorner.x - topLeftCorner.x;
				float subtractedWidth = fullWidth * remainingPct;

				points.push_back(sf::Vector2f(bottomRightCorner.x - subtractedWidth, bottomRightCorner.y));
			}			
		}
		else
		{
			float topRightY = topLeftCorner.y;

			float remaining = 135 - degree;
			float remainingPct = 1.f -  (remaining / 90.f);

			float fullHeight = bottomRightCorner.y - topLeftCorner.y;
			float addedHeight = fullHeight * remainingPct;

			points.push_back(sf::Vector2f(bottomRightCorner.x, topLeftCorner.y + addedHeight));
		}		
	}
	else
	{
		float topMiddleX = (topLeftCorner.x + bottomRightCorner.x) / 2;

		float remaining = 45 - degree;
		float remainingPct = 1.f - (float(remaining) / 45.f);

		float fullWidth = bottomRightCorner.x - topMiddleX;
		float addedWidth = fullWidth * remainingPct;

		points.push_back(sf::Vector2f(topMiddleX + addedWidth, topLeftCorner.y));
	}	
	
	sf::ConvexShape convex;
	convex.setPointCount(points.size());
	convex.setFillColor(sf::Color(0, 0, 0, 150));

	for (size_t i = 0; i < points.size(); ++i)
	{
		auto& point = points[i];

		sf::Vector2f relativepos = point - topLeftCorner;
		
		relativepos.x = float(mouseableWidth()) - relativepos.x + topLeftCorner.x;
		relativepos.y = point.y;

		convex.setPoint(i, relativepos);
	}

	sApplication->canvas().draw(convex);
}

void GameIcon::updateTooltipPosition()
{
	auto tooltip = getTooltip();

	if (tooltip == nullptr)
		return;

	auto newPosition = getTopLeftCornerRef() + sf::Vector2i(mouseableWidth() + 16, 0);

	if (m_tooltipOffset.x == 0 && m_tooltipOffset.y == 0)
	{
		int maxY = sApplication->sH() - 50;
		int maxX = sApplication->sW() / 2;

		if (tooltip->getHeight() + tooltip->getTextOffset().y < maxY)
		{
			// Draw "upward" if we would have exceeded screen height
			if (newPosition.y + tooltip->getHeight() + tooltip->getTextOffset().y > maxY)
				newPosition.y -= tooltip->getHeight() + (tooltip->getTextOffset().y * 2);
		}

		if (tooltip->getWidth() + tooltip->getTextOffset().x < maxX)
		{
			// Draw in the opposite horizontal direction if we're on right side of the screen
			if (m_alwaysLeftSide ||
				(m_allowHorizontalTooltipAdjust && newPosition.x + tooltip->getWidth() + tooltip->getTextOffset().x > maxX))
			{
				newPosition.x -= tooltip->getWidth() + (tooltip->getTextOffset().x * 2) + static_cast<int>(mouseableWidth() * 1.5) + 5;
			}
		}
	}
	else
	{
		newPosition += m_tooltipOffset;
	}

	// In such a case, we cannot guarentee it will look normal when squished
	if (m_alwaysLeftSide || !m_allowHorizontalTooltipAdjust)
		tooltip->setKeepInScreen(false);

	tooltip->moveTo(newPosition);
}

void GameIcon::setExpirationTimer(const __time64_t startDate, const __time64_t endDate)
{	
	m_durationExpireDate = endDate;
	m_durationStartDate = startDate;
}

bool GameIcon::changeEntry(const int entry)
{
	if (m_entry == entry)
		return false;

	m_entry = entry;
	
	m_cooldownExpireDate = 0;
	m_cooldownStartDate = 0;
	m_durationExpireDate = 0;
	m_durationStartDate = 0;

	cacheCasterLevel();
	pickIcon();
	
	if (m_icon != nullptr)
	{
		if (m_icon->getTexture() == nullptr)
		{
			m_icon = sContentMgr->spawnSprite("reward-unknown.png");	
			ASSERT(m_icon->getTexture() != nullptr);
		}

		const_cast<sf::Texture*>(m_icon->getTexture())->setSmooth(true);
	}

	m_needTooltipRefresh = true;

	onEntryChange();
	return true;
}

void GameIcon::release(World& source)
{
	ASSERT(source.getGrabbedIcon().get() == this);
	m_grabbed = false;
}

bool GameIcon::grab(World& giveTo)
{
	if (m_grabbed)
		return false;

	if (giveTo.getGrabbedIcon() != nullptr)
		return false;

	m_grabbed = true;
	giveTo.setGrabbedIcon(shared_from_this());
	return true;
}

World* GameIcon::getWorld() const
{
	if (auto parent = dynamic_cast<WorldChild*>(getOwner()))
		return &parent->world();

	if (auto parent = getOwner())
	{
		if (auto superParent = dynamic_cast<WorldChild*>(parent->getOwner()))
			return &superParent->world();
	}

	return nullptr;
}
	
string GameIcon::formSpellDescription(const int spellId) const
{
	auto& sv = sContentMgr->db("spell_template");
	string descriptionStr = sv.data(spellId, "description");

	if (descriptionStr.empty())
		return descriptionStr;

	// Duration
	string durationStr = sv.data(spellId, "duration");
	string durationFormula = sv.data(spellId, "duration_formula");

	if (!durationFormula.empty())
	{
		parseFormula(durationStr, durationFormula);
		durationStr = durationFormula;
	}

	int durationMs = atoi(durationStr.c_str());
	
	if (durationMs > 0)
	{
		string durationStr = formTimeString(durationMs, { "day", "hour", "min", "sec" });
		Util::strReplaceAll(descriptionStr, "$DUR", durationStr);
	}
	
	// Effect points from server
	if (m_effectPoints[0] != pair{ int16_t(0), int16_t(0) })
	{
		Util::strReplaceAll(descriptionStr, "$E1min", to_string(abs(m_effectPoints[0].first)));
		Util::strReplaceAll(descriptionStr, "$E1max", to_string(abs(m_effectPoints[0].second)));
	}
	
	if (m_effectPoints[1] != pair{ int16_t(0), int16_t(0) })
	{
		Util::strReplaceAll(descriptionStr, "$E2min", to_string(abs(m_effectPoints[1].first)));
		Util::strReplaceAll(descriptionStr, "$E2max", to_string(abs(m_effectPoints[1].second)));
	}
	
	if (m_effectPoints[2] != pair{ int16_t(0), int16_t(0) })
	{
		Util::strReplaceAll(descriptionStr, "$E3min", to_string(abs(m_effectPoints[2].first)));
		Util::strReplaceAll(descriptionStr, "$E3max", to_string(abs(m_effectPoints[2].second)));
	}

	// data1-3
	string effect1_data1 = sv.data(spellId, "effect1_data1");
	string effect1_data2 = sv.data(spellId, "effect1_data2");
	string effect1_data3 = sv.data(spellId, "effect1_data3");
	string effect2_data1 = sv.data(spellId, "effect2_data1");
	string effect2_data2 = sv.data(spellId, "effect2_data2");
	string effect2_data3 = sv.data(spellId, "effect2_data3");
	string effect3_data1 = sv.data(spellId, "effect3_data1");
	string effect3_data2 = sv.data(spellId, "effect3_data2");
	string effect3_data3 = sv.data(spellId, "effect3_data3");

	if (!effect1_data1.empty())
		Util::strReplaceAll(descriptionStr, "$E1D1", to_string(abs(atoi(effect1_data1.c_str()))));
			
	if (!effect1_data2.empty())
		Util::strReplaceAll(descriptionStr, "$E1D2", to_string(abs(atoi(effect1_data2.c_str()))));
	
	if (!effect1_data3.empty())
		Util::strReplaceAll(descriptionStr, "$E1D3", to_string(abs(atoi(effect1_data3.c_str()))));
	
	if (!effect2_data1.empty())
		Util::strReplaceAll(descriptionStr, "$E2D1", to_string(abs(atoi(effect2_data1.c_str()))));

	if (!effect2_data2.empty())
		Util::strReplaceAll(descriptionStr, "$E2D2", to_string(abs(atoi(effect2_data2.c_str()))));

	if (!effect2_data3.empty())
		Util::strReplaceAll(descriptionStr, "$E2D3", to_string(abs(atoi(effect2_data3.c_str()))));

	if (!effect3_data1.empty())
		Util::strReplaceAll(descriptionStr, "$E3D1", to_string(abs(atoi(effect3_data1.c_str()))));

	if (!effect3_data2.empty())
		Util::strReplaceAll(descriptionStr, "$E3D2", to_string(abs(atoi(effect3_data2.c_str()))));

	if (!effect3_data3.empty())
		Util::strReplaceAll(descriptionStr, "$E3D3", to_string(abs(atoi(effect3_data3.c_str()))));

	// Interval
	int intervalMs = atoi(sv.data(spellId, "interval").c_str());
	
	if (intervalMs > 0)
	{
		string durationStr = formTimeString(intervalMs, { "day", "hour", "min", "sec" });
		Util::strReplaceAll(descriptionStr, "$INVL", durationStr);
	}

	return descriptionStr;
}
	
string GameIcon::formTimeSuffix(int timeMs, const vector<string>& dictionary) const
{
	ASSERT(dictionary.size() == 4);
	
	// Round up
	timeMs += 500;

	if (timeMs >= 86400000)	
		return dictionary[0];	
	else if (timeMs >= 3600000)	
		return dictionary[1];	
	else if (timeMs >= 60000)	
		return dictionary[2];		

	return dictionary[3];	
}
	
string GameIcon::formTimePrefix(int timeMs) const
{
	// Round up
	timeMs += 500;

	if (timeMs >= 86400000)	
	{
		timeMs += 86400000 / 2;
		return to_string(timeMs / 86400000);	
	}
	else if (timeMs >= 3600000)	
	{
		timeMs += 3600000 / 2;
		return to_string(timeMs / 3600000);	
	}
	else if (timeMs >= 60000)	
	{
		timeMs += 60000 / 2;
		return to_string(timeMs / 60000);
	}
			
	return to_string(timeMs / 1000);
}

string GameIcon::formTimeString(int timeMs, const vector<string>& dictionary, const bool allowFloat) const
{
	ASSERT(dictionary.size() == 4);

	string durationStr;

	// Round up
	if (!allowFloat)
		timeMs += 500;

	if (timeMs >= 86400000)
	{
		if (timeMs % 86400000 == 0 || !allowFloat)
			durationStr = Util::fmtStr("%d %s", timeMs / 86400000, dictionary[0].c_str());
		else
			durationStr = Util::fmtStr("%.2f %s", float(timeMs) / 86400000.f, dictionary[0].c_str());
	}
	else if (timeMs >= 3600000)
	{
		if (timeMs % 3600000 == 0 || !allowFloat)
			durationStr = Util::fmtStr("%d %s", timeMs / 3600000, dictionary[1].c_str());
		else
			durationStr = Util::fmtStr("%.2f %s", float(timeMs) / 3600000.f, dictionary[1].c_str());
	}
	else if (timeMs >= 60000)
	{
		if (timeMs % 60000 == 0 || !allowFloat)
			durationStr = Util::fmtStr("%d %s", timeMs / 60000, dictionary[2].c_str());
		else
			durationStr = Util::fmtStr("%.2f %s", float(timeMs) / 60000.f, dictionary[2].c_str());
	}
	else
	{
		if (timeMs % 1000 == 0 || !allowFloat)
			durationStr = Util::fmtStr("%d %s", timeMs / 1000, dictionary[3].c_str());
		else
			durationStr = Util::fmtStr("%.2f %s", float(timeMs) / 1000.f, dictionary[3].c_str());
	}

	return durationStr;
}
		
void GameIcon::parseFormula(const string& baseValue, string& inputOutput) const
{
	Util::strReplaceAll(inputOutput, "value", baseValue);
	Util::strReplaceAll(inputOutput, "clvl", to_string(m_casterLevel));
	Util::strReplaceAll(inputOutput, "splvl", to_string(m_spellLevel));	
	inputOutput = to_string(Util::parseIntExpression(inputOutput));
}