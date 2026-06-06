-- Emberfire default UI — core namespace + binding helpers.
-- This is the built-in "FrameXML": it loads before user addons and is fully overridable (its frames are
-- globals an addon can Hide()). Lua = visualization only; all game data/logic comes from the C++ API
-- (getters like UnitHealth, events in the Events table, commands like TargetUnit).

EmberUI = EmberUI or {}

-- True only while the player is in the world (set by the stage events). The HUD modules gate their
-- visibility on this, since the frame manager is now persistent across the login/character screens.
EmberUI.inWorld = false

-- bind(widget, setterName, getterFn, token, eventName): call widget:<setter>(getter(token)) on the event + now.
function EmberUI.bind(widget, setter, getter, token, event)
	local d = CreateFrame('Frame', nil, nil)
	local function apply() widget[setter](widget, getter(token)) end
	d:SetScript('OnEvent', apply)
	d:RegisterEvent(event)
	apply()
	return d
end

-- bindBar(bar, getter, maxGetter, token, valueEvent[, maxEvent]): drive a StatusBar; event-driven + 0.5s poll
-- (poll catches max-value changes that don't fire an event).
function EmberUI.bindBar(bar, getter, maxGetter, token, valueEvent, maxEvent)
	local function refresh()
		local mx = maxGetter(token); if mx <= 0 then mx = 1 end
		bar:SetMinMaxValues(0, mx); bar:SetValue(getter(token))
	end
	local d = CreateFrame('Frame', nil, nil)
	d:SetScript('OnEvent', refresh)
	d:RegisterEvent(valueEvent); if maxEvent then d:RegisterEvent(maxEvent) end
	local acc = 0
	d:SetScript('OnUpdate', function(_, dt) acc = acc + dt; if acc >= 0.5 then acc = 0; refresh() end end)
	refresh()
	return d
end

-- CreateItemButton(parent, index, size): an item slot composed from primitives (no native widget needed) —
-- optional slot-frame bg (EmberUI._slotBg), an icon Texture, a stack-count FontString, and a Cooldown overlay.
-- Returns { frame, icon, count, cd, index, SetItem(entry, stack), Clear() }. Caller wires scripts on .frame.
function EmberUI.CreateItemButton(parent, index, size)
	size = size or 40
	local b = CreateFrame('Button', nil, parent)
	b:SetSize(size, size)
	if EmberUI._slotBg then b:SetTexture(EmberUI._slotBg) end
	b:SetHoverTexture(EmberUI._slotHover or 'gameicon40_hover.png')   -- reddish edge highlight on mouseover

	local icon = b:CreateTexture()
	icon:SetPoint('TOPLEFT', b, 'TOPLEFT', 2, 2)
	icon:SetSize(size - 4, size - 4)

	local cd = CreateFrame('Cooldown', nil, b)
	cd:SetPoint('TOPLEFT', b, 'TOPLEFT', 2, 2)
	cd:SetSize(size - 4, size - 4)

	local count = b:CreateFontString()
	count:SetFont('Arial'); count:SetFontSize(12)
	count:SetPoint('BOTTOMRIGHT', b, 'BOTTOMRIGHT', -3, -2)

	local obj = { frame = b, icon = icon, count = count, cd = cd, index = index }
	function obj.SetItem(entry, stack)
		if not entry or entry == 0 then obj.Clear(); return end
		local _, tex = GetItemInfo(entry)
		if tex and tex ~= '' then icon:SetTexture(tex) end
		icon:Show()
		if stack and stack > 1 then count:SetText(tostring(stack)); count:Show()
		else count:SetText(''); count:Hide() end
	end
	function obj.Clear()
		icon:Hide(); count:SetText(''); count:Hide()
	end
	obj.Clear()
	return obj
end

-- Cursor item: a reusable "held on the cursor" icon + grey-out of the source slot, for drag-pickup across
-- windows (inventory now; equipment/spellbook/actionbar as they migrate). Call EmberUI.PickupItem on drag
-- start and EmberUI.ClearCursor on drop/cancel; EmberUI.HasCursorItem() reports whether something is held.
EmberUI._held = nil

local function ensureCursor()
	if EmberUI.cursor then return EmberUI.cursor end
	local c = CreateFrame('Frame', 'EmberCursorItem', nil)
	c:SetSize(36, 36)
	c:SetFrameLevel(10000)   -- above everything
	local tex = c:CreateTexture()
	tex:SetAllPoints(c)
	c.tex = tex
	c:Hide()
	c:SetScript('OnUpdate', function()
		if not GetCursorPosition then return end
		local x, y = GetCursorPosition()
		c:SetPoint('TOPLEFT', x - 18, y - 18)   -- center the icon on the cursor
	end)
	EmberUI.cursor = c
	return c
end

-- iconTexture: the item's icon name to ride the cursor; sourceIcon: the source slot's icon Texture to grey.
function EmberUI.PickupItem(iconTexture, sourceIcon)
	if GetCursorPosition and iconTexture and iconTexture ~= '' then
		local c = ensureCursor()
		c.tex:SetTexture(iconTexture); c.tex:Show(); c:Show()
	end
	if sourceIcon then sourceIcon:SetVertexColor(80, 80, 80, 255) end   -- grey overlay on the picked-up item
	EmberUI._held = { source = sourceIcon }
end

function EmberUI.ClearCursor()
	if EmberUI.cursor then EmberUI.cursor:Hide() end
	if EmberUI._held and EmberUI._held.source then EmberUI._held.source:SetVertexColor(255, 255, 255, 255) end
	EmberUI._held = nil
end

function EmberUI.HasCursorItem() return EmberUI._held ~= nil end

print('EmberUI core loaded')
