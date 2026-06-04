#pragma once

class AnchoredPosition
{
	public:
		AnchoredPosition(const sf::Vector2i& baseOffset, sf::Vector2i* positionToChange, sf::Vector2i* anchor = nullptr);
		virtual ~AnchoredPosition();

		void setOffset(const sf::Vector2i& offset) { m_baseOffset = offset; }
		void setAnchor(sf::Vector2i* pAnchor) { m_anchorPos = pAnchor; }

		const auto& getOffset() const { return m_baseOffset; }

	protected:
		void updateAnchoredPosition();

		sf::Vector2i m_baseOffset;
		sf::Vector2i* m_positionToChange;
		sf::Vector2i* m_anchorPos;
};

