#pragma once

#include "WorldChild.h"

class Button;

class WorldPanel : public WorldChild
{
	enum Interface
	{
		Background = -100,
		CloseButton
	};

	public:
		WorldPanel(World& owner, const int id, const string& background, const sf::Vector2i& closeButtonOffset = { 240, 0 }, const string& closeButton = "panel_close");
		virtual ~WorldPanel();
		virtual void resetPosition() {}
		virtual void onClose() {}
		virtual void onOpen() {}
		virtual bool isFrontInsertPanel() const { return true; }

		bool independant() const { return m_independant; }

		int worldPanelYPos() const;

	protected:
		World& m_world;

		void input() override;
		void render() override;

		void setIndependant(const bool b) { m_independant = b; }
		void alterBackground(const string& textureName);

	private:
		bool m_independant{false};
		sf::Vector2i m_size;
};

