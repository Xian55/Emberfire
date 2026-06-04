#pragma once

#include "ExpandableWindow.h"

class TextLines;
class ScrollBar;

class ContextMenu : public ExpandableWindow
{
	enum Define
	{
		CtxWidth = 250,
		CtxHeight = 350,
		CtxTextOffset = 16
	};

	public:
		ContextMenu(RenderObjectHolder& owner, const int id, const int reportToId, sf::Vector2i topLeftCorner);
		virtual ~ContextMenu();

		void setMaxLines(const int val);
		void addLine(const string& str, const sf::Color color = sf::Color::White);

		bool popActivated() final { return popResult(m_wasActivated); }
		bool popRightClicked() final { return popResult(m_wasActivated); }
		bool popKeyBound() final { return popResult(m_wasActivated); }

		int getReportToId() const { return m_reportToId; }

		void resetActivated() final { m_wasActivated = true; }

	protected:
		void input() final;
		void render() final;

	private:
		bool popResult(bool& v);

		int m_height;
		int m_width;
		int m_reportToId;

		bool m_wasActivated;

		shared_ptr<ScrollBar> m_scrollBar;
		unique_ptr<TextLines> m_lines;
};

