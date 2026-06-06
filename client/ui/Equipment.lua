-- Lua Equipment: the 12 gear slots, replacing the C++ Equipment window (force-hidden, toggle mirrored like
-- the bag). Drag a bag item onto a slot to equip; drag a slot onto the bag (or right-click) to unequip.
-- v1 = gear loop only. Phase 2 (needs new getters/widgets + a relink): the stat tabs, level-up point
-- spending, combat rating, equipped-item tooltips.

-- Slot layout copied from C++ Equipment (attachObj offsets, 58px rows). {x, y, equipSlot(UnitDefines)}.
-- EquipSlot: Helm1 Necklace2 Chest3 Belt4 Legs5 Feet6 Hands7 Ring1=8 Ring2=9 Weapon1=10 Offhand11 Ranged12.
local SLOTS = {
	{ 46, 178, 1 }, { 46, 236, 2 }, { 46, 294, 3 }, { 46, 352, 12 }, { 46, 410, 11 }, { 46, 468, 10 },
	{ 289, 178, 8 }, { 289, 236, 9 }, { 289, 294, 7 }, { 289, 352, 4 }, { 289, 410, 5 }, { 289, 468, 6 },
}
local SIZE = 40

local W, H = GetTextureSize('equipment.png')
if W <= 0 then W, H = 520, 560 end
local root = CreateFrame('Frame', 'EmberEquipment', nil)
root:SetSize(W, H)
root:SetPoint('CENTER')
root:SetMovable(true); root:RegisterForDrag('LeftButton')
root:Hide()

local refresh   -- forward decl (handlers below capture it)

-- Releasing a held item over the backdrop returns it (cross-window held is cleared by EmberUI.ClearCursor).
root:SetScript('OnMouseUp', function(_, btn)
	if btn == 'LeftButton' and EmberUI.HasCursorItem() then EmberUI.ClearCursor(); if refresh then refresh() end end
end)

local bg = root:CreateTexture()
bg:SetAllPoints(root)
bg:SetTexture('equipment.png')

local name = root:CreateFontString()
name:SetFont('FrizBold'); name:SetFontSize(16); name:SetTextColor(183, 148, 110, 255)
name:SetPoint('TOPLEFT', root, 'TOPLEFT', 120, 113)
local subname = root:CreateFontString()
subname:SetFont('Palatino'); subname:SetFontSize(13); subname:SetTextColor(150, 130, 110, 255)
subname:SetPoint('TOPLEFT', root, 'TOPLEFT', 120, 135)

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
	-- pick up the equipped item (grey + cursor), payload tags it as an equip-slot item for cross-window drop.
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
			if p and p.from == 'bag' then EquipContainerItem(p.slot) end   -- equip a dragged bag item
			EmberUI.ClearCursor(); refresh()
		end
	end)
	slots[idx] = s
end

refresh = function()
	for _, s in ipairs(slots) do
		s.SetItem(GetInventorySlotItem(s.equip), nil)
	end
	name:SetText(string.upper(UnitName('player')))
	subname:SetText('Level ' .. UnitLevel('player'))
end

-- Mirror the C++ Equipment toggle state (the toolbar character button still drives it while force-hidden).
local shown = false
local function setShown(v)
	if v == shown then return end
	shown = v
	if v then root:Show(); refresh() else root:Hide(); EmberUI.ClearCursor() end
end

local stage = CreateFrame('Frame', nil, nil)
stage:SetScript('OnEvent', function(_, event)
	if event == Events.WORLD_SHOWN then
		SetGameFrameShown('EquipmentFrame', false)   -- retire the C++ window; we render the gear now
	elseif shown then
		refresh()   -- PLAYER_EQUIPMENT_CHANGED / UNIT_LEVEL
	end
end)
stage:RegisterEvent(Events.WORLD_SHOWN)
stage:RegisterEvent(Events.PLAYER_EQUIPMENT_CHANGED)
stage:RegisterEvent(Events.UNIT_LEVEL)

local poll = CreateFrame('Frame', nil, nil)
local acc = 0
poll:SetScript('OnUpdate', function(_, dt)
	if not EmberUI.inWorld then if shown then setShown(false) end return end
	acc = acc + dt
	if acc >= 0.15 then acc = 0; setShown(IsGameFrameShown('EquipmentFrame')) end
end)

print('EmberUI equipment loaded')
