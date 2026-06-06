-- Lua Inventory: a bag grid that REPLACES the C++ Inventory window. The C++ window is force-hidden in-world
-- but still toggled by its open button; this view MIRRORS that toggle state (IsGameFrameShown) and renders
-- bag contents from the data getters. Left-drag a slot onto another = MoveContainerItem; right-click = Use.
--
-- Hover shows the item tooltip (ShowItemTooltip, re-asserted each frame); money shown at the panel spot.
-- v1 deviations (tune in the live client): no quality border (set EmberUI._slotBg to skin the slots); fixed
-- CENTER position; no equip/sell/destroy wiring (the commands exist, add menus later).

-- Grid geometry copied from the C++ Inventory::attachIcon so the icons land in inventory.png's printed cells:
-- first slot at (27,75), 45px pitch, 40px icons, 7 columns.
local COLS, SIZE, PITCH = 7, 40, 45
local OX, OY = 27, 75
local NUM = GetContainerNumSlots()

-- Movable backdrop window sized to the real art (inventory.png is the C++ bag art — a confirmed asset).
local W, H = GetTextureSize('inventory.png')
if W <= 0 then W, H = 300, 420 end
local root = CreateFrame('Frame', 'EmberInventory', nil)
root:SetSize(W, H)
root:SetPoint('CENTER')
root:SetMovable(true); root:RegisterForDrag('LeftButton')   -- drag the window by its empty backdrop
root:Hide()
-- releasing a held item over the backdrop (not a slot) returns it to its source.
root:SetScript('OnMouseUp', function(_, btn)
	if btn == 'LeftButton' and EmberUI.HasCursorItem() then dragFrom = nil; EmberUI.ClearCursor() end
end)

local bg = root:CreateTexture()
bg:SetAllPoints(root)
bg:SetTexture('inventory.png')

-- Money readout (gold, with thousands commas) at the C++ panel's money spot.
local function commas(n)
	local s = tostring(n)
	local k
	repeat s, k = s:gsub('^(-?%d+)(%d%d%d)', '%1,%2') until k == 0
	return s
end
local money = root:CreateFontString()
money:SetFont('Palatino'); money:SetFontSize(15)
money:SetTextColor(255, 215, 0, 255)
money:SetPoint('TOPLEFT', root, 'TOPLEFT', 53, 399)
local function refreshMoney() money:SetText(commas(GetMoney())) end

-- Item drag-swap: record the pressed slot, act on the released slot (no physical frame move).
-- hoverSlot tracks the slot under the cursor so the OnUpdate pump can re-assert its tooltip each frame.
local dragFrom = nil
local hoverSlot = nil
local slots = {}
for i = 1, NUM do
	local col  = (i - 1) % COLS
	local rowi = math.floor((i - 1) / COLS)
	local s = EmberUI.CreateItemButton(root, i, SIZE)
	s.frame:SetPoint('TOPLEFT', root, 'TOPLEFT', OX + col * PITCH, OY + rowi * PITCH)
	s.frame:EnableMouse(true)
	-- Press to pick up (grey the slot + put the icon on the cursor), release over a slot to drop/move.
	s.frame:SetScript('OnMouseDown', function(_, btn)
		if btn ~= 'LeftButton' then return end
		local id = GetContainerItem(i)
		if id == 0 then return end                  -- nothing to pick up from an empty slot
		dragFrom = i
		local _, tex = GetItemInfo(id)
		EmberUI.PickupItem(tex, s.icon)             -- grey this slot + put the icon on the cursor
	end)
	s.frame:SetScript('OnMouseUp', function(_, btn)
		if btn == 'RightButton' then
			UseContainerItem(i)
		elseif btn == 'LeftButton' then
			if dragFrom and dragFrom ~= i then MoveContainerItem(dragFrom, i) end
			dragFrom = nil
			EmberUI.ClearCursor()
		end
	end)
	s.frame:SetScript('OnEnter', function() hoverSlot = i end)
	s.frame:SetScript('OnLeave', function() if hoverSlot == i then hoverSlot = nil end end)
	slots[i] = s
end

local function refresh()
	for i = 1, NUM do
		local id, count = GetContainerItem(i)
		slots[i].SetItem(id, count)
	end
	refreshMoney()
end

-- Mirror the C++ bag's toggle state.
local shown = false
local function setShown(v)
	if v == shown then return end
	shown = v
	if v then root:Show(); refresh() else root:Hide(); EmberUI.ClearCursor() end   -- closing drops any held item
end

local stage = CreateFrame('Frame', nil, nil)
stage:SetScript('OnEvent', function(_, event)
	if event == Events.WORLD_SHOWN then
		SetGameFrameShown('InventoryFrame', false)   -- retire the C++ bag; we render it now
	elseif event == Events.BAG_UPDATE then
		if shown then refresh() end
	elseif event == Events.PLAYER_MONEY then
		if shown then refreshMoney() end
	end
end)
stage:RegisterEvent(Events.WORLD_SHOWN)
stage:RegisterEvent(Events.BAG_UPDATE)
stage:RegisterEvent(Events.PLAYER_MONEY)

local poll = CreateFrame('Frame', nil, nil)
local acc = 0
poll:SetScript('OnUpdate', function(_, dt)
	if not EmberUI.inWorld then if shown then setShown(false) end return end
	acc = acc + dt
	if acc >= 0.15 then acc = 0; setShown(IsGameFrameShown('InventoryFrame')) end
	-- re-assert the hovered item's tooltip every frame (the Application clears tooltips per frame).
	-- guarded so a /reload on an exe without ShowItemTooltip (pre-relink) doesn't error on hover.
	if shown and hoverSlot and ShowItemTooltip then ShowItemTooltip(hoverSlot) end
end)

print('EmberUI inventory loaded')
