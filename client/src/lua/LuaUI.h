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
	void setAllPoints(int handle, int relHandle);   // anchor TOPLEFT+BOTTOMRIGHT to rel (0 => owner/screen)
	void clearAllPoints(int handle);                // drop all anchors
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

	// ---- unit-frame parity getters (token: "player"|"target"|"party1".."party3") ----
	int  unitNameColor(const std::string& token);                 // packed 0xRRGGBB (faction/level-scaled)
	int  unitFlag(const std::string& token, const std::string& name);  // generic getVariable: Elite/Boss/InCombat/InArenaQueue/DynGreyTagged
	bool unitIsDead(const std::string& token);
	bool unitIsPlayer(const std::string& token);                  // false => NPC (or absent)
	bool unitIsPartyLeader(const std::string& token);
	std::string unitPortraitTexture(const std::string& token);    // square portrait texture name
	int  unitCastSpell(const std::string& token);                 // casting spell id (0 = not casting)
	int  unitCastElapsedMs(const std::string& token);
	int  unitCastTotalMs(const std::string& token);
	int  unitAuraCount(const std::string& token, bool harmful);
	// fills spellId/count/remainingMs/durationMs for aura `index` (0-based); false if out of range.
	bool unitAura(const std::string& token, int index, bool harmful, int& spellId, int& count, int& remainingMs, int& durationMs);
	bool partyMemberExists(int idx);                              // idx 0..2
	// open the C++ right-click menu for the unit; extraLines (newline-joined) are appended + handled Lua-side.
	void unitContextMenu(const std::string& token, const std::string& extraLines);
	bool popMenuResult(std::string& out);                        // drain a clicked menu line (for the engine)
	void showUnitTooltip(const std::string& token);             // re-assert the C++ unit tooltip (call while hovering)
	void showSpellTooltip(int spellId);                         // re-assert a spell/aura tooltip (call while hovering)

	// ---- persisted UI settings (default UI has no SavedVariables; backed by the config file) ----
	void saveUISetting(const std::string& key, int value);
	int  getUISetting(const std::string& key, int def);

	// ---- spell DB (icon/name by spell id) ----
	std::string spellTexture(int spellId);
	std::string spellName(int spellId);

	// ---- texture metrics (natural pixel size of a texture, for art-accurate layout) ----
	int textureWidth(const std::string& name);
	int textureHeight(const std::string& name);

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

	// True once the player is spawned in the world (HUD should be visible). Used by /reload to decide
	// whether to re-fire the in-world events (WORLD_SHOWN/PLAYER_LOGIN) that reveal the HUD.
	bool isInWorld();

	// True if the cursor is within this (visible) object's bounds. Backs frame:IsMouseOver() and the
	// engine's edge-detected OnEnter/OnLeave hover dispatch (see LuaEngine::onFrame).
	bool isMouseOver(int handle);

	// ---- widget interaction (mouse buttons / wheel / drag) ----
	// Mouse buttons, the wheel and drag are detected in the manager's input() pass (so they can be CONSUMED
	// before the world's click-to-move acts) and queued; LuaEngine::onFrame drains them and fires the Lua
	// OnMouseDown/OnMouseUp/OnMouseWheel/OnDragStart/OnDragStop/OnReceiveDrag handlers.
	struct WidgetMouseEvent
	{
		int   handle = 0;
		int   kind   = 0;   // 0=MouseDown 1=MouseUp 2=MouseWheel 3=DragStart 4=DragStop 5=ReceiveDrag
		int   button = 0;   // sf::Mouse index: 0=Left 1=Right 2=Middle (for MouseDown/Up)
		float delta  = 0.f; // wheel delta (for MouseWheel)
	};
	enum WidgetEventKind { WE_MouseDown = 0, WE_MouseUp, WE_MouseWheel, WE_DragStart, WE_DragStop, WE_ReceiveDrag };

	bool popMouseEvent(WidgetMouseEvent& out);          // drain one queued event; false when empty

	void setMouseEnabled(int handle, bool v);           // EnableMouse: frame becomes a hit target that consumes
	void setMovable(int handle, bool v);                // SetMovable: frame can be dragged
	void setDragButton(int handle, int sfButton);       // RegisterForDrag: -1 = none

	bool  isShownSelf(int handle);                      // the object's OWN shown flag (!isHidden), not ancestor
	float statusBarValue(int handle);                   // 0 if not a StatusBar
	float statusBarMin(int handle);
	float statusBarMax(int handle);
	int   frameWidth(int handle);                       // current size (sizeOf), 0 if unknown
	int   frameHeight(int handle);
	int   frameLeft(int handle);                        // current top-left screen X (reliable corner)
	int   frameTop(int handle);                         // current top-left screen Y

	// ---- introspection / visual primitives ----
	int   parentOf(int handle);                         // owner's handle (0 = top-level)
	void  setName(int handle, const std::string& name); // store the CreateFrame name
	std::string frameName(int handle);
	bool  isVisible(int handle);                        // shown AND no hidden ancestor
	std::string objectType(int handle);                 // "Frame"/"Button"/"StatusBar"/...
	void  setAlpha(int handle, float a);                // 0..1 -> per-widget vertex/text alpha
	void  setTexCoord(int handle, float l, float r, float t, float b);   // 0..1 sub-rect (texture crop)

	// ---- z-order within the parent (WoW SetFrameLevel/GetFrameLevel/Raise/Lower) ----
	void setFrameLevel(int handle, int level);
	int  getFrameLevel(int handle);
	void raiseFrame(int handle);
	void lowerFrame(int handle);
	void sortRootFrames();   // re-sort top-level frames by z-level (after addons load)

	// ---- debug ----
	void setDebugBounds(bool v);   // v=true: log every visible Lua widget's type/pos/size to the log
}
