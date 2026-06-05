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
	int  createTexture(int frameHandle);       // a texture region owned by the frame
	int  createFontString(int frameHandle);    // a text region owned by the frame

	int  popClickedHandle();                   // drains the next clicked button handle (0 = none)

	void setPoint(int handle, int point, int relHandle, int relPoint, float xOfs, float yOfs);
	void setSize(int handle, float w, float h);
	void setText(int handle, const std::string& text);
	void setTexture(int handle, const std::string& textureName);
	void show(int handle, bool shown);
	void clearAllFrames();   // destroy every Lua-created frame (used by /reload)

	bool valid(int handle);

	// ---- game-state getters exposed to Lua (M4). token: "player" (others later). 0 if unavailable. ----
	int unitHealth(const std::string& token);
	int unitHealthMax(const std::string& token);
	int unitLevel(const std::string& token);
}
