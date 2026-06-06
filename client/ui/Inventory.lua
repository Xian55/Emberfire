-- Lua Inventory: a bag grid that REPLACES the C++ Inventory window. The C++ window is force-hidden in-world
-- but still toggled by its open button; this view MIRRORS that toggle state (IsGameFrameShown) and renders
-- bag contents from the data getters. Left-drag a slot onto another = MoveContainerItem; right-click = Use.
--
-- v1 deviations (tune in the live client): no quality border (set EmberUI._slotBg to skin the slots); fixed
-- CENTER position; no money display; no equip/sell/destroy wiring (the commands exist, add menus later).

local COLS, SIZE, PAD = 7, 40, 4
local NUM = GetContainerNumSlots()
local rows = math.ceil(NUM / COLS)
local W = COLS * (SIZE + PAD) + PAD
local H = rows * (SIZE + PAD) + PAD

-- Movable backdrop window (inventory.png is the C++ bag art — a confirmed asset).
local root = CreateFrame('Frame', 'EmberInventory', nil)
root:SetSize(W, H)
root:SetPoint('CENTER')
root:SetMovable(true); root:RegisterForDrag('LeftButton')   -- drag the window by its empty backdrop
root:Hide()

local bg = root:CreateTexture()
bg:SetAllPoints(root)
bg:SetTexture('inventory.png')

-- Item drag-swap: record the pressed slot, act on the released slot (no physical frame move).
local dragFrom = nil
local slots = {}
for i = 1, NUM do
	local col  = (i - 1) % COLS
	local rowi = math.floor((i - 1) / COLS)
	local s = EmberUI.CreateItemButton(root, i, SIZE)
	s.frame:SetPoint('TOPLEFT', root, 'TOPLEFT', PAD + col * (SIZE + PAD), PAD + rowi * (SIZE + PAD))
	s.frame:EnableMouse(true)
	s.frame:SetScript('OnMouseDown', function(_, btn) if btn == 'LeftButton' then dragFrom = i end end)
	s.frame:SetScript('OnMouseUp', function(_, btn)
		if btn == 'RightButton' then
			UseContainerItem(i)
		elseif btn == 'LeftButton' then
			if dragFrom and dragFrom ~= i then MoveContainerItem(dragFrom, i) end
			dragFrom = nil
		end
	end)
	slots[i] = s
end

local function refresh()
	for i = 1, NUM do
		local id, count = GetContainerItem(i)
		slots[i].SetItem(id, count)
	end
end

-- Mirror the C++ bag's toggle state.
local shown = false
local function setShown(v)
	if v == shown then return end
	shown = v
	if v then root:Show(); refresh() else root:Hide() end
end

local stage = CreateFrame('Frame', nil, nil)
stage:SetScript('OnEvent', function(_, event)
	if event == Events.WORLD_SHOWN then
		SetGameFrameShown('InventoryFrame', false)   -- retire the C++ bag; we render it now
	elseif event == Events.BAG_UPDATE then
		if shown then refresh() end
	end
end)
stage:RegisterEvent(Events.WORLD_SHOWN)
stage:RegisterEvent(Events.BAG_UPDATE)

local poll = CreateFrame('Frame', nil, nil)
local acc = 0
poll:SetScript('OnUpdate', function(_, dt)
	if not EmberUI.inWorld then if shown then setShown(false) end return end
	acc = acc + dt
	if acc >= 0.15 then acc = 0; setShown(IsGameFrameShown('InventoryFrame')) end
end)

print('EmberUI inventory loaded')
