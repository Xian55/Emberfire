-- Lua quest-offer dialog, replacing the C++ QuestOffer visuals (force-hidden, mirrored). The C++ panel is
-- populated (setForQuest) BEFORE PANEL_OPENED fires, so this view reads GetQuestOfferInfo + the shared
-- rewards block and drives AcceptQuestOffer / DeclineQuestOffer. Positions from the C++ ctor (panel 365,20;
-- title 78,95; desc 55,135; rewards 95,420; accept 145,500); objectives use a fixed Y (C++ derives it from
-- the description height) -- tune live.

local PANEL_X, PANEL_Y = 365, 20

local W, H = GetTextureSize('quest.png')
if W <= 0 then W, H = 440, 560 end
local root = CreateFrame('Frame', 'EmberQuestOffer', nil)
root:SetSize(W, H)
root:SetPoint('TOPLEFT', PANEL_X, PANEL_Y)
root:Hide()

local bg = root:CreateTexture()
bg:SetAllPoints(root)
bg:SetTexture('quest.png')

local title = root:CreateFontString()
title:SetFont('Palatino'); title:SetFontSize(16); title:SetTextColor(189, 166, 145, 255)
title:SetPoint('TOPLEFT', root, 'TOPLEFT', 78, 95)

local desc = root:CreateFontString()
desc:SetFont('Palatino'); desc:SetFontSize(14); desc:SetTextColor(142, 119, 98, 255)
desc:SetPoint('TOPLEFT', root, 'TOPLEFT', 55, 135); desc:SetWidth(330)

local objBanner = root:CreateFontString()
objBanner:SetFont('Palatino'); objBanner:SetFontSize(15); objBanner:SetTextColor(189, 166, 145, 255)
objBanner:SetPoint('TOPLEFT', root, 'TOPLEFT', 55, 310)
objBanner:SetText('Objectives')

local obj = root:CreateFontString()
obj:SetFont('Palatino'); obj:SetFontSize(12); obj:SetTextColor(142, 119, 98, 255)
obj:SetPoint('TOPLEFT', root, 'TOPLEFT', 55, 335); obj:SetWidth(330)

local rewards = EmberUI.CreateQuestRewards(root, 60, 420)

-- Accept (label printed on the art; highlight/click region at the C++ spot)
local accept = CreateFrame('Button', nil, root)
accept:SetTexture('generic_highlight_idle.png'); accept:SetHoverTexture('generic_highlight_hover.png')
local bw, bh = GetTextureSize('generic_highlight_idle.png')
accept:SetSize(bw > 0 and bw or 130, bh > 0 and bh or 28)
accept:SetPoint('TOPLEFT', root, 'TOPLEFT', 145, 500); accept:EnableMouse(true)
accept:SetScript('OnClick', function() AcceptQuestOffer() end)
local acceptFS = root:CreateFontString()
acceptFS:SetFont('Palatino'); acceptFS:SetFontSize(14); acceptFS:SetTextColor(189, 166, 145, 255)
acceptFS:SetPoint('TOPLEFT', root, 'TOPLEFT', 145 + 42, 500 + 6)
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

print('EmberUI quest offer loaded')
