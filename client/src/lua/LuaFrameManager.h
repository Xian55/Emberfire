#pragma once

#include "RenderObjectHolder.h"

#include <map>

class Sprite;
class Text;
class PromptBox;

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
		void setColor(const int r, const int g, const int b, const int a);   // tint + alpha
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
		void setFontSize(const int n);
		void setColor(const int r, const int g, const int b, const int a);
		void setFont(const std::string& fontName);   // "Ringbearer", "Palatino", ...
		sf::Vector2i textSize() const;

	private:
		void input() final {}
		void render() final;

		unique_ptr<Text> m_text;
};

// A clickable frame: holds child regions (like LuaFrame) AND detects a left click over its
// bounds, drained by the manager into OnClick. WoW Buttons are frames, so this is a holder —
// otherwise its child textures/bars/fontstrings would never be created or drawn.
class LuaButton : public RenderObjectHolder
{
	public:
		LuaButton(RenderObject& owner, const int id);

		void setTexture(const std::string& textureName);
		void setHoverTexture(const std::string& textureName);
		void setHitSize(const int w, const int h) { m_w = w; m_h = h; }
		sf::Vector2i hitSize() const { return { m_w, m_h }; }
		bool popClicked();

	private:
		void input() final;
		void render() final;

		shared_ptr<Sprite> m_sprite;
		shared_ptr<Sprite> m_hoverSprite;
		int m_w{0};
		int m_h{0};
		bool m_clicked{false};
};

// A status/progress bar: a fill sprite scaled horizontally by (value-min)/(max-min).
class LuaStatusBar : public RenderObject
{
	public:
		LuaStatusBar(RenderObject& owner, const int id);

		void setTexture(const std::string& textureName);
		void setBarSize(const int w, const int h);
		void setMinMax(const float mn, const float mx);
		void setValue(const float v);
		void setColor(const int r, const int g, const int b, const int a);
		sf::Vector2i barSize() const { return { m_w, m_h }; }

	private:
		void input() final {}
		void render() final;

		shared_ptr<Sprite> m_fill;
		int   m_w{0};
		int   m_h{0};
		float m_min{0.f};
		float m_max{1.f};
		float m_value{0.f};
};

// A single-line text input: wraps a C++ PromptBox child. Click focuses it (setCurrentPrompt),
// type to edit, Enter submits (drained into OnEnter). Password masking + max length supported.
class LuaEditBox : public RenderObjectHolder
{
	public:
		LuaEditBox(RenderObject& owner, const int id);
		~LuaEditBox();

		void setText(const std::string& str);
		std::string getText() const;
		void setMaxLetters(const int n);
		void setPassword(const bool masked);
		void setNumeric(const bool v);
		void setFontSize(const int n);
		void setTextColor(const int r, const int g, const int b, const int a);
		void setBoxSize(const int w, const int h);
		sf::Vector2i boxSize() const { return { m_w, m_h }; }
		bool popSubmitted();

	private:
		void input() final;
		void render() final;

		shared_ptr<PromptBox> m_prompt;
		sf::Color m_textColor{ 220, 194, 171, 255 };   // base text color; brightened on hover
		int m_w{0};
		int m_h{0};
		bool m_submitted{false};
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
		int  createStatusBar(int parentHandle);
		int  createTexture(int frameHandle);
		int  createFontString(int frameHandle);
		int  createEditBox(int parentHandle);
		int  popClickedHandle();
		int  popSubmittedHandle();
		std::string getText(int handle) const;
		void setEditBoxPassword(int handle, bool masked);
		void setEditBoxMaxLen(int handle, int n);
		void setEditBoxNumeric(int handle, bool v);
		void setEditBoxFontSize(int handle, int n);
		void setEditBoxColor(int handle, int r, int g, int b, int a);
		void setVertexColor(int handle, int r, int g, int b, int a);   // texture tint + alpha
		void setFont(int handle, const std::string& fontName);          // fontstring font face
		void setMinMax(int handle, float mn, float mx);
		void setValue(int handle, float v);
		void setColor(int handle, int r, int g, int b, int a);
		void setPoint(int handle, int point, int relHandle, int relPoint, float x, float y);
		void setSize(int handle, float w, float h);
		void setText(int handle, const std::string& text);
		void setTexture(int handle, const std::string& textureName);
		void setHoverTexture(int handle, const std::string& textureName);
		void show(int handle, bool shown);
		void clearAll();   // destroy every Lua frame + reset the handle registry
		bool valid(int handle) const { return lookup(handle) != nullptr; }

		RenderObject* lookup(int handle) const;

		static LuaFrameManager* instance() { return s_instance; }
		static void debugDumpBounds();   // log every visible widget's type/pos/size (layout debug)

	private:
		void dumpBounds() const;

		// Attach `child` under `parent` (anchored to parent's top-left + offset), register its handle.
		void attachChild(RenderObjectHolder& parent, shared_ptr<RenderObject> child, int handle);
		sf::Vector2i sizeOf(RenderObject* o) const;

		int m_nextHandle{100000};   // reserved Lua id range (no collision with World/Application ids)
		std::map<int, shared_ptr<RenderObject>> m_objects;

		static LuaFrameManager* s_instance;
};
