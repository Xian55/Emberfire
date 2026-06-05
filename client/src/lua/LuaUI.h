#pragma once

#include <string>

// sol-free, client-header-free boundary between the ISOLATED sol2 binding TU (LuaEngine.cpp, built with
// std::byte re-enabled) and the C++ widget adapters (LuaFrameManager.cpp, a normal client TU). Both sides
// include this; it must use only plain primitives (ints/strings/floats) so it is ABI-safe across the
// _HAS_STD_BYTE boundary. Handles are ints (0 = none / the root frame manager). Implemented in
// LuaFrameManager.cpp; called from the sol bindings.
namespace LuaUI
{
	enum Point { TopLeft = 0, Center, Top, Bottom, Left, Right, TopRight, BottomLeft, BottomRight };

	int  createFrame(int parentHandle);        // parent 0 => the root manager (top-level frame)
	int  createButton(int parentHandle);       // a clickable region (OnClick via the poll bridge)
	int  createStatusBar(int parentHandle);    // a fill-bar (SetMinMaxValues/SetValue)
	int  createTexture(int frameHandle);       // a texture region owned by the frame
	int  createFontString(int frameHandle);    // a text region owned by the frame
	int  createEditBox(int parentHandle);      // a single-line text input (wraps PromptBox)

	int  popClickedHandle();                   // drains the next clicked button handle (0 = none)
	int  popSubmittedHandle();                 // drains the next Enter-submitted editbox handle (0 = none)

	std::string getText(int handle);                       // editbox text (empty if not an editbox)
	void setEditBoxPassword(int handle, bool masked);      // render typed text as asterisks
	void setEditBoxMaxLen(int handle, int n);              // clamp max length
	void setEditBoxNumeric(int handle, bool v);            // digits-only
	void setEditBoxFontSize(int handle, int n);            // character size
	void setEditBoxColor(int handle, int r, int g, int b, int a);   // editbox/fontstring text color
	void setVertexColor(int handle, int r, int g, int b, int a);    // texture tint + alpha
	void setFont(int handle, const std::string& fontName);          // fontstring font face

	void setMinMax(int handle, float mn, float mx);
	void setValue(int handle, float v);
	void setBarColor(int handle, int r, int g, int b, int a);

	void setPoint(int handle, int point, int relHandle, int relPoint, float xOfs, float yOfs);
	void setSize(int handle, float w, float h);
	void setText(int handle, const std::string& text);
	void setTexture(int handle, const std::string& textureName);
	void setHoverTexture(int handle, const std::string& textureName);   // button: art shown on mouse-over
	void show(int handle, bool shown);
	void clearAllFrames();   // destroy every Lua-created frame (used by /reload)

	bool valid(int handle);

	// ---- game-state getters exposed to Lua. token: "player" | "target". 0/"" if unavailable. ----
	int unitHealth(const std::string& token);
	int unitHealthMax(const std::string& token);
	int unitLevel(const std::string& token);
	int unitPower(const std::string& token);
	int unitPowerMax(const std::string& token);
	std::string unitName(const std::string& token);
	bool unitExists(const std::string& token);
	int playerXP();
	int playerMaxXP();

	// ---- commands ----
	void targetUnit(const std::string& token);
	void clearTarget();

	// ---- hide/show the C++ windows a Lua view replaces ----
	// HUD: "PlayerFrame"|"TargetFrame"|"XPBar"; screens: "LoginScreen"|"CharSelectScreen"|"CharCreateScreen"
	void setGameFrameShown(const std::string& name, bool shown);

	// ---- login screen ----
	void submitLogin(const std::string& user, const std::string& pass, bool remember);
	std::string getSavedLogin();

	// ---- screen metrics (for Lua layout) ----
	int screenWidth();
	int screenHeight();

	// ---- z-order within the parent (WoW SetFrameLevel/GetFrameLevel/Raise/Lower) ----
	void setFrameLevel(int handle, int level);
	int  getFrameLevel(int handle);
	void raiseFrame(int handle);
	void lowerFrame(int handle);
	void sortRootFrames();   // re-sort top-level frames by z-level (after addons load)

	// ---- debug ----
	void setDebugBounds(bool v);   // v=true: log every visible Lua widget's type/pos/size to the log
}
