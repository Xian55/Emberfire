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
	int  createCooldown(int parentHandle);     // a radial cooldown sweep (SetCooldown)
	int  createScrollFrame(int parentHandle);  // a scroll viewport (SetScrollChild + Set/GetVerticalScroll)
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
	bool unitHasBrokenEquipment(const std::string& token);       // player repair icon (ClientPlayer)
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
	int playerMoney();   // copper (ObjDefines::Variable::Money); 0 if not in world

	// ---- character sheet (Equipment) ----
	int playerVariable(int varId);          // INTERNAL (statRow); not exposed as a Lua global (raw var id)
	int playerExperience();                 // total accumulated Experience (named alias; GetXP = Progression)
	std::string playerClassName();          // "Paladin"/"Mage"/...
	std::string playerRankName();           // player_exp_levels[level].name
	void showEquipTooltip(int equipSlot, int ownerHandle, int anchor);   // re-assert an equipped item's tooltip
	void showStatTooltip(int varId, int ownerHandle, int anchor);        // stat-description tooltip
	int  statRowCount(int tab);             // per-tab stat-row count (tab 0=General 1=Combat 2=Skills)
	// fills row `index` of `tab`: label, formatted value, packed 0xRRGGBB, tooltipVar (0=none). false if oob.
	bool statRow(int tab, int index, std::string& label, std::string& value, int& rgb, int& tooltipVar);

	// ---- stat-point spending (drives the live force-hidden C++ Equipment; stats only) ----
	// statVar is a stat's variable id (= StatsStart + Stat) — the same value statRow returns as tooltipVar.
	bool isSpendingPoints();
	void beginStatSpend();                  // enter spend mode (clears pending, recomputes cost)
	void cancelStatSpend();                 // leave without committing
	bool addStatPoint(int statVar);         // queue +1 (gated by cost/cap/skill); false if not allowed
	bool removeStatPoint(int statVar);      // queue -1; false if nothing pending
	bool canAddStat(int statVar);           // may the +1 button show?
	bool canMinusStat(int statVar);         // may the -1 button show?
	int  pendingStatPoints(int statVar);    // points currently queued on this stat
	int  pendingLevelupCost();              // total queued XP cost (World::getCachedPendingLevelupCost)
	void confirmStatSpend();                // commit (sends op40); server replies LEVELUP_RESULT
	bool hasUnspentPoints();                // server flagged spendable points (the exclaim notice)

	// ---- bag / equipment / item data (read-only; reuses the force-hidden C++ Inventory + ClientPlayer) ----
	int  containerNumSlots();   // total bag slots (PlayerDefines::Inventory::NumSlots)
	// fills item at bag `slot` (0-based); false if no inventory / out of range. itemId 0 => empty slot.
	bool containerItem(int slot, int& itemId, int& count, int& durability, int& enchant, bool& soulbound);
	// fills the item in equip `slot` (UnitDefines::EquipSlot 1..12); false if no player. itemId 0 => empty.
	bool equipItem(int equipSlot, int& itemId, int& durability, int& enchant);
	// item_template lookups by entry (local DB — no server needed).
	void itemInfo(int entry, std::string& name, std::string& icon, int& quality, int& sellPrice, int& maxDurability);
	int  itemQualityColor(int entry);   // packed 0xRRGGBB (ItemIcon::itemColor for the quality)

	// ---- commands ----
	void targetUnit(const std::string& token);
	void clearTarget();

	// ---- item action commands (send the same packets the C++ Inventory does; slot = 0-based bag index;
	//      the full ItemDef is read from the live ItemIcon, so Lua only passes indices) ----
	void moveContainerItem(int fromSlot, int toSlot);   // bag slot -> bag slot (swap/move)
	void useContainerItem(int slot);
	void equipContainerItem(int slot);
	void sellContainerItem(int slot);                   // requires an open vendor server-side
	void destroyContainerItem(int slot);
	void unequipItem(int equipSlot, int invDest);       // equipSlot 1..12 -> bag slot
	// Tooltip anchor: 0=cursor, 1=right of owner frame, 2=left, 3=top, 4=bottom.
	void showItemTooltip(int slot, int ownerHandle, int anchor);   // re-assert the item tooltip (while hovering)
	bool containerItemMerchantAction(int slot);         // bank/trade/vendor open -> move/add/sell; true if handled

	// ---- loot window (reads + drives the live force-hidden LootWindow's GameIconList) ----
	int  lootSlotCount();                                            // number of loot entries (items + money)
	// fills loot entry `index` (0-based): itemId, affix, count, isGold (money entry). false if oob / no window.
	bool lootSlot(int index, int& itemId, int& affix, int& count, bool& isGold);
	void lootSlotTake(int index);                                   // click an entry -> loot it
	void lootTakeAll();                                             // Take All
	void lootSlotLink(int index);                                  // shift-click -> chat link
	void lootClose();                                              // close the loot panel (X button)
	void showLootTooltip(int index, int ownerHandle, int anchor);  // re-assert a loot item's tooltip
	bool isShiftDown();                                            // LShift/RShift held (shift-click chat link)

	// ---- bank (49-slot grid; reads/drives the live force-hidden Bank) ----
	int  bankNumSlots();
	bool bankItem(int slot, int& itemId, int& count, int& durability, int& enchant, bool& soulbound);
	void withdrawBankItem(int slot);                  // right-click -> GP_Client_UnBankItem (autoselect inv slot)
	void moveBankItem(int from, int to);              // bank<->bank drag
	void depositBagItem(int bagSlot, int bankSlot);   // bag->bank drag (bankSlot<0 = server picks)
	void sortBank();
	void showBankTooltip(int slot, int ownerHandle, int anchor);

	// ---- guild roster (reads/drives the live force-hidden GuildRoster) ----
	std::string guildName();
	std::string guildMotd();
	int  guildNumMembers();
	int  guildLocalRank();                  // GuildDefines::Rank ordinal (0 Initiate .. 3 Leader)
	bool guildMember(int index, std::string& name, int& level, bool& online, int& guid,
		std::string& className, int& classColor, std::string& rankName, int& rank);
	void setGuildMotd(const std::string& text);
	void guildPromote(int guid);
	void guildDemote(int guid);
	void guildKick(int guid);               // server-side TODO (placeholder opcode); parity with C++
	void invitePlayerToParty(const std::string& name);
	void requestGuildRoster();
	void whisperPlayer(const std::string& name);
	void useOrEquipContainerItem(int slot);             // right-click: use a consumable/quest item or equip gear
	bool containerItemUnusable(int slot);               // true if the item's requirements aren't met (red overlay)
	bool containerItemTargetsItem(int slot);            // true if using it targets another item (gem/orb)
	void useContainerItemOnItem(int sourceSlot, int targetSlot);   // apply a target-item consumable to a bag item

	// ---- input + confirm dialog (for drag-out destroy etc.) ----
	bool mouseButtonDown(int sfBtn);                    // raw button state (0=Left 1=Right 2=Middle)
	int  showConfirm(const std::string& msg);           // spawn a YesNo ConfirmMessageBox -> its unique code
	bool popConfirm(int& code, bool& yes);              // drain a finished confirm (code + yes/no); false if none

	// ---- hide/show the C++ windows a Lua view replaces ----
	// HUD: "PlayerFrame"|"TargetFrame"|"XPBar"; screens: "LoginScreen"|"CharSelectScreen"|"CharCreateScreen"
	// windows: "InventoryFrame"|"EquipmentFrame"
	void setGameFrameShown(const std::string& name, bool shown);
	// logical shown state of a toggled C++ window (InventoryFrame/EquipmentFrame), for the Lua mirror.
	bool gameFrameShown(const std::string& name);

	// ---- login screen ----
	void submitLogin(const std::string& user, const std::string& pass, bool remember);
	std::string getSavedLogin();

	// ---- screen metrics (for Lua layout) ----
	int screenWidth();
	int screenHeight();
	int cursorX();   // mouse position in render space (for a cursor-follow item icon)
	int cursorY();

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
	void  setTextureCircle(int handle, int radius);                     // render a texture clipped to a circle
	void  setCooldown(int handle, int remainingMs, int durationMs);     // drive a Cooldown widget's radial sweep
	int   cooldownRemaining(int handle);                               // live remaining ms (for the text label)

	// ---- scroll frame (faux-scroll: content child moved by offset, off-screen rows hidden) ----
	void  setScrollChild(int scrollFrameHandle, int childHandle);   // designate an existing child as the content
	void  setVerticalScroll(int handle, float v);                   // clamped to [0, range]
	float verticalScroll(int handle);
	float verticalScrollRange(int handle);                          // max(0, contentH - viewportH)
	void  setHorizontalScroll(int handle, float v);
	float horizontalScroll(int handle);
	float horizontalScrollRange(int handle);
	void  setScrollWheelStep(int handle, float px);                 // px moved per wheel notch (default 40)

	// ---- z-order within the parent (WoW SetFrameLevel/GetFrameLevel/Raise/Lower) ----
	void setFrameLevel(int handle, int level);
	int  getFrameLevel(int handle);
	void raiseFrame(int handle);
	void lowerFrame(int handle);
	void sortRootFrames();   // re-sort top-level frames by z-level (after addons load)

	// ---- debug ----
	void setDebugBounds(bool v);   // v=true: log every visible Lua widget's type/pos/size to the log
}
