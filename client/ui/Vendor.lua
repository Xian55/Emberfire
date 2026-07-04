-- Lua Vendor: item grid replacing the C++ VendorNpc (force-hidden, mirrored). The live window stays the model
-- (populated by GP_Server_GossipMenu vendor items); this view reads the FULL list via GetVendorItem and scrolls
-- it itself (no C++ pagination). Buy (with a confirm), Repair, Buyback. Money = the player's money (GetMoney).
-- Event-driven: PANEL_OPENED/CLOSED('VendorFrame') show/hide, VENDOR_UPDATE refresh. NO OnUpdate.
-- NOTE: 2-column layout + positions are C++-derived guesses (VendorNpc) — tune live.

local COLS, ROWS_PER_COL = 2, 6
local VISIBLE = COLS * ROWS_PER_COL
local ICON_X = { 33, 226 }
local NAME_DX, COST_DX, CELL_Y0, CELL_PITCH, ICON = 50, 64, 83, 66, 38
local MONEY_X, MONEY_Y = 325, 519

local W, H = 428, 568
local win = EmberUI.CreateWindow{ name = 'EmberVendor', width = W, height = H, title = 'Merchant' }
local root = win.frame
root:SetPoint('CENTER')
root:Hide()

local function vis(f, v) if v then f:Show() else f:Hide() end end
local function texSize(name, dw, dh) local w, h = GetTextureSize(name); if w <= 0 then return dw, dh end return w, h end
local function commas(n)
	local s = tostring(n)
	return (s:reverse():gsub('(%d%d%d)', '%1,'):reverse():gsub('^,', ''))
end

local moneyFS = root:CreateFontString()
moneyFS:SetFont('Palatino'); moneyFS:SetFontSize(14); moneyFS:SetTextColor(255, 215, 0, 255)
moneyFS:SetPoint('TOPLEFT', root, 'TOPLEFT', MONEY_X, MONEY_Y)

local scroll = 0
local refresh   -- forward decl

-- ---- vendor cells (icon + name + cost) ----
local cells = {}
for c = 1, VISIBLE do
	local col = math.floor((c - 1) / ROWS_PER_COL)
	local row = (c - 1) % ROWS_PER_COL
	local x = ICON_X[col + 1]
	local y = CELL_Y0 + row * CELL_PITCH
	local s = EmberUI.CreateItemButton(root, c, ICON)
	s.frame:SetPoint('TOPLEFT', root, 'TOPLEFT', x, y)
	s.frame:EnableMouse(true)
	local nameFS = root:CreateFontString(); nameFS:SetFont('Constantia'); nameFS:SetFontSize(12); nameFS:SetTextColor(255, 255, 255, 255)
	nameFS:SetPoint('TOPLEFT', root, 'TOPLEFT', x + NAME_DX, y)
	local costFS = root:CreateFontString(); costFS:SetFont('Constantia'); costFS:SetFontSize(12); costFS:SetTextColor(255, 215, 0, 255)
	costFS:SetPoint('TOPLEFT', root, 'TOPLEFT', x + COST_DX, y + 18)
	local cell = { item = s, name = nameFS, cost = costFS }
	s.frame:SetScript('OnMouseUp', function(_, btn)
		if btn ~= 'LeftButton' or not cell._idx then return end
		local nm = cell._name or 'item'
		local cst = cell._cost or 0
		EmberUI.ShowMenu({
			{ text = 'Buy ' .. nm .. '?' },
			{ text = 'Buy 1 for ' .. commas(cst) .. 'g', color = { 120, 220, 120 }, onClick = function() BuyVendorItem(cell._idx, 1) end },
			{ text = 'Cancel' },
		})
	end)
	cells[c] = cell
end

-- ---- Repair / Buyback ----
local function labelButton(idle, hover, x, y, label, onClick)
	local bw, bh = texSize(idle, 84, 26)
	local b = CreateFrame('Button', nil, root)
	b:SetTexture(idle); b:SetHoverTexture(hover); b:SetSize(bw, bh)
	b:SetPoint('TOPLEFT', root, 'TOPLEFT', x, y); b:EnableMouse(true); b:SetScript('OnClick', onClick)
	if label then
		local fs = root:CreateFontString(); fs:SetFont('Arial'); fs:SetFontSize(12); fs:SetTextColor(255, 255, 255, 255)
		fs:SetPoint('TOPLEFT', root, 'TOPLEFT', x + 10, y + 6); fs:SetText(label)
	end
	return b
end

labelButton('generic_highlight_medium_idle.png', 'generic_highlight_medium_hover.png', 35, 512, 'Repair', function() RepairGear() end)
labelButton('undo_buyback_idle.png', 'undo_buyback_hover.png', 125, 521, nil, function() VendorBuyback() end)

refresh = function()
	moneyFS:SetText(commas(GetMoney()))
	local n = GetVendorNumItems()
	local maxS = math.max(0, n - VISIBLE)
	if scroll > maxS then scroll = maxS end
	if scroll < 0 then scroll = 0 end

	for c = 1, VISIBLE do
		local idx = scroll + c
		local cell = cells[c]
		if idx <= n then
			local itemId, _, cost = GetVendorItem(idx)
			local nm = GetItemInfo(itemId)
			cell.item.SetItem(itemId, 1)
			cell.name:SetText(nm or ''); cell.name:Show()
			cell.cost:SetText(commas(cost) .. 'g'); cell.cost:Show()
			cell.item.frame:SetTooltipVendor(idx)
			cell._idx = idx; cell._name = nm; cell._cost = cost
			vis(cell.item.frame, true)
		else
			cell.item.Clear()
			cell.item.frame:SetTooltipVendor(0)
			cell.name:Hide(); cell.cost:Hide(); cell._idx = nil
			vis(cell.item.frame, false)
		end
	end
end

root:SetScript('OnMouseWheel', function(_, delta) scroll = scroll - delta; refresh() end)

local shown = false
local function setShown(v)
	if v == shown then return end
	shown = v
	if v then root:Show(); refresh() else root:Hide(); EmberUI.HideMenu() end
end

local stage = CreateFrame('Frame', nil, nil)
stage:SetScript('OnEvent', function(_, event, arg)
	if event == Events.WORLD_SHOWN then
		SetGameFrameShown('VendorFrame', false)
	elseif event == Events.PANEL_OPENED then
		if arg == 'VendorFrame' then setShown(true) end
	elseif event == Events.PANEL_CLOSED then
		if arg == 'VendorFrame' then setShown(false) end
	elseif event == Events.VENDOR_UPDATE or event == Events.PLAYER_MONEY then
		if shown then refresh() end
	else
		setShown(false)
	end
end)
for _, e in ipairs({ Events.WORLD_SHOWN, Events.PANEL_OPENED, Events.PANEL_CLOSED, Events.VENDOR_UPDATE,
                     Events.PLAYER_MONEY, Events.LOGIN_SHOWN, Events.CHARSELECT_SHOWN, Events.CHARCREATE_SHOWN }) do
	stage:RegisterEvent(e)
end

print('EmberUI vendor loaded')
