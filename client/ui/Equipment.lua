-- Lua Equipment / character sheet: gear slots + stat tabs + Progression/Experience, replacing the C++
-- Equipment window (force-hidden, toggle mirrored). v2a = display parity; point-spending = phase 2b.
-- Stat rows (which stats per tab, their labels/colours/values) are QUERIED from C++ (GetStatRow*), not
-- hardcoded here. Tooltips are REGISTERED on the frames (frame:SetTooltip*) and re-asserted by the engine
-- while hovered — no addon OnUpdate poll. Static chrome (title, tab names, section labels) is on equipment.png.
-- NOTE: pixel positions are C++-derived starting points — tune live via screenshot.

local SIZE = 40
-- Gear slots: {x, y, equipSlot}. Helm1 Necklace2 Chest3 Belt4 Legs5 Feet6 Hands7 Ring8 Ring9 Weapon10
-- Offhand11 Ranged12. Left column then right column (C++ Equipment offsets, 58px rows).
local SLOTS = {
	{ 46, 178, 1 }, { 46, 236, 2 }, { 46, 294, 3 }, { 46, 352, 12 }, { 46, 410, 11 }, { 46, 468, 10 },
	{ 289, 178, 8 }, { 289, 236, 9 }, { 289, 294, 7 }, { 289, 352, 4 }, { 289, 410, 5 }, { 289, 468, 6 },
}

-- Layout (tune live).
local STAT_LX, STAT_VX, STAT_Y0, STAT_ROWH = 375, 480, 125, 23
local TAB_X, TAB_Y = { 150, 250, 384 }, 64
local TAB_NAMES = { 'GENERAL', 'COMBAT', 'SKILLS' }
local PROG_X, PROG_Y, EXP_Y = 165, 250, 312   -- Progression/Experience values (next to the printed labels)

local W, H = GetTextureSize('equipment.png')
if W <= 0 then W, H = 520, 560 end
local root = CreateFrame('Frame', 'EmberEquipment', nil)
root:SetSize(W, H)
root:SetPoint('CENTER')
root:SetMovable(true); root:RegisterForDrag('LeftButton')
root:Hide()

local refresh, refreshStats   -- forward decls

root:SetScript('OnMouseUp', function(_, btn)
	if btn == 'LeftButton' and EmberUI.HasCursorItem() then EmberUI.ClearCursor(); if refresh then refresh() end end
end)

local bg = root:CreateTexture()
bg:SetAllPoints(root)
bg:SetTexture('equipment.png')

local nameFS = root:CreateFontString()
nameFS:SetFont('FrizBold'); nameFS:SetFontSize(16); nameFS:SetTextColor(183, 148, 110, 255)
nameFS:SetPoint('TOPLEFT', root, 'TOPLEFT', 120, 113)
local subFS = root:CreateFontString()
subFS:SetFont('Palatino'); subFS:SetFontSize(13); subFS:SetTextColor(150, 130, 110, 255)
subFS:SetPoint('TOPLEFT', root, 'TOPLEFT', 120, 135)

-- Progression / Experience values next to the printed labels (middle column).
local progVal = root:CreateFontString()
progVal:SetFont('Palatino'); progVal:SetFontSize(13); progVal:SetTextColor(148, 133, 116, 255)
progVal:SetPoint('TOPLEFT', root, 'TOPLEFT', PROG_X, PROG_Y)
local expVal = root:CreateFontString()
expVal:SetFont('Palatino'); expVal:SetFontSize(13); expVal:SetTextColor(148, 133, 116, 255)
expVal:SetPoint('TOPLEFT', root, 'TOPLEFT', PROG_X, EXP_Y)

-- ---- gear slots (tooltip registered; hover highlight is built into the Button) ----
local function firstFreeBag()
	for i = 1, GetContainerNumSlots() do
		if GetContainerItem(i) == 0 then return i end
	end
	return nil
end

local slots = {}
for idx, def in ipairs(SLOTS) do
	local x, y, eq = def[1], def[2], def[3]
	local s = EmberUI.CreateItemButton(root, idx, SIZE)
	s.equip = eq
	s.frame:SetPoint('TOPLEFT', root, 'TOPLEFT', x, y)
	s.frame:EnableMouse(true)
	s.frame:SetTooltipEquip(eq)
	s.frame:SetScript('OnMouseDown', function(_, btn)
		if btn ~= 'LeftButton' then return end
		local id = GetInventorySlotItem(eq)
		if id == 0 then return end
		local _, tex = GetItemInfo(id)
		EmberUI.PickupItem(tex, s.icon, { from = 'equip', slot = eq })
	end)
	s.frame:SetScript('OnMouseUp', function(_, btn)
		if btn == 'RightButton' then
			local free = firstFreeBag()
			if free and GetInventorySlotItem(eq) ~= 0 then UnequipInventoryItem(eq, free) end
		elseif btn == 'LeftButton' then
			local p = EmberUI.HeldPayload and EmberUI.HeldPayload()
			if p and p.from == 'bag' then EquipContainerItem(p.slot) end
			EmberUI.ClearCursor(); refresh()
		end
	end)
	slots[idx] = s
end

-- ---- stat tabs + active-tab indicator ----
-- The tab names are printed on equipment.png (can't recolour), and overlaying the active name doubled it.
-- So mark the active tab with a bright caret to its LEFT. Tune TAB_MARK_DX/DY.
local TAB_MARK_DX, TAB_MARK_DY = -14, 0
local currentTab = 1
local activeMarker = root:CreateFontString()
activeMarker:SetFont('Trebuchet'); activeMarker:SetFontSize(14); activeMarker:SetTextColor(255, 224, 150, 255)
activeMarker:SetText('>')
local function setTab(t)
	currentTab = t
	activeMarker:SetPoint('TOPLEFT', root, 'TOPLEFT', TAB_X[t] + TAB_MARK_DX, TAB_Y + TAB_MARK_DY)
	refreshStats()
end
for t = 1, 3 do
	local b = CreateFrame('Button', nil, root)
	b:SetSize(120, 26)
	b:SetPoint('TOPLEFT', root, 'TOPLEFT', TAB_X[t], TAB_Y)
	b:EnableMouse(true)
	b:SetScript('OnClick', function() setTab(t) end)
end

-- ---- stat row pool (label + value). Rows + their data come from C++ (GetStatRow). ----
local ROWS = 14
local rowL, rowV = {}, {}
for i = 1, ROWS do
	local l = root:CreateFontString()
	l:SetFont('Palatino'); l:SetFontSize(13); l:SetTextColor(148, 133, 116, 255)
	l:SetPoint('TOPLEFT', root, 'TOPLEFT', STAT_LX, STAT_Y0 + (i - 1) * STAT_ROWH)
	l:Hide()
	local v = root:CreateFontString()
	v:SetFont('Palatino'); v:SetFontSize(13)
	v:SetPoint('TOPLEFT', root, 'TOPLEFT', STAT_VX, STAT_Y0 + (i - 1) * STAT_ROWH)
	v:Hide()
	rowL[i], rowV[i] = l, v
end

refreshStats = function()
	local n = GetStatRowCount(currentTab - 1)   -- C++ tabs are 0-based
	for i = 1, ROWS do
		if i <= n then
			local label, value, r, g, b, tipVar = GetStatRow(currentTab - 1, i - 1)
			rowL[i]:SetText(label); rowL[i]:Show(); rowL[i]:SetTooltipStat(tipVar)
			rowV[i]:SetText(value); rowV[i]:SetTextColor(r, g, b, 255); rowV[i]:Show()
		else
			rowL[i]:Hide(); rowV[i]:Hide(); rowL[i]:SetTooltipStat(0)
		end
	end
end

-- ---- level-up button (covers the printed bottom-left button; spending = phase 2b) ----
local levelBtn = CreateFrame('Button', nil, root)
levelBtn:SetTexture('level_up')
levelBtn:SetPoint('TOPLEFT', root, 'TOPLEFT', 14, 527)
levelBtn:EnableMouse(true)
levelBtn:SetScript('OnClick', function() print('Equipment: point-spending is coming soon') end)

-- ---- refresh + lifecycle ----
refresh = function()
	for _, s in ipairs(slots) do s.SetItem(GetInventorySlotItem(s.equip), nil) end
	nameFS:SetText(string.upper(UnitName('player')))
	local rank = GetPlayerRankName and GetPlayerRankName() or ''
	local sub = 'Level ' .. UnitLevel('player') .. ' ' .. (GetPlayerClassName and GetPlayerClassName() or '')
	if rank ~= '' then sub = sub .. ' (' .. rank .. ')' end
	subFS:SetText(sub)
	progVal:SetText(GetXP() .. ' / ' .. GetMaxXP())   -- Progression (current toward next level)
	local e = tostring(GetExperience()); e = e:reverse():gsub('(%d%d%d)', '%1,'):reverse():gsub('^,', '')
	expVal:SetText(e)
	refreshStats()
end

local shown = false
local function setShown(v)
	if v == shown then return end
	shown = v
	if v then root:Show(); refresh() else root:Hide(); EmberUI.ClearCursor() end
end

-- Fully event-driven: PANEL_OPENED/CLOSED show/hide (WoW Show()->OnShow), data events refresh while shown,
-- glue-screen events hide on leaving the world. NO OnUpdate in this window.
local stage = CreateFrame('Frame', nil, nil)
stage:SetScript('OnEvent', function(_, event, arg)
	if event == Events.WORLD_SHOWN then
		SetGameFrameShown('EquipmentFrame', false)   -- retire the C++ window
	elseif event == Events.PANEL_OPENED then
		if arg == 'EquipmentFrame' then setShown(true) end
	elseif event == Events.PANEL_CLOSED then
		if arg == 'EquipmentFrame' then setShown(false) end
	elseif event == Events.LOGIN_SHOWN or event == Events.CHARSELECT_SHOWN or event == Events.CHARCREATE_SHOWN then
		setShown(false)   -- left the world
	elseif shown then
		refresh()         -- PLAYER_EQUIPMENT_CHANGED / UNIT_* / PLAYER_STATS / PLAYER_XP_UPDATE
	end
end)
for _, e in ipairs({ Events.WORLD_SHOWN, Events.PANEL_OPENED, Events.PANEL_CLOSED,
                     Events.LOGIN_SHOWN, Events.CHARSELECT_SHOWN, Events.CHARCREATE_SHOWN,
                     Events.PLAYER_EQUIPMENT_CHANGED, Events.PLAYER_XP_UPDATE, Events.PLAYER_STATS }) do
	stage:RegisterEvent(e)
end
-- Unit events fire with a token; only the player's matter here, so filter at the source (RegisterUnitEvent)
-- instead of waking the handler for every target/party change.
for _, e in ipairs({ Events.UNIT_LEVEL, Events.UNIT_HEALTH, Events.UNIT_MAXHEALTH, Events.UNIT_POWER, Events.UNIT_MAXPOWER }) do
	stage:RegisterUnitEvent(e, 'player')
end

setTab(1)

print('EmberUI equipment loaded')
