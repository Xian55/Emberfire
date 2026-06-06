#pragma once

#include "RenderObjectHolder.h"
#include "LuaUI.h"   // LuaUI::WidgetMouseEvent (queued in the manager)

#include <map>
#include <vector>
#include <set>
#include <string>

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
		void setAlpha(const int a);                                          // change only alpha, keep tint
		void setTexCoord(const float l, const float r, const float t, const float b);  // 0..1 sub-rect crop
		void setCircle(const int radius);                                    // render clipped to a circle (portraits)
		sf::Vector2i naturalSize() const;

	private:
		void input() final {}
		void render() final;

		shared_ptr<Sprite> m_sprite;
		sf::Color m_color{ 255, 255, 255, 255 };   // last tint (so setAlpha keeps RGB)
		int m_w{0};   // 0 => natural size (no scale)
		int m_h{0};
		bool m_circle{false};   // render as a circle (portrait) instead of a stretched rect
		int  m_circleRadius{0};
};

// A font-string region: draws a line of Text at its anchored position.
class LuaFontString : public RenderObject
{
	public:
		LuaFontString(RenderObject& owner, const int id);

		void setText(const std::string& str);
		void setFontSize(const int n);
		void setColor(const int r, const int g, const int b, const int a);
		void setAlpha(const int a);                   // change only alpha, keep RGB
		void setFont(const std::string& fontName);   // "Ringbearer", "Palatino", ...
		sf::Vector2i textSize() const;

	private:
		void input() final {}
		void render() final;

		unique_ptr<Text> m_text;
		sf::Color m_color{ 255, 255, 255, 255 };   // last fill color (so setAlpha keeps RGB)
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
		void setAlpha(const int a);                   // change only alpha, keep RGB
		sf::Vector2i barSize() const { return { m_w, m_h }; }
		float value() const    { return m_value; }   // for OnValueChanged edge detection
		float minValue() const { return m_min; }      // for OnMinMaxChanged edge detection
		float maxValue() const { return m_max; }

	private:
		void input() final {}
		void render() final;

		shared_ptr<Sprite> m_fill;
		sf::Color m_color{ 255, 255, 255, 255 };   // last tint (so setAlpha keeps RGB)
		int   m_w{0};
		int   m_h{0};
		float m_min{0.f};
		float m_max{1.f};
		float m_value{0.f};
};

// A radial cooldown sweep: draws a clockwise-shrinking dark pie over its rect representing time remaining.
// Reuses the shared drawCooldownPie helper (same pie as the C++ game icons).
class LuaCooldown : public RenderObject
{
	public:
		LuaCooldown(RenderObject& owner, const int id);

		void setCooldownSize(const int w, const int h) { m_w = w; m_h = h; }
		void setCooldown(const int remainingMs, const int durationMs);   // remaining<=0 or duration<=0 => clear
		int  remainingMs() const;                                        // live remaining (>=0)
		sf::Vector2i cooldownSize() const { return { m_w, m_h }; }

	private:
		void input() final {}
		void render() final;

		int m_w{0};
		int m_h{0};
		long long m_startMs{0};
		long long m_endMs{0};
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
		int  createCooldown(int parentHandle);
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
		void setAllPoints(int handle, int relHandle);
		void clearAllPoints(int handle);
		void setSize(int handle, float w, float h);
		void setText(int handle, const std::string& text);
		void setTexture(int handle, const std::string& textureName);
		void setHoverTexture(int handle, const std::string& textureName);
		void show(int handle, bool shown);
		void clearAll();   // destroy every Lua frame + reset the handle registry
		bool valid(int handle) const { return lookup(handle) != nullptr; }

		// Widget interaction state (set from Lua via the LuaUI wrappers).
		void setMouseEnabled(int handle, bool v);
		void setMovable(int handle, bool v);
		void setDragButton(int handle, int sfButton);
		bool popMouseEvent(LuaUI::WidgetMouseEvent& out);   // drain one queued mouse/drag event
		bool  isShownSelf(int handle) const;
		float statusBarValue(int handle) const;
		float statusBarMin(int handle) const;
		float statusBarMax(int handle) const;
		sf::Vector2i frameSize(int handle) const;           // current size via sizeOf

		// Introspection / visual primitives.
		int          parentOf(int handle) const;
		void         setName(int handle, const std::string& name);
		std::string  frameName(int handle) const;
		bool         isVisible(int handle) const;           // shown AND no hidden ancestor
		std::string  objectType(int handle) const;
		void         setAlpha(int handle, float a);
		void         setTexCoord(int handle, float l, float r, float t, float b);

		// Route results for context menus we host (Lua-opened unit menus) back to the C++ UnitFrame in World.
		void unregisterCtxMenu(const int id, const int childId, const std::string& result) override;
		bool popMenuResult(std::string& out);   // drain a clicked menu line for the engine (Lock/Unlock etc.)

		RenderObject* lookup(int handle) const;

		// True if point p (screen space) is within handle's visible bounds. Uses top-left + sizeOf (the
		// bottom-right corner ref is not maintained for all widget types), and rejects hidden ancestors.
		bool hitTest(int handle, sf::Vector2i p) const;

		static LuaFrameManager* instance() { return s_instance; }
		static void debugDumpBounds();   // log every visible widget's type/pos/size (layout debug)

	private:
		void input() override;    // detect+consume mouse buttons/wheel + run the drag state machine
		void render() override;   // recompute dynamic (relative/two-point) anchors, then draw children
		void dumpBounds() const;

		// Topmost (highest frame level) mouse-enabled handle whose bounds contain p; 0 if none.
		int topMouseHandleAt(sf::Vector2i p) const;

		// Recompute a frame's offset (+ size for two-point) from its stored anchors and apply it.
		void relayout(int handle);

		// Attach `child` under `parent` (anchored to parent's top-left + offset), register its handle.
		void attachChild(RenderObjectHolder& parent, shared_ptr<RenderObject> child, int handle);
		sf::Vector2i sizeOf(RenderObject* o) const;

		int m_nextHandle{100000};   // reserved Lua id range (no collision with World/Application ids)
		std::map<int, shared_ptr<RenderObject>> m_objects;

		// Anchors: WoW-style SetPoint. Each frame may have several (one per anchor point). relHandle 0 =>
		// owner/screen. Frames with a relative-to-other-frame or a two-point (stretch) anchor are recomputed
		// every frame (m_dynamicAnchored); pure single-point-to-owner anchors are applied once at setPoint.
		struct Anchor { int point; int relHandle; int relPoint; int x; int y; };
		std::map<int, std::vector<Anchor>> m_anchors;
		std::set<int>                      m_dynamicAnchored;
		std::map<int, int>                 m_parent;   // handle -> parent handle (GetParent)
		std::map<int, std::string>         m_name;     // handle -> name (GetName)

		// Per-handle interaction flags (EnableMouse / SetMovable / RegisterForDrag).
		struct LuaInputState { bool mouseEnabled = false; bool movable = false; int dragButton = -1; };
		std::map<int, LuaInputState>         m_inputState;
		std::vector<LuaUI::WidgetMouseEvent> m_mouseQueue;   // produced in input(), drained by the engine
		std::vector<std::string>             m_menuResults;  // clicked context-menu lines, drained by the engine
		int          m_dragHandle = 0;     // handle currently being dragged (0 = none)
		sf::Vector2i m_dragGrab;           // cursor-to-topLeft offset captured at drag start

		static LuaFrameManager* s_instance;
};
