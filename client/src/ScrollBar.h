#pragma once

#include "RenderObjectHolder.h"

#include <assert.h>

class Button;
class DraggableNode;

class ScrollBar : public RenderObjectHolder
{
	public:
		enum ScrollBarType
		{
			ScrollTopDown,
			ScrollBottomUp,
			ScrollLeftRight
		};

		enum Interface
		{
			ScrollUpButton,
			ScrollDownButton,
			ScrollSquare
		};

	public:
		ScrollBar(RenderObject& owner, const string& scrollUpButton, const string& scrollDownButton, const ScrollBarType scrollType, const string& scrollSquare = "", const int id = 0);
		virtual ~ScrollBar();

		virtual void input() override;
		virtual void render() override;

		void setAllowMousewheelOnlyMousedOver(const bool v) { m_mouseWheelMouseoverOnly = v; }
		void setAllowMousewheel(const bool v) { m_allowMouseWheel = v; }
		void setMaxOffset(const int offset) { m_maxOffset = offset; }
		void setScrollOffset(const int offset);
		void changeHeight(const int height);

		bool isGrabbed() const;
		bool isBottomUp() const { return m_type == ScrollBottomUp; }

		int getScrollOffset() const { return m_scrollOffset; }
		int getMaxOffset() const { return m_maxOffset; }

		// 0.0f to 1.0f result
		float getScrollPercent() const;

		shared_ptr<Button> getScrollUpButton() const { return m_scrollUpButton; }
		shared_ptr<Button> getScrollDownButton() const { return m_scrollDownButton; }
		shared_ptr<Button> getScrollSquare() const { return m_scrollSquare; }
		
	private:
		void updateScrollBarPosition();

		int m_height;
		int m_scrollOffset;
		int m_maxOffset;

		bool m_allowMouseWheel;
		bool m_mouseWheelMouseoverOnly;

		shared_ptr<Button> m_scrollUpButton;
		shared_ptr<Button> m_scrollDownButton;
		shared_ptr<Button> m_scrollSquare;

		shared_ptr<DraggableNode> m_dragNode;

		const ScrollBarType m_type;
};

