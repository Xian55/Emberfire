#pragma once

#include "RenderObject.h"

class DraggableNode;
class Sprite;

class ExpandableWindow : public RenderObject
{
	public:
		ExpandableWindow(RenderObject& owner, const int id,
			const string& textureTopRight,
			const string& textureTopAcross,
			const string& textureTopLeft,
			const string& textureBottomRight,
			const string& textureBottomAcross,
			const string& textureBottomLeft,
			const string& textureLeftUp,
			const string& textureRightUp,
			const string& textureCenter,
			const bool bDynamic = true);
		virtual ~ExpandableWindow();

		void setKeepInScreen(const bool v) { m_keepInScreen = v; }

		// The space between one corner and the other, that's why we multiply width/height by 2
		int getCenterWidth() const;
		int getCenterHeight() const;
		
	protected:
		virtual void input() override;
		virtual void render() override;

		sf::Vector2i m_topRightCorner;

	private:
		shared_ptr<Sprite> m_spriteTopRight;
		shared_ptr<Sprite> m_spriteTopAcross;
		shared_ptr<Sprite> m_spriteTopLeft;
		shared_ptr<Sprite> m_spriteBottomRight;
		shared_ptr<Sprite> m_spriteBottomAcross;
		shared_ptr<Sprite> m_spriteBottomLeft;
		shared_ptr<Sprite> m_spriteLeftUp;
		shared_ptr<Sprite> m_spriteRightUp;
		shared_ptr<Sprite> m_spriteCenter;

		unique_ptr<DraggableNode> m_dragNode;

		bool m_keepInScreen;
};

