-- Lua Equipment / character sheet: gear slots + stat tabs + Progression/Experience, replacing the C++
-- Equipment window (force-hidden, toggle mirrored). v2a = display parity. Point-spending (the +/- buttons,
-- cost, level-up confirm) is phase 2b. Static chrome (title, tab names, section labels) is printed on
-- equipment.png; we draw the dynamic rows/values + interactive buttons over it.
-- NOTE: stat-row / value pixel positions are C++-derived starting points — tune live via screenshot.

local SIZE = 40
-- Gear slots: {x, y, equipSlot}. EquipSlot: Helm1 Necklace2 Chest3 Belt4 Legs5 Feet6 Hands7 Ring8 Ring9
-- Weapon10 Offhand11 Ranged12. Left column then right column (C++ Equipment offsets, 58px rows).
local SLOTS = {
	{ 46, 178, 1 }, { 46, 236, 2 }, { 46, 294, 3 }, { 46, 352, 12 }, { 46, 410, 11 }, { 46, 468, 10 },
	{ 289, 178, 8 }, { 289, 236, 9 }, { 289, 294, 7 }, { 289, 352, 4 }, { 289, 410, 5 }, { 289, 468, 6 },
}

-- Stat colours (C++ Equipment::variableVaueColor). Default tan; a few themed.
local C_DEF  = { 148, 133, 116 }
local C_HP   = { 156, 74, 51 }
local C_MANA = { 55, 111, 184 }
local C_FROST= { 55, 182, 184 }
local C_FIRE = { 188, 104, 48 }
local C_SHAD = { 61, 48, 188 }
local C_HOLY = { 188, 48, 147 }

-- Per-tab stat rows: {varId, label, color, fmt}. varId = ObjDefines::Variable (stats = StatsStart 0x0e+index).
local GENERAL = {
	{ 0x03, 'Health', C_HP }, { 0x0f, 'Mana', C_MANA }, { 0x11, 'Armor Value' },
	{ 0x12, 'Strength' }, { 0x13, 'Agility' }, { 0x14, 'Willpower' }, { 0x15, 'Intelligence' }, { 0x16, 'Courage' },
	{ 0x17, 'Regeneration' }, { 0x18, 'Meditate' },
	{ 0x29, 'Fortitude' }, { 0xb3, 'Player Kills' }, { 0xb1, 'Arena Rating' },
}
local COMBAT = {
	{ 0x19, 'Melee Value' }, { 0x1a, 'Melee Speed', nil, 'speed' }, { 0x1b, 'Ranged Value' }, { 0x1c, 'Ranged Speed', nil, 'speed' },
	{ 0x1d, 'Melee Critical' }, { 0x1e, 'Ranged Critical' }, { 0x1f, 'Spell Critical' },
	{ 0x20, 'Dodge Rating' }, { 0x21, 'Block Rating' },
	{ 0x23, 'Resist Frost', C_FROST }, { 0x24, 'Resist Fire', C_FIRE }, { 0x25, 'Resist Shadow', C_SHAD }, { 0x26, 'Resist Holy', C_HOLY },
}
local SKILLS = {
	{ 0x2a, 'Staves' }, { 0x2b, 'Maces' }, { 0x2c, 'Axes' }, { 0x2d, 'Swords' }, { 0x2e, 'Ranged' },
	{ 0x2f, 'Daggers' }, { 0x30, 'Wands' }, { 0x31, 'Shields' },
	{ 0x27, 'Bartering' }, { 0x28, 'Lockpicking' },
}
local TABS = { GENERAL, COMBAT, SKILLS }

-- Stat-row layout (tune live).
local STAT_LX, STAT_VX, STAT_Y0, STAT_ROWH = 330, 470, 150, 23

local W, H = GetTextureSize('equipment.png')
if W <= 0 then W, H = 520, 560 end
local root = CreateFrame('Frame', 'EmberEquipment', nil)
root:SetSize(W, H)
root:SetPoint('CENTER')
root:SetMovable(true); root:RegisterForDrag('LeftButton')
root:Hide()

local refresh   -- forward decl

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

-- ---- gear slots ----
local hoverEquip = nil
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
	s.frame:SetScript('OnEnter', function() hoverEquip = eq end)
	s.frame:SetScript('OnLeave', function() if hoverEquip == eq then hoverEquip = nil end end)
	slots[idx] = s
end

-- ---- stat tabs (clickable regions over the printed GENERAL/COMBAT/SKILLS labels) ----
local currentTab = 1
local function refreshStats() end   -- defined below; forward via upvalue
local TAB_X = { 110, 250, 384 }
for t = 1, 3 do
	local b = CreateFrame('Button', nil, root)
	b:SetSize(120, 26)
	b:SetPoint('TOPLEFT', root, 'TOPLEFT', TAB_X[t], 64)
	b:EnableMouse(true)
	b:SetScript('OnClick', function() currentTab = t; refreshStats() end)
end

-- ---- stat row pool ----
local ROWS = 16
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

local function valText(varId, fmt)
	local n = GetPlayerVariable(varId)
	if fmt == 'speed' then return string.format('%.2f', n / 1000) end
	return tostring(n)
end

refreshStats = function()
	local list = TABS[currentTab]
	local row = 1
	for _, st in ipairs(list) do
		local l, v = rowL[row], rowV[row]
		l:SetText(st[2]); l:Show()
		v:SetText(valText(st[1], st[4]))
		local c = st[3] or C_DEF
		v:SetTextColor(c[1], c[2], c[3], 255); v:Show()
		row = row + 1
	end
	-- Progression + Experience after a gap (all tabs).
	row = row + 1
	if row <= ROWS then
		rowL[row]:SetText('Progression'); rowL[row]:Show()
		rowV[row]:SetText(GetPlayerVariable(0xa3) .. ' / ' .. GetMaxXP()); rowV[row]:SetTextColor(148, 133, 116, 255); rowV[row]:Show()
		row = row + 1
	end
	if row <= ROWS then
		rowL[row]:SetText('Experience'); rowL[row]:Show()
		local e = tostring(GetPlayerVariable(0xa4)); e = e:reverse():gsub('(%d%d%d)', '%1,'):reverse():gsub('^,', '')
		rowV[row]:SetText(e); rowV[row]:SetTextColor(148, 133, 116, 255); rowV[row]:Show()
		row = row + 1
	end
	for i = row, ROWS do rowL[i]:Hide(); rowV[i]:Hide() end
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
	refreshStats()
end

local shown = false
local function setShown(v)
	if v == shown then return end
	shown = v
	if v then root:Show(); refresh() else root:Hide(); EmberUI.ClearCursor() end
end

local stage = CreateFrame('Frame', nil, nil)
stage:SetScript('OnEvent', function(_, event)
	if event == Events.WORLD_SHOWN then
		SetGameFrameShown('EquipmentFrame', false)
	elseif shown then
		refresh()
	end
end)
stage:RegisterEvent(Events.WORLD_SHOWN)
stage:RegisterEvent(Events.PLAYER_EQUIPMENT_CHANGED)
stage:RegisterEvent(Events.UNIT_LEVEL)
stage:RegisterEvent(Events.UNIT_HEALTH)
stage:RegisterEvent(Events.UNIT_MAXHEALTH)
stage:RegisterEvent(Events.UNIT_POWER)
stage:RegisterEvent(Events.UNIT_MAXPOWER)
stage:RegisterEvent(Events.PLAYER_XP_UPDATE)

local poll = CreateFrame('Frame', nil, nil)
local acc = 0
local statAcc = 0
poll:SetScript('OnUpdate', function(_, dt)
	if not EmberUI.inWorld then if shown then setShown(false) end return end
	acc = acc + dt
	if acc >= 0.15 then acc = 0; setShown(IsGameFrameShown('EquipmentFrame')) end
	-- stats have no dedicated event for most rows; refresh on a slow poll while open.
	if shown then
		statAcc = statAcc + dt
		if statAcc >= 0.5 then statAcc = 0; refreshStats() end
	end
	if shown and hoverEquip and ShowEquipTooltip then ShowEquipTooltip(hoverEquip) end
end)

print('EmberUI equipment loaded')
