#pragma once

#include "WorldChild.h"

class Sprite;
class Text;

class ToolbarXp : public WorldChild
{
	public:
		enum Interface
		{

		};

	public:
		ToolbarXp(World& owner, const int id);
		virtual ~ToolbarXp();

	private:
		void input() final;
		void render() final;

		shared_ptr<Sprite> m_sprite;
		unique_ptr<Text> m_text;
};

