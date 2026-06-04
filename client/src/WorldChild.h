#pragma once

#include "RenderObjectHolder.h"

class GameIcon;
class RenderObject;
class World;

class WorldChild : public RenderObjectHolder
{
	public:
		auto& world() const { return m_world; }

		bool grabIcon();

	protected:
		WorldChild(RenderObjectHolder& owner, const int id, World& world);
		virtual ~WorldChild();
		
		virtual bool overrideIconGrab(shared_ptr<GameIcon> myIcon) { return false; }

		bool depositGrabbedIconToEmptySpace() const;

	private:
		virtual void givenGabbedIcon(shared_ptr<GameIcon> myIcon, shared_ptr<GameIcon> heldIcon) {}

		World& m_world;
};

