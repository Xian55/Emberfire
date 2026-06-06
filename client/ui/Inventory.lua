-- Lua Inventory: a bag grid that REPLACES the C++ Inventory window. The C++ window is force-hidden in-world
-- but still toggled by its open button; this view MIRRORS that toggle state (IsGameFrameShown) and renders
-- bag contents from the data getters. Left-drag a slot onto another = MoveContainerItem; right-click = Use.
--
-- Hover = item tooltip; right-click = use/equip; unusable items tint red; drag out of the bag = destroy
-- confirm; money shown at the panel spot.
-- v1 deviations (tune in the live client): no quality border (set EmberUI._slotBg to skin the slots); fixed
-- CENTER position; red is an icon tint (no solid-overlay primitive); no vendor/bank right-click branches.

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

-- Forward-declared so the handlers created below (before the loop + refresh) capture the right upvalues.
-- dragFrom = a left-drag move in progress; applyFrom = a target-an-item consumable awaiting a target click.
local dragFrom, applyFrom, hoverSlot, refresh
local slots = {}

-- releasing a held item over the backdrop (not a slot) returns it to its source / cancels targeting.
root:SetScript('OnMouseUp', function(_, btn)
	if btn == 'LeftButton' and EmberUI.HasCursorItem() then
		dragFrom = nil; applyFrom = nil; EmberUI.ClearCursor(); if refresh then refresh() end
	end
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
for i = 1, NUM do
	local col  = (i - 1) % COLS
	local rowi = math.floor((i - 1) / COLS)
	local s = EmberUI.CreateItemButton(root, i, SIZE)
	s.frame:SetPoint('TOPLEFT', root, 'TOPLEFT', OX + col * PITCH, OY + rowi * PITCH)
	s.frame:EnableMouse(true)
	-- Press to pick up (grey the slot + put the icon on the cursor), release over a slot to drop/move.
	s.frame:SetScript('OnMouseDown', function(_, btn)
		if btn ~= 'LeftButton' then return end
		if applyFrom then return end                -- targeting mode: left-press picks a target, not a drag
		local id = GetContainerItem(i)
		if id == 0 then return end                  -- nothing to pick up from an empty slot
		dragFrom = i
		local _, tex = GetItemInfo(id)
		EmberUI.PickupItem(tex, s.icon)             -- grey this slot + put the icon on the cursor
	end)
	s.frame:SetScript('OnMouseUp', function(_, btn)
		if btn == 'RightButton' then
			if applyFrom then                                       -- right-click again cancels targeting
				applyFrom = nil; EmberUI.ClearCursor(); refresh()
			elseif ContainerItemTargetsItem and ContainerItemTargetsItem(i) then
				local id = GetContainerItem(i)
				if id ~= 0 then                                     -- gem/orb: enter targeting (orb on cursor)
					applyFrom = i
					local _, tex = GetItemInfo(id)
					EmberUI.PickupItem(tex, s.icon)
				end
			elseif UseOrEquipContainerItem then
				UseOrEquipContainerItem(i)
			else
				UseContainerItem(i)
			end
		elseif btn == 'LeftButton' then
			if applyFrom then                                       -- apply the targeting item to this slot
				if GetContainerItem(i) ~= 0 and UseContainerItemOnItem then UseContainerItemOnItem(applyFrom, i) end
				applyFrom = nil; EmberUI.ClearCursor(); refresh()
			else
				if dragFrom and dragFrom ~= i then MoveContainerItem(dragFrom, i) end
				dragFrom = nil
				EmberUI.ClearCursor()
				refresh()   -- restore tints (ClearCursor reset the dragged icon to white)
			end
		end
	end)
	s.frame:SetScript('OnEnter', function() hoverSlot = i end)
	s.frame:SetScript('OnLeave', function() if hoverSlot == i then hoverSlot = nil end end)
	slots[i] = s
end

refresh = function()
	for i = 1, NUM do
		local id, count = GetContainerItem(i)
		slots[i].SetItem(id, count)
		if id ~= 0 then
			if IsContainerItemUsable and not IsContainerItemUsable(i) then
				slots[i].icon:SetVertexColor(255, 90, 90, 255)    -- unusable (level/class/req not met): red tint
			else
				slots[i].icon:SetVertexColor(255, 255, 255, 255)
			end
		end
	end
	refreshMoney()
end

-- Mirror the C++ bag's toggle state.
local shown = false
local function setShown(v)
	if v == shown then return end
	shown = v
	if v then root:Show(); refresh() else root:Hide(); dragFrom = nil; applyFrom = nil; EmberUI.ClearCursor() end   -- closing drops any held item
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
local wasDown = false
local pendingDestroy = nil   -- { code, slot } while a destroy confirm is open
poll:SetScript('OnUpdate', function(_, dt)
	if not EmberUI.inWorld then if shown then setShown(false) end return end
	acc = acc + dt
	if acc >= 0.15 then acc = 0; setShown(IsGameFrameShown('InventoryFrame')) end
	-- re-assert the hovered item's tooltip every frame (the Application clears tooltips per frame).
	-- guarded so a /reload on an exe without ShowItemTooltip (pre-relink) doesn't error on hover.
	if shown and hoverSlot and ShowItemTooltip then ShowItemTooltip(hoverSlot) end

	-- Holding an item + LEFT released OUTSIDE the bag => destroy confirm (reuses the C++ ConfirmMessageBox).
	-- (This OnUpdate runs before the mouse-event drain, so over-slot/over-bag releases — where IsMouseOver is
	-- true — are skipped here and handled by the slot/backdrop OnMouseUp instead.)
	if IsMouseButtonDown then
		local down = IsMouseButtonDown('LeftButton')
		if wasDown and not down and EmberUI.HasCursorItem() and not root:IsMouseOver() then
			local slot = dragFrom                      -- only a MOVE (dragFrom) can destroy; targeting just cancels
			dragFrom = nil; applyFrom = nil
			EmberUI.ClearCursor(); refresh()
			if slot and ShowConfirm then
				pendingDestroy = { code = ShowConfirm('This will DESTROY the item. Accept?'), slot = slot }
			end
		end
		wasDown = down
	end
	if pendingDestroy and PopConfirm then
		local code, yes = PopConfirm()
		if code ~= 0 and code == pendingDestroy.code then
			if yes then DestroyContainerItem(pendingDestroy.slot) end
			pendingDestroy = nil
		end
	end
end)

print('EmberUI inventory loaded')
