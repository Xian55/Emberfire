#pragma once

#include "..\Geo2d.h"

class MouseableNode;

class DraggableNode
{
	public:
		DraggableNode();
		virtual ~DraggableNode();

		void updateDraggableObject(MouseableNode& self, const bool wholeObject = false);
		void clearAnchors();
		void setLoneAnchor(sf::Vector2i* anchor);
		void setLoneAnchor(Geo2d::Vector2* anchor);

		void setDragNodeHeight(const int val) { m_dragNodeHeight = val; }
		void setReverse(const bool bParam) { m_reverse = bParam; }
		void addAnchor(sf::Vector2i* anchor) { m_anchors.insert(anchor); }
		void addAnchor(Geo2d::Vector2* anchor) { m_anchorsGeo2d.insert(anchor); }
		void setMouseDragButton(const sf::Mouse::Button b) { m_dragButton = b; }
		void cancelGrab() { m_isGrabbed = false; }
		void setDragSpeed(const float ratio) { m_dragSpeed = ratio; }

		bool isGrabbed() const { return m_isGrabbed; }

	private:
		bool m_reverse;
		bool m_isGrabbed;

		int m_dragNodeHeight;

		float m_dragSpeed{1.f};

		sf::Mouse::Button m_dragButton;
		sf::Vector2i m_lastGrabPos;
		set<sf::Vector2i*> m_anchors;
		set<Geo2d::Vector2*> m_anchorsGeo2d;
};

