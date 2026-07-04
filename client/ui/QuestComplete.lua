-- Lua quest-complete (turn-in) dialog, replacing the C++ QuestComplete visuals (force-hidden, mirrored).
-- Populated (setForQuest) BEFORE PANEL_OPENED; reads GetQuestCompleteInfo + the shared rewards block (with
-- CHOICE selection when the quest has choice rewards) and drives CompleteQuest(choiceSlot). Picking no
-- reward when one is required shows the C++ "must choose" error (validated engine-side). Positions from the
-- C++ ctor (panel 365,20; title 78,95; desc 55,135; rewards 95,420; complete 145,500) -- tune live.

local PANEL_X, PANEL_Y = 365, 20

local W, H = GetTextureSize('questcomplete.png')
if W <= 0 then W, H = 440, 560 end
local root = CreateFrame('Frame', 'EmberQuestComplete', nil)
root:SetSize(W, H)
root:SetPoint('TOPLEFT', PANEL_X, PANEL_Y)
root:Hide()

local bg = root:CreateTexture()
bg:SetAllPoints(root)
bg:SetTexture('questcomplete.png')

local title = root:CreateFontString()
title:SetFont('Palatino'); title:SetFontSize(16); EmberUI.SetColor(title, EmberUI.Color.Title)
title:SetPoint('TOPLEFT', root, 'TOPLEFT', 78, 95)

local desc = root:CreateFontString()
desc:SetFont('Palatino'); desc:SetFontSize(14); EmberUI.SetColor(desc, EmberUI.Color.Body)
desc:SetPoint('TOPLEFT', root, 'TOPLEFT', 55, 135); desc:SetWidth(330)

local chooseHint = root:CreateFontString()
chooseHint:SetFont('Palatino'); chooseHint:SetFontSize(13); EmberUI.SetColor(chooseHint, EmberUI.Color.Title)
chooseHint:SetPoint('TOPLEFT', root, 'TOPLEFT', 55, 395)
chooseHint:SetText('Choose your reward:')

local rewards = EmberUI.CreateQuestRewards(root, 60, 420)

-- Complete (label printed on the art; highlight/click region at the C++ spot)
local complete = CreateFrame('Button', nil, root)
complete:SetTexture('generic_highlight_idle.png'); complete:SetHoverTexture('generic_highlight_hover.png')
local bw, bh = GetTextureSize('generic_highlight_idle.png')
complete:SetSize(bw > 0 and bw or 130, bh > 0 and bh or 28)
complete:SetPoint('TOPLEFT', root, 'TOPLEFT', 145, 500); complete:EnableMouse(true)
complete:SetScript('OnClick', function()
	-- choice = the ORIGINAL 1-based reward slot; nil/0 = none (engine errors if a choice was required)
	CompleteQuest(rewards.GetChoice() or 0)
end)
local completeFS = root:CreateFontString()
completeFS:SetFont('Palatino'); completeFS:SetFontSize(14); completeFS:SetTextColor(189, 166, 145, 255)
completeFS:SetPoint('TOPLEFT', root, 'TOPLEFT', 145 + 36, 500 + 6)
completeFS:SetText('Complete')

local function refresh()
	local questId, titleStr, descStr = GetQuestCompleteInfo()
	if questId == 0 then return end
	title:SetText(titleStr)
	desc:SetText(descStr)
	local choosing = QuestCompleteNeedsChoice()
	rewards.SetQuest(questId, choosing)
	if choosing then chooseHint:Show() else chooseHint:Hide() end
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
		SetGameFrameShown('QuestCompleteFrame', false)
	elseif event == Events.PANEL_OPENED then
		if arg == 'QuestCompleteFrame' then setShown(true) end
	elseif event == Events.PANEL_CLOSED then
		if arg == 'QuestCompleteFrame' then setShown(false) end
	else
		setShown(false)
	end
end)
for _, e in ipairs({ Events.WORLD_SHOWN, Events.PANEL_OPENED, Events.PANEL_CLOSED,
                     Events.LOGIN_SHOWN, Events.CHARSELECT_SHOWN, Events.CHARCREATE_SHOWN }) do
	stage:RegisterEvent(e)
end

print('EmberUI quest complete loaded')
