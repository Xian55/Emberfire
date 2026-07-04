-- Lua NPC gossip dialog, replacing the C++ DialogNpc visuals (force-hidden, mirrored). The C++ panel stays
-- the model: the GossipMenu packet populates it BEFORE PANEL_OPENED fires, so this view just reads
-- GetGossipNpcName/GetGossipText/GetGossipOption and drives SelectGossipOption (which runs the C++ per-type
-- flow: dialog -> op8 select, quest -> opens QuestOffer/QuestComplete, vendor -> switches panels) and
-- CloseGossip (Goodbye). Option icons mirror the C++: quest_ready/quest_done/gold pouch/gossip dot.
-- Positions from the C++ ctor (panel at 365,20; title 90,95; desc 55,135; goodbye 145,500); options use a
-- fixed Y (the C++ derives it from the description height) -- tune live.

local PANEL_X, PANEL_Y = 365, 20
local OPT_Y0, OPT_H, MAX_OPTS = 330, 22, 8

local W, H = 428, 568
local win = EmberUI.CreateWindow{ name = 'EmberDialogNpc', width = W, height = H, movable = false }
local root = win.frame
root:SetPoint('TOPLEFT', PANEL_X, PANEL_Y)
root:Hide()

local title = root:CreateFontString()
title:SetFont('Palatino'); title:SetFontSize(16); EmberUI.SetColor(title, EmberUI.Color.Title)
title:SetPoint('TOPLEFT', root, 'TOPLEFT', 90, 95)

local desc = root:CreateFontString()
desc:SetFont('Palatino'); desc:SetFontSize(14); EmberUI.SetColor(desc, EmberUI.Color.Body)
desc:SetPoint('TOPLEFT', root, 'TOPLEFT', 55, 135); desc:SetWidth(330)

local optTitle = root:CreateFontString()
optTitle:SetFont('Palatino'); optTitle:SetFontSize(15); EmberUI.SetColor(optTitle, EmberUI.Color.Title)
optTitle:SetPoint('TOPLEFT', root, 'TOPLEFT', 55, OPT_Y0 - 28)
optTitle:SetText('Options:')

local ICON_FOR = { dialog = 'gossip_option.png', quest = 'quest_ready_gossip.png',
                   complete = 'quest_done_gossip.png', vendor = 'gossip_gold_pouch.png' }

local rows = {}
for i = 1, MAX_OPTS do
	local y = OPT_Y0 + (i - 1) * OPT_H
	local btn = CreateFrame('Button', nil, root)
	btn:SetSize(290, OPT_H); btn:SetPoint('TOPLEFT', root, 'TOPLEFT', 55, y); btn:EnableMouse(true)
	btn:SetHoverColor(255, 235, 180, 30)
	btn:SetScript('OnClick', function() SelectGossipOption(i) end)
	local icon = btn:CreateTexture()
	icon:SetSize(14, 14); icon:SetPoint('TOPLEFT', btn, 'TOPLEFT', 2, 4)
	local fs = root:CreateFontString()
	fs:SetFont('Palatino'); fs:SetFontSize(13); fs:SetTextColor(189, 166, 145, 255)
	fs:SetPoint('TOPLEFT', root, 'TOPLEFT', 55 + 22, y + 3)
	rows[i] = { btn = btn, icon = icon, fs = fs }
end

-- Goodbye (the label is printed on the panel art; this is the highlight/click region at the C++ spot)
local bye = CreateFrame('Button', nil, root)
bye:SetTexture('generic_highlight_idle.png'); bye:SetHoverTexture('generic_highlight_hover.png')
local bw, bh = GetTextureSize('generic_highlight_idle.png')
bye:SetSize(bw > 0 and bw or 130, bh > 0 and bh or 28)
bye:SetPoint('TOPLEFT', root, 'TOPLEFT', 145, 500); bye:EnableMouse(true)
bye:SetScript('OnClick', function() CloseGossip() end)
local byeFS = root:CreateFontString()
byeFS:SetFont('Palatino'); byeFS:SetFontSize(14); byeFS:SetTextColor(189, 166, 145, 255)
byeFS:SetPoint('TOPLEFT', root, 'TOPLEFT', 145 + 38, 500 + 6)
byeFS:SetText('Goodbye')

local function refresh()
	title:SetText(GetGossipNpcName() .. ' says:')
	desc:SetText(GetGossipText())
	local n = GetNumGossipOptions()
	if n > 0 then optTitle:Show() else optTitle:Hide() end
	for i = 1, MAX_OPTS do
		local row = rows[i]
		if i <= n then
			local typ, _, label = GetGossipOption(i)
			row.icon:SetTexture(ICON_FOR[typ] or ICON_FOR.dialog); row.icon:Show()
			row.fs:SetText(label); row.fs:Show()
			row.btn:Show()
		else
			row.icon:Hide(); row.fs:Hide(); row.btn:Hide()
		end
	end
end

local shown = false
local function setShown(v)
	if v == shown then return end
	shown = v
	if v then root:Show(); refresh() else root:Hide() end
end

local stage = CreateFrame('Frame', nil, nil)
stage:SetScript('OnEvent', function(_, event, arg)
	if event == Events.WORLD_SHOWN then
		SetGameFrameShown('DialogNpcFrame', false)
	elseif event == Events.PANEL_OPENED then
		if arg == 'DialogNpcFrame' then setShown(true) end
	elseif event == Events.PANEL_CLOSED then
		if arg == 'DialogNpcFrame' then setShown(false) end
	elseif event == Events.DIALOG_NPC_UPDATE then
		if shown then refresh() end   -- gossip chain repopulated while open (no PANEL_OPENED fires)
	else
		setShown(false)
	end
end)
for _, e in ipairs({ Events.WORLD_SHOWN, Events.PANEL_OPENED, Events.PANEL_CLOSED, Events.DIALOG_NPC_UPDATE,
                     Events.LOGIN_SHOWN, Events.CHARSELECT_SHOWN, Events.CHARCREATE_SHOWN }) do
	stage:RegisterEvent(e)
end

print('EmberUI npc dialog loaded')
