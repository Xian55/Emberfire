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
local cell = EmberUI.Layout.Grid{ cols = COLS, pitch = PITCH, x = OX, y = OY }   -- i -> cell top-left

-- 9-slice window (retires inventory.png; item cells skinned via EmberUI._slotBg). Grid offsets below are
-- unchanged -- they were tuned to the real bag dims, so the frame swap keeps every slot in place.
local W, H = 364, 436
local win = EmberUI.CreateWindow{ name = 'EmberInventory', width = W, height = H, title = 'Inventory' }
local root = win.frame
root:SetPoint('CENTER')
root:Hide()

-- Forward-declared so the handlers created below (before the loop + refresh) capture the right upvalues.
-- dragFrom = a left-drag move in progress; applyFrom = a target-an-item consumable awaiting a target click.
local dragFrom, applyFrom, refresh
local slots = {}

-- releasing a held item over the backdrop (not a slot) returns it to its source / cancels targeting.
root:SetScript('OnMouseUp', function(_, btn)
	if btn == 'LeftButton' and EmberUI.HasCursorItem() then
		dragFrom = nil; applyFrom = nil; EmberUI.ClearCursor(); if refresh then refresh() end
	end
end)

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

-- Item drag-swap: record the pressed slot, act on the released slot (no physical frame move). Each slot
-- registers its item tooltip via SetTooltipItem; the engine re-asserts it while hovered (no poll here).
for i = 1, NUM do
	local s = EmberUI.CreateItemButton(root, i, SIZE)
	local gx, gy = cell(i)
	s.frame:SetPoint('TOPLEFT', root, 'TOPLEFT', gx, gy)
	s.frame:EnableMouse(true)
	s.frame:SetTooltipItem(i)   -- engine re-asserts the item tooltip while this slot is hovered
	-- Press to pick up (grey the slot + put the icon on the cursor), release over a slot to drop/move.
	s.frame:SetScript('OnMouseDown', function(_, btn)
		if btn ~= 'LeftButton' then return end
		if applyFrom then return end                -- targeting mode: left-press picks a target, not a drag
		local id = GetContainerItem(i)
		if id == 0 then return end                  -- nothing to pick up from an empty slot
		dragFrom = i
		local _, tex = GetItemInfo(id)
		EmberUI.PickupItem(tex, s.icon, { from = 'bag', slot = i })   -- grey + icon on cursor; payload for cross-window
	end)
	s.frame:SetScript('OnMouseUp', function(_, btn)
		if btn == 'RightButton' then
			if MerchantRightClick and MerchantRightClick(i) then    -- bank/trade/vendor open: move/add/SELL, not use
				applyFrom = nil; EmberUI.ClearCursor(); refresh()
			elseif applyFrom then                                   -- right-click again cancels targeting
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
				local p = EmberUI.HeldPayload and EmberUI.HeldPayload()
				if p and p.from == 'equip' and UnequipInventoryItem then
					UnequipInventoryItem(p.slot, i)             -- drop an equipped item into this bag slot
				elseif p and p.from == 'bank' and WithdrawBankItemTo then
					WithdrawBankItemTo(p.slot, i)               -- drop a bank item into this bag slot (withdraw)
				elseif dragFrom and dragFrom ~= i then
					MoveContainerItem(dragFrom, i)
				end
				dragFrom = nil
				EmberUI.ClearCursor()
				refresh()   -- restore tints (ClearCursor reset the dragged icon to white)
			end
		end
	end)
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

-- Show/hide is event-driven (PANEL_OPENED/CLOSED, WoW Show()->OnShow); data refreshes on its events; glue
-- screens hide on leaving the world. No visibility polling.
local stage = CreateFrame('Frame', nil, nil)
stage:SetScript('OnEvent', function(_, event, arg)
	if event == Events.WORLD_SHOWN then
		SetGameFrameShown('InventoryFrame', false)   -- retire the C++ bag; we render it now
	elseif event == Events.PANEL_OPENED then
		if arg == 'InventoryFrame' then setShown(true) end
	elseif event == Events.PANEL_CLOSED then
		if arg == 'InventoryFrame' then setShown(false) end
	elseif event == Events.LOGIN_SHOWN or event == Events.CHARSELECT_SHOWN or event == Events.CHARCREATE_SHOWN then
		setShown(false)   -- left the world
	elseif event == Events.BAG_UPDATE then
		if shown then refresh() end
	elseif event == Events.PLAYER_MONEY then
		if shown then refreshMoney() end
	end
end)
for _, e in ipairs({ Events.WORLD_SHOWN, Events.PANEL_OPENED, Events.PANEL_CLOSED,
                     Events.LOGIN_SHOWN, Events.CHARSELECT_SHOWN, Events.CHARCREATE_SHOWN,
                     Events.BAG_UPDATE, Events.PLAYER_MONEY }) do
	stage:RegisterEvent(e)
end

-- The only OnUpdate left here: drag-destroy is INPUT (no "released over the world" event), so it must watch
-- the mouse. Early-out unless actually holding / mid-confirm, so it's idle the rest of the time.
local poll = CreateFrame('Frame', nil, nil)
local wasDown = false
local pendingDestroy = nil   -- { code, slot } while a destroy confirm is open
local dropCheck = nil        -- { slot } a release-on-nothing pending the next-frame "was it consumed?" check
poll:SetScript('OnUpdate', function(_, dt)
	if not (EmberUI.HasCursorItem() or dropCheck or pendingDestroy) then return end

	-- Holding an item + LEFT released OUTSIDE the bag => maybe destroy. But the release might instead be a
	-- cross-window drop (e.g. onto the Equipment window), whose slot OnMouseUp runs in the mouse-event drain
	-- AFTER this OnUpdate. So we DEFER one frame: flag the release now, then next frame destroy only if the
	-- item is STILL held (nothing consumed it = truly dropped on the world). Bag-internal/over-bag releases
	-- keep IsMouseOver true and never flag here.
	if IsMouseButtonDown then
		if dropCheck then
			local pending = dropCheck; dropCheck = nil
			if EmberUI.HasCursorItem() then
				local p = EmberUI.HeldPayload and EmberUI.HeldPayload()
				dragFrom = nil; applyFrom = nil
				EmberUI.ClearCursor(); refresh()
				if pending.slot and p and p.from == 'bag' and ShowConfirm then
					pendingDestroy = { code = ShowConfirm('This will DESTROY the item. Accept?'), slot = pending.slot }
				end
			end
		end
		local down = IsMouseButtonDown('LeftButton')
		if wasDown and not down and EmberUI.HasCursorItem() and not root:IsMouseOver() then
			dropCheck = { slot = dragFrom }   -- decide next frame, after slot handlers get a chance to consume
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
