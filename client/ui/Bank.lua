-- Lua Bank: 49-slot grid replacing the C++ Bank (force-hidden, mirrored). Same Model=C++/View=Lua pattern as
-- Inventory: the live Bank stays the model (populated by GP_Server_Bank), this view reads it via GetBankItem
-- and drives it via WithdrawBankItem/MoveBankItem/DepositBagItem/SortBank. Right-click a slot = withdraw
-- (auto-pick inventory slot); drag bank<->bank = move; drag a bag item onto a slot = deposit (bag->bank also
-- works by right-clicking the bag item via the Inventory window's MerchantRightClick). Tooltips engine-asserted.
-- Event-driven: PANEL_OPENED/CLOSED('BankFrame') show/hide, BANK_UPDATE refreshes. NO OnUpdate.
-- NOTE: grid geometry copied from C++ Bank::attachIcon (first slot 27,75; 45px pitch; 7 cols). Tune live.

local COLS, SIZE, PITCH = 7, 40, 45
local OX, OY = 27, 75
local NUM = GetBankNumSlots()

local W, H = GetTextureSize('bank.png')
if W <= 0 then W, H = 330, 420 end
local root = CreateFrame('Frame', 'EmberBank', nil)
root:SetSize(W, H)
root:SetPoint('CENTER')
root:SetMovable(true); root:RegisterForDrag('LeftButton')
root:Hide()

local function vis(f, v) if v then f:Show() else f:Hide() end end
local function texSize(name, dw, dh) local w, h = GetTextureSize(name); if w <= 0 then return dw, dh end return w, h end

local refresh                 -- forward decl
local slots = {}

-- Release a held item over the backdrop (not a slot) cancels / returns it.
root:SetScript('OnMouseUp', function(_, btn)
	if btn == 'LeftButton' and EmberUI.HasCursorItem() then EmberUI.ClearCursor(); if refresh then refresh() end end
end)

local bg = root:CreateTexture()
bg:SetAllPoints(root)
bg:SetTexture('bank.png')

for i = 1, NUM do
	local col  = (i - 1) % COLS
	local rowi = math.floor((i - 1) / COLS)
	local s = EmberUI.CreateItemButton(root, i, SIZE)
	s.frame:SetPoint('TOPLEFT', root, 'TOPLEFT', OX + col * PITCH, OY + rowi * PITCH)
	s.frame:EnableMouse(true)
	s.frame:SetTooltipBank(i)
	s.frame:SetScript('OnMouseDown', function(_, btn)
		if btn ~= 'LeftButton' then return end
		local id = GetBankItem(i)
		if id == 0 then return end
		local _, tex = GetItemInfo(id)
		EmberUI.PickupItem(tex, s.icon, { from = 'bank', slot = i })
	end)
	s.frame:SetScript('OnMouseUp', function(_, btn)
		if btn == 'RightButton' then
			if GetBankItem(i) ~= 0 then WithdrawBankItem(i) end       -- withdraw to an auto-picked bag slot
		elseif btn == 'LeftButton' then
			local p = EmberUI.HeldPayload and EmberUI.HeldPayload()
			if p and p.from == 'bank' then MoveBankItem(p.slot, i)    -- bank -> bank
			elseif p and p.from == 'bag' then DepositBagItem(p.slot, i) end  -- bag -> bank
			EmberUI.ClearCursor(); refresh()
		end
	end)
	slots[i] = s
end

-- Sort button (C++ Bank places "sort_inventory" at 257,390).
local SW, SH = texSize('sort_inventory_idle.png', 24, 24)
local sortBtn = CreateFrame('Button', nil, root)
sortBtn:SetTexture('sort_inventory_idle.png'); sortBtn:SetHoverTexture('sort_inventory_hover.png')
sortBtn:SetSize(SW, SH)
sortBtn:SetPoint('TOPLEFT', root, 'TOPLEFT', 257, 390)
sortBtn:EnableMouse(true)
sortBtn:SetScript('OnClick', function() SortBank() end)

-- close button (guarantees the bank is always closable, regardless of the C++ dialog-close path)
local CW, CH = texSize('panel_close_small_idle.png', 16, 16)
local closeBtn = CreateFrame('Button', nil, root)
closeBtn:SetTexture('panel_close_small_idle.png'); closeBtn:SetHoverTexture('panel_close_small_hover.png')
closeBtn:SetSize(CW, CH)
closeBtn:SetPoint('TOPLEFT', root, 'TOPLEFT', W - 22, 10)
closeBtn:EnableMouse(true)
closeBtn:SetScript('OnClick', function() CloseBank() end)

refresh = function()
	for i = 1, NUM do
		local id, count = GetBankItem(i)
		slots[i].SetItem(id, count)
	end
end

local shown = false
local function setShown(v)
	if v == shown then return end
	shown = v
	if v then root:Show(); refresh() else root:Hide(); EmberUI.ClearCursor() end
end

local stage = CreateFrame('Frame', nil, nil)
stage:SetScript('OnEvent', function(_, event, arg)
	if event == Events.WORLD_SHOWN then
		SetGameFrameShown('BankFrame', false)   -- retire the C++ bank window
	elseif event == Events.PANEL_OPENED then
		if arg == 'BankFrame' then setShown(true) end
	elseif event == Events.PANEL_CLOSED then
		if arg == 'BankFrame' then setShown(false) end
	elseif event == Events.BANK_UPDATE then
		if shown then refresh() end   -- only refresh; the server pre-populates the bank on LOGIN, so do NOT
		                              -- auto-show here. The bank shows ONLY via PANEL_OPENED (real OpenBank).
	else
		setShown(false)                          -- left the world
	end
end)
for _, e in ipairs({ Events.WORLD_SHOWN, Events.PANEL_OPENED, Events.PANEL_CLOSED, Events.BANK_UPDATE,
                     Events.LOGIN_SHOWN, Events.CHARSELECT_SHOWN, Events.CHARCREATE_SHOWN }) do
	stage:RegisterEvent(e)
end

print('EmberUI bank loaded')
