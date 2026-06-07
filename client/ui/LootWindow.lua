-- Lua Loot window: replaces the C++ LootWindow (force-hidden, mirrored). The live C++ window stays the MODEL
-- (its GameIconList holds the loot, populated by the server); this View reads it via GetLootSlot* and drives
-- clicks via LootSlot/LootAll/CloseLoot. WoW-style events: LOOT_READY (loot available -> show + (re)populate,
-- also fires after each single loot), LOOT_CLOSED (hide). Money is just a loot entry (isGold). Item tooltips
-- are registered (frame:SetTooltipLoot) and re-asserted by the engine while hovered. NO addon OnUpdate.
-- NOTE: pixel positions are C++-derived starting points (LootWindow.cpp) — tune live via screenshot.

local ROW_X, ROW_Y0, ROW_H = 12, 40, 43      -- list origin + row pitch (C++ list at TL+(10,38), slotHeight 43)
local ICON = 34
local NAME_DX, NAME_DY = 42, 12              -- item-name text offset within a row
local VISIBLE = 4                            -- visible rows (C++ numVerticalElements); mouse-wheel/arrows scroll
local TAKE_ALL_X, TAKE_ALL_Y = 37, 217
local UP_X, UP_Y, DOWN_X, DOWN_Y = 183, 40, 183, 230

local W, H = GetTextureSize('loot_window.png')
if W <= 0 then W, H = 210, 290 end
local root = CreateFrame('Frame', 'EmberLoot', nil)
root:SetSize(W, H)
root:SetPoint('CENTER')
root:SetMovable(true); root:RegisterForDrag('LeftButton')
root:Hide()

local bg = root:CreateTexture()
bg:SetAllPoints(root)
bg:SetTexture('loot_window.png')

-- shared idioms (same as the other windows)
local function commas(n)
	local s = tostring(n)
	return (s:reverse():gsub('(%d%d%d)', '%1,'):reverse():gsub('^,', ''))
end
local function vis(f, v) if v then f:Show() else f:Hide() end end
local function texSize(name, dw, dh) local w, h = GetTextureSize(name); if w <= 0 then return dw, dh end return w, h end

local scroll = 0       -- index of the first visible entry (0-based)
local refresh          -- forward decl

-- ---- row pool (icon button + item-name text). Index into the loot list = scroll + row. ----
local rows = {}
for r = 1, VISIBLE do
	local rowY = ROW_Y0 + (r - 1) * ROW_H
	local b = EmberUI.CreateItemButton(root, r, ICON)
	b.frame:SetPoint('TOPLEFT', root, 'TOPLEFT', ROW_X, rowY)
	b.frame:EnableMouse(true)
	local nameFS = root:CreateFontString()
	nameFS:SetFont('Arial'); nameFS:SetFontSize(13); nameFS:SetTextColor(255, 255, 255, 255)
	nameFS:SetPoint('TOPLEFT', root, 'TOPLEFT', ROW_X + NAME_DX, rowY + NAME_DY)
	b.frame:SetScript('OnMouseUp', function(_, btn)
		if btn ~= 'LeftButton' then return end
		local idx = b._loot           -- 1-based loot index (b is a Lua table, fields ok)
		if not idx then return end
		if IsShiftKeyDown and IsShiftKeyDown() then LinkLootSlot(idx) else LootSlot(idx) end
	end)
	rows[r] = { btn = b, name = nameFS }
end

-- ---- Take All / close / scroll arrows (real *_idle.png sprites, NOT the .bscript names) ----
local function mkButton(idle, hover, x, y, onClick)
	local w, h = texSize(idle, 24, 24)
	local f = CreateFrame('Button', nil, root)
	f:SetTexture(idle); f:SetHoverTexture(hover); f:SetSize(w, h)
	f:SetPoint('TOPLEFT', root, 'TOPLEFT', x, y)
	f:EnableMouse(true); f:SetScript('OnClick', onClick)
	return f
end

local takeAll = mkButton('generic_highlight_idle.png', 'generic_highlight_hover.png', TAKE_ALL_X, TAKE_ALL_Y,
	function() LootAll() end)
local closeBtn = mkButton('panel_close_small_idle.png', 'panel_close_small_hover.png', W - 22, 10,
	function() CloseLoot() end)
local upBtn = mkButton('up_arrow_idle.png', 'up_arrow_hover.png', UP_X, UP_Y,
	function() scroll = scroll - 1; refresh() end)
local downBtn = mkButton('down_arrow_idle.png', 'down_arrow_hover.png', DOWN_X, DOWN_Y,
	function() scroll = scroll + 1; refresh() end)

refresh = function()
	local n = GetLootSlotCount()
	local maxScroll = math.max(0, n - VISIBLE)
	if scroll > maxScroll then scroll = maxScroll end
	if scroll < 0 then scroll = 0 end

	for r = 1, VISIBLE do
		local idx = scroll + r              -- 1-based loot index
		local row = rows[r]
		if idx <= n then
			local itemId, _, count, isGold = GetLootSlot(idx)
			row.btn._loot = idx
			if isGold then
				row.btn.SetItem(itemId, 1)                        -- coin icon, amount goes in the name text
				row.name:SetText(commas(count) .. ' Gold')
				row.name:SetTextColor(255, 224, 120, 255)
			else
				row.btn.SetItem(itemId, count)                    -- icon + stack-count overlay
				row.name:SetText(GetItemInfo(itemId) or '')
				local cr, cg, cb = GetItemQualityColor(itemId)
				row.name:SetTextColor(cr, cg, cb, 255)
			end
			row.btn.frame:SetTooltipLoot(idx)
			vis(row.btn.frame, true); row.name:Show()
		else
			row.btn._loot = nil
			row.btn.Clear()
			row.btn.frame:SetTooltipLoot(0)                       -- clear the row's tooltip
			vis(row.btn.frame, false); row.name:Hide()
		end
	end

	vis(upBtn, scroll > 0)
	vis(downBtn, scroll < maxScroll)
end

root:SetScript('OnMouseWheel', function(_, delta)
	scroll = scroll - delta                                       -- wheel up -> earlier entries
	refresh()
end)

-- ---- lifecycle (event-driven, zero OnUpdate) ----
local shown = false
local function setShown(v)
	if v == shown then return end
	shown = v
	if v then scroll = 0; root:Show(); refresh() else root:Hide() end
end

local stage = CreateFrame('Frame', nil, nil)
stage:SetScript('OnEvent', function(_, event)
	if event == Events.WORLD_SHOWN then
		SetGameFrameShown('LootFrame', false)        -- retire the C++ loot window
	elseif event == Events.LOOT_READY then
		if shown then refresh() else setShown(true) end   -- show on first ready, refresh on re-loot
	elseif event == Events.LOOT_CLOSED then
		setShown(false)
	else                                              -- left the world
		setShown(false)
	end
end)
for _, e in ipairs({ Events.WORLD_SHOWN, Events.LOOT_READY, Events.LOOT_CLOSED,
                     Events.LOGIN_SHOWN, Events.CHARSELECT_SHOWN, Events.CHARCREATE_SHOWN }) do
	stage:RegisterEvent(e)
end

print('EmberUI loot loaded')
