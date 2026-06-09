-- Lua ActionBar: the 3x12 action bars, drawn over the C++ `toolbar_base.png` art (which, with the XP bar and
-- the 6 interface buttons, stays C++). The 3 C++ Toolbars are flipped to "Lua view" via SetActionBarsLuaView:
-- they draw nothing but stay the data + cast + cooldown + per-player cache engine, and KEEP keybind->cast
-- (keyboard-global, so it can't conflict with the mouse). This addon owns the visuals, click-cast and drag.
--
-- Reads:  GetActionInfo / GetActionCooldown / GetActionUsable / GetActionCount / GetActionKeybind.
-- Drives: UseAction (click) / PlaceAction (drop+swap) / ClearAction (drag off the bar).
-- Tooltip: SetTooltipAction(slot) -- the engine re-asserts the C++ icon's own tooltip while hovered.
-- Drag uses the shared EmberUI cursor, so a spell dragged from the (Lua) spellbook or an item from a bag drops
-- straight onto a slot. Layout mirrors World::updateGeometry: centered horizontally, anchored to the bottom.

local NUM_BARS, PER_BAR = 3, 12
local NUM = NUM_BARS * PER_BAR
local SLOT, SPACING = 40, 46
local ICON_INSET = 2
-- per-bar top-left, measured from the sprite origin (the exact C++ values in World::updateGeometry).
local BAR_OFF = { { 151, 11 }, { 151, -36 }, { 151, -83 } }

local root = CreateFrame('Frame', 'EmberActionBar', nil)
root:Hide()

local buttons = {}        -- [globalSlot] = button object
local heldAction = nil    -- the global slot we picked up FROM the bar (nil for external drags); for void-drop clear
local shown = false
local refresh             -- forward decl

-- Resolve a held cursor payload to (typeStr, entry) for PlaceAction; nil if it isn't something we can place.
local function payloadToAction(p)
	if not p then return nil end
	if p.kind == 'spell'  then return 'spell', p.id end
	if p.kind == 'action' then return p.atype, p.entry end
	if p.from == 'bag' then
		local id = GetContainerItem(p.slot)
		if id and id ~= 0 then return 'item', id end
	end
	return nil
end

-- Texture for a spell/item entry (the icon that rides the cursor while dragging a slot).
local function spellOrItemTex(typ, entry)
	if typ == 'spell' then return GetSpellTexture(entry) end
	local _, tex = GetItemInfo(entry)
	return tex
end

-- ---- one action button ----
local function makeButton(g)
	local b = CreateFrame('Button', nil, root)
	b:SetSize(SLOT, SLOT); b:EnableMouse(true)
	b:SetHoverTexture('gameicon40_hover.png')   -- red edge highlight on mouse-over (matches the bag slots)

	local icon = b:CreateTexture()
	icon:SetPoint('TOPLEFT', b, 'TOPLEFT', ICON_INSET, ICON_INSET)
	icon:SetSize(SLOT - ICON_INSET * 2, SLOT - ICON_INSET * 2)

	local cd = CreateFrame('Cooldown', nil, b)   -- radial sweep
	cd:SetPoint('TOPLEFT', b, 'TOPLEFT', ICON_INSET, ICON_INSET)
	cd:SetSize(SLOT - ICON_INSET * 2, SLOT - ICON_INSET * 2)

	local count = b:CreateFontString()           -- item stack count (bottom-right)
	count:SetFont('Arial'); count:SetFontSize(12); count:SetTextColor(255, 255, 255, 255)
	count:SetPoint('BOTTOMRIGHT', b, 'BOTTOMRIGHT', -3, -2)

	local key = b:CreateFontString()             -- keybind label (top-left)
	key:SetFont('Arial'); key:SetFontSize(11)
	key:SetPoint('TOPLEFT', b, 'TOPLEFT', 3, 1)

	local cdtext = b:CreateFontString()          -- numeric cooldown countdown (center)
	cdtext:SetFont('Arial'); cdtext:SetFontSize(14); cdtext:SetTextColor(255, 220, 60, 255)
	cdtext:SetPoint('CENTER', b, 'CENTER', 0, 0)

	local obj = { frame = b, icon = icon, cd = cd, count = count, key = key, cdtext = cdtext, slot = g, entry = 0 }

	b:SetTooltipAction(g)   -- engine re-asserts the C++ icon's tooltip while this slot is hovered

	-- Left-press picks the slot's icon up onto the cursor (greyed). A release on the SAME slot is a click->use;
	-- a release on a DIFFERENT slot drops/swaps. (Matches the bag's proven OnMouseDown/Up drag idiom.)
	b:SetScript('OnMouseDown', function(_, btn)
		if btn ~= 'LeftButton' then return end
		if EmberUI.HasCursorItem() then return end   -- already dragging something (e.g. from the spellbook)
		if obj.entry == 0 then return end
		local typ = obj.type
		EmberUI.PickupItem(spellOrItemTex(typ, obj.entry), icon, { kind = 'action', slot = g, atype = typ, entry = obj.entry })
		heldAction = g
	end)

	b:SetScript('OnMouseUp', function(_, btn)
		if btn == 'RightButton' then
			if EmberUI.HasCursorItem() then       -- right-click cancels a pickup
				heldAction = nil; EmberUI.ClearCursor(); refresh()
			else
				UseAction(g)
			end
			return
		end
		if btn ~= 'LeftButton' then return end

		if heldAction == g and EmberUI.HasCursorItem() then
			-- picked up here and released here => a click => use it
			heldAction = nil; EmberUI.ClearCursor(); UseAction(g); refresh()
			return
		end

		local p = EmberUI.HeldPayload and EmberUI.HeldPayload()
		local typ, entry = payloadToAction(p)
		if typ and entry then
			if p.kind == 'action' and p.slot ~= g then
				-- swap: move this slot's old content back to the source
				local oldType, oldEntry = obj.type, obj.entry
				PlaceAction(g, typ, entry)
				if oldEntry and oldEntry ~= 0 then PlaceAction(p.slot, oldType, oldEntry) else ClearAction(p.slot) end
			else
				PlaceAction(g, typ, entry)   -- drop a spell/bag item (or self) here
			end
		end
		heldAction = nil
		EmberUI.ClearCursor()
		refresh()
	end)

	buttons[g] = obj
	return obj
end

for g = 1, NUM do makeButton(g) end

-- ---- layout: place each button at the exact C++ screen pixel (centered, bottom-anchored) ----
local function relayout()
	local sw, sh = GetScreenWidth(), GetScreenHeight()
	local bw, bh = GetTextureSize('toolbar_base.png')
	if bw <= 0 then bw, bh = 980, 94 end
	local spx, spy = math.floor(sw / 2 - bw / 2), sh - bh
	for g = 1, NUM do
		local bar = math.floor((g - 1) / PER_BAR)
		local col = (g - 1) % PER_BAR
		local x = spx + BAR_OFF[bar + 1][1] + col * SPACING
		local y = spy + BAR_OFF[bar + 1][2]
		buttons[g].frame:ClearAllPoints()
		buttons[g].frame:SetPoint('TOPLEFT', x, y)
	end
end

-- ---- content: icon/type/entry/count/keybind (cheap enough to run on the data events) ----
refresh = function()
	for g = 1, NUM do
		local o = buttons[g]
		local typ, entry, tex = GetActionInfo(g)
		o.type, o.entry = typ, entry or 0
		o.cdShown = false   -- re-evaluate the cooldown sweep after any content change (drag/assign)
		if entry and entry ~= 0 then
			if tex and tex ~= '' then o.icon:SetTexture(tex) end
			o.icon:Show()
			local n = GetActionCount(g)
			if n and n > 1 then o.count:SetText(tostring(n)); o.count:Show() else o.count:SetText(''); o.count:Hide() end
		else
			o.icon:Hide(); o.count:SetText(''); o.count:Hide()
		end
		o.key:SetText(GetActionKeybind(g) or '')
	end
end

-- ---- dynamic: cooldown sweep + usable/range/mana tinting (per OnUpdate tick) ----
-- The Cooldown widget self-animates after ONE SetCooldown (like AuraRenderer): seed it when a cooldown begins
-- (or is re-triggered), read the live remaining via GetCooldownRemaining for the numeric text, clear when done.
local function refreshDynamic()
	for g = 1, NUM do
		local o = buttons[g]
		if o.entry ~= 0 then
			local rem, dur = GetActionCooldown(g)
			if rem and rem > 0 then
				if not o.cdShown or rem > (o.cdLastRem or 0) + 250 or math.abs((o.cdDur or 0) - dur) > 1 then
					o.cd:SetCooldown(rem, dur); o.cdDur = dur   -- new / refreshed cooldown -> (re)seed the sweep
				end
				o.cdShown = true; o.cdLastRem = rem
				local live = o.cd:GetCooldownRemaining()
				o.cdtext:SetText(live >= 1000 and tostring(math.floor(live / 1000 + 0.5)) or ''); o.cdtext:Show()
			else
				if o.cdShown then o.cd:SetCooldown(0, 0); o.cdShown = false end
				o.cdtext:SetText(''); o.cdtext:Hide()
			end
			local usable, oor, oom = GetActionUsable(g)
			if not usable then o.icon:SetVertexColor(100, 100, 100, 255)        -- darkened (can't act / req unmet)
			elseif oom then    o.icon:SetVertexColor(120, 120, 255, 255)        -- out of mana (blue)
			else               o.icon:SetVertexColor(255, 255, 255, 255) end
			if oor then o.key:SetTextColor(255, 80, 80, 255) else o.key:SetTextColor(255, 255, 255, 255) end   -- out of range -> red
		end
	end
end

-- ---- show only in-world; flip the C++ bars headless once we have a World ----
local function setShown(v)
	if v == shown then return end
	shown = v
	if v then root:Show(); relayout(); refresh() else root:Hide(); heldAction = nil; EmberUI.ClearCursor() end
end

local stage = CreateFrame('Frame', nil, nil)
stage:SetScript('OnEvent', function(_, event, arg)
	if event == Events.WORLD_SHOWN or event == Events.PLAYER_LOGIN then
		SetActionBarsLuaView(true)   -- the 3 C++ bars go headless (keep keybind/cast/cooldown/cache)
		setShown(true)
	elseif event == Events.LOGIN_SHOWN or event == Events.CHARSELECT_SHOWN or event == Events.CHARCREATE_SHOWN then
		setShown(false)
	elseif event == Events.SPELLBOOK_UPDATE or event == Events.BAG_UPDATE then
		if shown then refresh() end
	end
end)
for _, e in ipairs({ Events.WORLD_SHOWN, Events.PLAYER_LOGIN, Events.LOGIN_SHOWN, Events.CHARSELECT_SHOWN,
                     Events.CHARCREATE_SHOWN, Events.SPELLBOOK_UPDATE, Events.BAG_UPDATE }) do
	stage:RegisterEvent(e)
end

-- ---- per-frame: cooldown/tint refresh + void-drop (release an action off the bar => clear the slot) ----
local poll = CreateFrame('Frame', nil, nil)
local acc, wasDown, dropCheck = 0, false, false
poll:SetScript('OnUpdate', function(_, dt)
	if not shown then return end

	acc = acc + dt
	if acc >= 0.1 then acc = 0; refreshDynamic() end

	-- Void-drop: a slot's OnMouseUp consumes a drop (PlaceAction + ClearCursor) the same frame; we decide a frame
	-- LATER, so a still-held cursor means it was dropped on nothing. The ActionBar owns the lifecycle of action +
	-- spell pickups (inventory owns bag/equip/bank item pickups), so only those are cleared here.
	local down = IsMouseButtonDown('LeftButton')
	if dropCheck then
		if EmberUI.HasCursorItem() then
			if heldAction then ClearAction(heldAction); refresh() end   -- an action dragged off the bar => clear it
			EmberUI.ClearCursor()                                       -- (a spell dropped on nothing => just drop it)
		end
		heldAction = nil; dropCheck = false
	elseif wasDown and not down and EmberUI.HasCursorItem() then
		local p = EmberUI.HeldPayload and EmberUI.HeldPayload()
		if p and (p.kind == 'action' or p.kind == 'spell') then dropCheck = true end   -- decide next frame
	end
	wasDown = down
end)

print('EmberUI action bar loaded')
