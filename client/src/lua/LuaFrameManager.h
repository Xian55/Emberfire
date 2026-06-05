#pragma once

#include "RenderObjectHolder.h"

#include <map>

class Sprite;
class Text;

// ---- Lua-created widget adapters (each is a RenderObject in the existing holder tree) ----

// A top-level (or nested) frame: an invisible positioned container its child regions anchor to.
class LuaFrame : public RenderObjectHolder
{
	public:
		LuaFrame(RenderObject& owner, const int id);

		void setLogicalSize(const int w, const int h);
		sf::Vector2i logicalSize() const { return { m_w, m_h }; }

	private:
		int m_w{0};
		int m_h{0};
		// input()/render() inherited from RenderObjectHolder (routes/draws children).
};

// A texture region: draws a Sprite at its anchored position (mirrors SpriteRo).
class LuaTexture : public RenderObject
{
	public:
		LuaTexture(RenderObject& owner, const int id);

		void setTexture(const std::string& textureName);
		void setScaleSize(const int w, const int h);
		sf::Vector2i naturalSize() const;

	private:
		void input() final {}
		void render() final;

		shared_ptr<Sprite> m_sprite;
		int m_w{0};   // 0 => natural size (no scale)
		int m_h{0};
};

// A font-string region: draws a line of Text at its anchored position.
class LuaFontString : public RenderObject
{
	public:
		LuaFontString(RenderObject& owner, const int id);

		void setText(const std::string& str);
		sf::Vector2i textSize() const;

	private:
		void input() final {}
		void render() final;

		unique_ptr<Text> m_text;
};

// A clickable region: detects a left click over its bounds, drained by the manager into OnClick.
class LuaButton : public RenderObject
{
	public:
		LuaButton(RenderObject& owner, const int id);

		void setTexture(const std::string& textureName);
		void setHitSize(const int w, const int h) { m_w = w; m_h = h; }
		sf::Vector2i hitSize() const { return { m_w, m_h }; }
		bool popClicked();

	private:
		void input() final;
		void render() final;

		shared_ptr<Sprite> m_sprite;
		int m_w{0};
		int m_h{0};
		bool m_clicked{false};
};

// ---- Manager: root holder for all Lua frames + the handle registry behind LuaUI:: ----
class LuaFrameManager : public RenderObjectHolder
{
	public:
		LuaFrameManager(RenderObject& owner, const int id);
		~LuaFrameManager();

		// LuaUI backend (return 0 / no-op on bad handles).
		int  createFrame(int parentHandle);
		int  createButton(int parentHandle);
		int  createTexture(int frameHandle);
		int  createFontString(int frameHandle);
		int  popClickedHandle();
		void setPoint(int handle, int point, int relHandle, int relPoint, float x, float y);
		void setSize(int handle, float w, float h);
		void setText(int handle, const std::string& text);
		void setTexture(int handle, const std::string& textureName);
		void show(int handle, bool shown);
		void clearAll();   // destroy every Lua frame + reset the handle registry
		bool valid(int handle) const { return lookup(handle) != nullptr; }

		RenderObject* lookup(int handle) const;

		static LuaFrameManager* instance() { return s_instance; }

	private:
		// Attach `child` under `parent` (anchored to parent's top-left + offset), register its handle.
		void attachChild(RenderObjectHolder& parent, shared_ptr<RenderObject> child, int handle);
		sf::Vector2i sizeOf(RenderObject* o) const;

		int m_nextHandle{100000};   // reserved Lua id range (no collision with World/Application ids)
		std::map<int, shared_ptr<RenderObject>> m_objects;

		static LuaFrameManager* s_instance;
};
