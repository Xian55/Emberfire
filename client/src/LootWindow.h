#pragma once

#include "WorldPanel.h"

#include "..\Shared\ItemDefines.h"

class DraggableNode;
class GameIconList;

class LootWindow : public WorldPanel
{
	public:
		enum Interface
		{
			ScrollUp,
			ScrollDown,
			IconList,
			TakeAll
		};

	public:
		LootWindow(World& owner, const int id);
		virtual ~LootWindow();

		void reset();
		void setSourceGuid(const int guid) { m_sourceGuid = guid; }
		void addItem(const ItemDefines::ItemDefinition itemId, const int count);
		void addMoney(const int amount);

		int getSourceGuid() const { return m_sourceGuid; }

	private:
		void input() final;
		void render() final;

		int m_sourceGuid{ 0 };

		shared_ptr<Button> m_upButton;
		shared_ptr<Button> m_downButton;
		shared_ptr<Button> m_takeallButton;
		shared_ptr<GameIconList> m_gameIconList;
		unique_ptr<DraggableNode> m_dragNode;
};