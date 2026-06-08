-- Lua Trade window: dual-sided (your offer | partner's offer) replacing the C++ TradeWindow (force-hidden,
-- mirrored). The live window stays the model (re-populated by GP_Server_TradeUpdate on every change); this view
-- reads it via GetTradeItem/GetTradeGold/IsTradeReady and drives add/remove/confirm/cancel. Add = drag a bag
-- item onto a LOCAL slot (AddTradeItem); remove = right-click a local slot (RemoveTradeItem by itemGuid).
-- Event-driven: PANEL_OPENED/CLOSED('TradeFrame') show/hide, TRADE_UPDATE refresh. NO OnUpdate.
-- NOTE: positions are C++-derived guesses (TradeWindow). v1 limits: gold-SET needs a number prompt (deferred,
-- display-only); portraits deferred.

local SLOTS, SLOT_Y0, SLOT_PITCH, ICON = 5, 150, 66, 40
local LOCAL_X, REMOTE_X = 33, 226
local NAME_LX, NAME_RX, NAME_Y = 100, 290, 64
local GOLD_LX, GOLD_RX, GOLD_Y = 45, 251, 470
local READY_LX, READY_RX, READY_Y = 33, 226, 120

local W, H = GetTextureSize('trade.png')
if W <= 0 then W, H = 365, 545 end
local root = CreateFrame('Frame', 'EmberTrade', nil)
root:SetSize(W, H)
root:SetPoint('CENTER')
root:SetMovable(true); root:RegisterForDrag('LeftButton')
root:Hide()

local function vis(f, v) if v then f:Show() else f:Hide() end end
local function texSize(name, dw, dh) local w, h = GetTextureSize(name); if w <= 0 then return dw, dh end return w, h end
local function commas(n)
	local s = tostring(n)
	return (s:reverse():gsub('(%d%d%d)', '%1,'):reverse():gsub('^,', ''))
end

local bg = root:CreateTexture()
bg:SetAllPoints(root)
bg:SetTexture('trade.png')

local function fs(font, size, r, g, b, x, y)
	local f = root:CreateFontString(); f:SetFont(font); f:SetFontSize(size); f:SetTextColor(r, g, b, 255)
	f:SetPoint('TOPLEFT', root, 'TOPLEFT', x, y); return f
end

local localNameFS  = fs('Palatino', 14, 124, 114, 101, NAME_LX, NAME_Y)
local remoteNameFS = fs('Palatino', 14, 124, 114, 101, NAME_RX, NAME_Y)
local localGoldFS  = fs('Palatino', 14, 255, 215, 0, GOLD_LX, GOLD_Y)
local remoteGoldFS = fs('Palatino', 14, 255, 215, 0, GOLD_RX, GOLD_Y)
local localReadyFS = fs('Ringbearer', 13, 120, 220, 120, READY_LX, READY_Y)
local remoteReadyFS = fs('Ringbearer', 13, 120, 220, 120, READY_RX, READY_Y)

local refresh   -- forward decl

-- ---- item slots: 5 local (interactive) + 5 remote (display) ----
local localSlots, remoteSlots = {}, {}
for i = 1, SLOTS do
	local y = SLOT_Y0 + (i - 1) * SLOT_PITCH
	-- local (our offer): drag a bag item on to add; right-click to remove
	local ls = EmberUI.CreateItemButton(root, i, ICON)
	ls.frame:SetPoint('TOPLEFT', root, 'TOPLEFT', LOCAL_X, y); ls.frame:EnableMouse(true)
	ls.frame:SetTooltipTrade(i, true)
	ls.frame:SetScript('OnMouseUp', function(_, btn)
		if btn == 'RightButton' then
			if ls._guid and ls._guid ~= 0 then RemoveTradeItem(ls._guid) end
		elseif btn == 'LeftButton' then
			local p = EmberUI.HeldPayload and EmberUI.HeldPayload()
			if p and p.from == 'bag' then AddTradeItem(p.slot) end
			EmberUI.ClearCursor()
		end
	end)
	localSlots[i] = ls
	-- remote (their offer): display only
	local rs = EmberUI.CreateItemButton(root, i, ICON)
	rs.frame:SetPoint('TOPLEFT', root, 'TOPLEFT', REMOTE_X, y); rs.frame:EnableMouse(true)
	rs.frame:SetTooltipTrade(i, false)
	remoteSlots[i] = rs
end

-- ---- Trade (confirm) / Cancel buttons ----
local function labelButton(x, y, label, onClick)
	local bw, bh = texSize('generic_highlight_idle.png', 96, 26)
	local b = CreateFrame('Button', nil, root)
	b:SetTexture('generic_highlight_idle.png'); b:SetHoverTexture('generic_highlight_hover.png')
	b:SetSize(bw, bh); b:SetPoint('TOPLEFT', root, 'TOPLEFT', x, y); b:EnableMouse(true); b:SetScript('OnClick', onClick)
	local t = root:CreateFontString(); t:SetFont('Arial'); t:SetFontSize(13); t:SetTextColor(255, 255, 255, 255)
	t:SetPoint('TOPLEFT', root, 'TOPLEFT', x + 14, y + 6); t:SetText(label)
	return b
end
labelButton(250, 512, 'Trade',  function() ConfirmTrade() end)
labelButton(35,  512, 'Cancel', function() CancelTrade() end)

refresh = function()
	localNameFS:SetText('You')
	remoteNameFS:SetText(GetTradePartnerName() or '')
	localGoldFS:SetText(commas(GetTradeGold(true)) .. 'g')
	remoteGoldFS:SetText(commas(GetTradeGold(false)) .. 'g')
	localReadyFS:SetText(IsTradeReady(true) and 'Accepted' or '')
	remoteReadyFS:SetText(IsTradeReady(false) and 'Accepted' or '')

	for i = 1, SLOTS do
		local id, count, guid = GetTradeItem(true, i)
		localSlots[i].SetItem(id, count); localSlots[i]._guid = guid
		local rid, rcount = GetTradeItem(false, i)
		remoteSlots[i].SetItem(rid, rcount)
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
		SetGameFrameShown('TradeFrame', false)
	elseif event == Events.PANEL_OPENED then
		if arg == 'TradeFrame' then setShown(true) end
	elseif event == Events.PANEL_CLOSED then
		if arg == 'TradeFrame' then setShown(false) end
	elseif event == Events.TRADE_UPDATE then
		if shown then refresh() end
	else
		setShown(false)
	end
end)
for _, e in ipairs({ Events.WORLD_SHOWN, Events.PANEL_OPENED, Events.PANEL_CLOSED, Events.TRADE_UPDATE,
                     Events.LOGIN_SHOWN, Events.CHARSELECT_SHOWN, Events.CHARCREATE_SHOWN }) do
	stage:RegisterEvent(e)
end

print('EmberUI trade loaded')
