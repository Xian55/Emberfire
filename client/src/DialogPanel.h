#pragma once

#include "WorldPanel.h"

class DraggableNode;		
class TextBoxRo;

class DialogPanel : public WorldPanel
{
	public:
		virtual ~DialogPanel();
		
		void resetPosition() final;
		void refreshDockPosition(int xpos = -1);

	protected:
		DialogPanel(World& owner, const int id, const string& background, const sf::Vector2i& closeButtonOffset);

		virtual void input() override;
		virtual void render() override;

	private:
		int getDefualtXPos() const;

		int m_lastWorldPanelXpos{0};
		bool m_docked{false};
		
		vector<string> m_scrollTextCache;
		unique_ptr<DraggableNode> m_dragNode;
};

