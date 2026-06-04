#include "stdafx.h"
#include "AnchoredPosition.h"

AnchoredPosition::AnchoredPosition(const sf::Vector2i& baseOffset, sf::Vector2i* positionToChange, sf::Vector2i* anchor /*= nullptr*/) :
	m_anchorPos(anchor),
	m_baseOffset(baseOffset),
	m_positionToChange(positionToChange)
{

}

AnchoredPosition::~AnchoredPosition()
{

}

void AnchoredPosition::updateAnchoredPosition()
{
	if (m_positionToChange == nullptr || m_anchorPos == nullptr)
		return;

	m_positionToChange->x = m_anchorPos->x + m_baseOffset.x;
	m_positionToChange->y = m_anchorPos->y + m_baseOffset.y;
}