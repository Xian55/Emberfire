-- Lua quest-offer dialog, rebuilt on the shared 9-slice window frame (EmberUI.CreateWindow) instead of the
-- baked quest.png background. The frame + header rule are now vector + tiny slices, so the panel stays crisp
-- at any UI scale and quest.png is retired. The C++ QuestOffer visuals stay force-hidden/mirrored; this view
-- reads GetQuestOfferInfo + the shared rewards block and drives AcceptQuestOffer / DeclineQuestOffer.
-- Pixel positions are starting points -- tune live via /reload + screenshot (same convention as the other panels).

local PANEL_X, PANEL_Y = 365, 20
local W, H = 440, 560

local win = EmberUI.CreateWindow{ name = 'EmberQuestOffer', width = W, height = H, movable = false }
local root = win.frame
root:SetPoint('TOPLEFT', PANEL_X, PANEL_Y)
root:Hide()

-- Header: the quest title (dynamic) + a rule under it, replacing the baked title banner.
local title = root:CreateFontString()
title:SetFont('Ringbearer'); title:SetFontSize(18); EmberUI.SetColor(title, EmberUI.Color.Title)
title:SetPoint('TOPLEFT', root, 'TOPLEFT', 28, 22); title:SetWidth(W - 56)
win.AddDivider(52, 24, W - 48)

local desc = root:CreateFontString()
desc:SetFont('Palatino'); desc:SetFontSize(14); EmberUI.SetColor(desc, EmberUI.Color.Body)
desc:SetPoint('TOPLEFT', root, 'TOPLEFT', 28, 68); desc:SetWidth(W - 56)

local objBanner = root:CreateFontString()
objBanner:SetFont('Ringbearer'); objBanner:SetFontSize(15); EmberUI.SetColor(objBanner, EmberUI.Color.Title)
objBanner:SetPoint('TOPLEFT', root, 'TOPLEFT', 28, 300)
objBanner:SetText('Objectives')

local obj = root:CreateFontString()
obj:SetFont('Palatino'); obj:SetFontSize(12); EmberUI.SetColor(obj, EmberUI.Color.Body)
obj:SetPoint('TOPLEFT', root, 'TOPLEFT', 28, 324); obj:SetWidth(W - 56)

local rewards = EmberUI.CreateQuestRewards(root, 34, 410)

-- Accept: reuse the generic highlight sprite pair + a label (unchanged behaviour).
local accept = CreateFrame('Button', nil, root)
accept:SetTexture('generic_highlight_idle.png'); accept:SetHoverTexture('generic_highlight_hover.png')
local bw, bh = GetTextureSize('generic_highlight_idle.png')
local abw, abh = (bw > 0 and bw or 130), (bh > 0 and bh or 28)
accept:SetSize(abw, abh)
accept:SetPoint('TOPLEFT', root, 'TOPLEFT', (W - abw) / 2, H - 54); accept:EnableMouse(true)
accept:SetScript('OnClick', function() AcceptQuestOffer() end)
local acceptFS = root:CreateFontString()
acceptFS:SetFont('Palatino'); acceptFS:SetFontSize(14); acceptFS:SetTextColor(189, 166, 145, 255)
acceptFS:SetPoint('TOPLEFT', accept, 'TOPLEFT', 42, 6)
acceptFS:SetText('Accept')

local function refresh()
	local questId, titleStr, descStr, objStr = GetQuestOfferInfo()
	if questId == 0 then return end
	title:SetText(titleStr)
	desc:SetText(descStr)
	obj:SetText(objStr)
	rewards.SetQuest(questId, false)
end

local shown = false
local function setShown(v)
	if v == shown then return end
	shown = v
	if v then root:Show(); refresh() else root:Hide(); rewards.Hide() end
end

local stage = CreateFrame('Frame', nil, nil)
stage:SetScript('OnEvent', function(_, event, arg)
	if event == Events.WORLD_SHOWN then
		SetGameFrameShown('QuestOfferFrame', false)
	elseif event == Events.PANEL_OPENED then
		if arg == 'QuestOfferFrame' then setShown(true) end
	elseif event == Events.PANEL_CLOSED then
		if arg == 'QuestOfferFrame' then setShown(false) end
	else
		setShown(false)
	end
end)
for _, e in ipairs({ Events.WORLD_SHOWN, Events.PANEL_OPENED, Events.PANEL_CLOSED,
                     Events.LOGIN_SHOWN, Events.CHARSELECT_SHOWN, Events.CHARCREATE_SHOWN }) do
	stage:RegisterEvent(e)
end

print('EmberUI quest offer loaded (9-slice)')
