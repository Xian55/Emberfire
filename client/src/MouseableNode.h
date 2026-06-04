#pragma once

class Application;

class MouseableNode
{
	public:
		MouseableNode();
		virtual ~MouseableNode();

		auto& getTopLeftCornerRef() { return m_topLeftCorner; }
		auto& getBottomRightCornerRef() { return m_bottomRightCorner; }

		auto mouseableWidth() const { return abs(m_bottomRightCorner.x - m_topLeftCorner.x); }
		auto mouseableHeight() const { return abs(m_topLeftCorner.y - m_bottomRightCorner.y); }

		void updateTopLeftCorner(const sf::Vector2i pos) { m_topLeftCorner = pos; }
		void updateBottomRightCorner(const sf::Vector2i pos) { m_bottomRightCorner = pos; }

		void setPos(const sf::Vector2i pos)
		{
			m_topLeftCorner = pos;
			m_bottomRightCorner.x = pos.x + mouseableWidth();
			m_bottomRightCorner.y = pos.y + mouseableHeight();
		}

		virtual bool isMousedOver(const bool useRealPosition = false);

	protected:
		bool m_bMouseOverUsesActualMousePos;

		sf::Vector2i m_topLeftCorner;
		sf::Vector2i m_bottomRightCorner;
};

