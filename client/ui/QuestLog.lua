-- Lua Quest Log: quest list + selected-quest details, replacing the C++ QuestLog (force-hidden, mirrored).
-- The live window stays the model (populated by the server quest packets); this view reads it via GetNumQuests
-- /GetQuestInfo/GetQuestObjectives/GetQuestDescription and drives track/untrack (local + cached) + abandon.
-- Event-driven: PANEL_OPENED/CLOSED('QuestLogFrame') show/hide, QUEST_LOG_UPDATE / QUEST_OBJECTIVE_UPDATE
-- refresh. NO OnUpdate. NOTE: positions are guesses (the C++ details pane is dynamic) — tune live.

local LIST_X, LIST_Y, LIST_ROWH, LIST_VISIBLE = 28, 72, 22, 10
local DETAIL_X, TITLE_Y, DESC_Y, OBJ_Y = 250, 64, 96, 300
local DETAIL_W = 230

local W, H = GetTextureSize('questlog.png')
if W <= 0 then W, H = 500, 540 end
local root = CreateFrame('Frame', 'EmberQuestLog', nil)
root:SetSize(W, H)
root:SetPoint('CENTER')
root:SetMovable(true); root:RegisterForDrag('LeftButton')
root:Hide()

local function vis(f, v) if v then f:Show() else f:Hide() end end
local function texSize(name, dw, dh) local w, h = GetTextureSize(name); if w <= 0 then return dw, dh end return w, h end

local bg = root:CreateTexture()
bg:SetAllPoints(root)
bg:SetTexture('questlog.png')

-- right-side details
local detTitle = root:CreateFontString()
detTitle:SetFont('Fontin'); detTitle:SetFontSize(16); detTitle:SetTextColor(232, 199, 22, 255)
detTitle:SetPoint('TOPLEFT', root, 'TOPLEFT', DETAIL_X, TITLE_Y)
local detDesc = root:CreateFontString()
detDesc:SetFont('Helvetica'); detDesc:SetFontSize(13); detDesc:SetTextColor(220, 220, 220, 255)
detDesc:SetPoint('TOPLEFT', root, 'TOPLEFT', DETAIL_X, DESC_Y); detDesc:SetWidth(DETAIL_W)
local detObj = root:CreateFontString()
detObj:SetFont('Helvetica'); detObj:SetFontSize(13); detObj:SetTextColor(232, 199, 22, 255)
detObj:SetPoint('TOPLEFT', root, 'TOPLEFT', DETAIL_X, OBJ_Y); detObj:SetWidth(DETAIL_W)

local selectedIdx = 1
local scroll = 0
local refresh   -- forward decl

-- ---- quest list rows ----
local rows = {}
for r = 1, LIST_VISIBLE do
	local y = LIST_Y + (r - 1) * LIST_ROWH
	local fs = root:CreateFontString(); fs:SetFont('Helvetica'); fs:SetFontSize(14)
	fs:SetPoint('TOPLEFT', root, 'TOPLEFT', LIST_X, y)
	local hit = CreateFrame('Button', nil, root)
	hit:SetSize(200, LIST_ROWH); hit:SetPoint('TOPLEFT', root, 'TOPLEFT', LIST_X, y); hit:EnableMouse(true)
	local row = { fs = fs, hit = hit }
	hit:SetScript('OnClick', function() if row._idx then selectedIdx = row._idx; refresh() end end)
	rows[r] = row
end

-- ---- track / abandon buttons (generic_highlight bg + text label) ----
local function labelButton(x, y, onClick)
	local bw, bh = texSize('generic_highlight_idle.png', 96, 26)
	local b = CreateFrame('Button', nil, root)
	b:SetTexture('generic_highlight_idle.png'); b:SetHoverTexture('generic_highlight_hover.png')
	b:SetSize(bw, bh); b:SetPoint('TOPLEFT', root, 'TOPLEFT', x, y); b:EnableMouse(true)
	b:SetScript('OnClick', onClick)
	local fs = root:CreateFontString(); fs:SetFont('Arial'); fs:SetFontSize(13); fs:SetTextColor(255, 255, 255, 255)
	fs:SetPoint('TOPLEFT', root, 'TOPLEFT', x + 12, y + 6)
	return b, fs
end

local function selectedQuestId()
	local id = select(1, GetQuestInfo(selectedIdx))
	return id
end

local trackBtn, trackFS = labelButton(DETAIL_X, 470, function()
	local id = selectedQuestId()
	if id and id ~= 0 then SetQuestTracked(id, not IsQuestTracked(id)); refresh() end
end)
local abandonBtn, abandonFS = labelButton(DETAIL_X + 110, 470, function()
	local id = selectedQuestId()
	if not id or id == 0 then return end
	EmberUI.ShowMenu({
		{ text = 'Abandon this quest?' },
		{ text = 'Yes, abandon', color = { 220, 90, 90 }, onClick = function() AbandonQuest(id) end },
		{ text = 'Cancel' },
	})
end)
abandonFS:SetText('Abandon')

refresh = function()
	local n = GetNumQuests()
	if selectedIdx > n then selectedIdx = n end
	if selectedIdx < 1 then selectedIdx = 1 end

	-- keep the selected row in view
	if selectedIdx <= scroll then scroll = selectedIdx - 1 end
	if selectedIdx > scroll + LIST_VISIBLE then scroll = selectedIdx - LIST_VISIBLE end
	local maxS = math.max(0, n - LIST_VISIBLE)
	if scroll > maxS then scroll = maxS end
	if scroll < 0 then scroll = 0 end

	for r = 1, LIST_VISIBLE do
		local idx = scroll + r
		local row = rows[r]
		if idx <= n then
			local id, title, done, tracked = GetQuestInfo(idx)
			row._idx = idx
			local label = (tracked and '> ' or '  ') .. title .. (done and '  (Complete)' or '')
			row.fs:SetText(label)
			if idx == selectedIdx then row.fs:SetTextColor(255, 255, 255, 255)
			elseif done then row.fs:SetTextColor(120, 220, 120, 255)
			else row.fs:SetTextColor(232, 199, 22, tracked and 255 or 150) end
			row.fs:Show(); row.hit:Show()
		else
			row._idx = nil; row.fs:Hide(); row.hit:Hide()
		end
	end

	-- details for the selected quest
	if n > 0 then
		local id, title = GetQuestInfo(selectedIdx)
		detTitle:SetText(title or ''); detTitle:Show()
		detDesc:SetText(GetQuestDescription(id) or ''); detDesc:Show()
		detObj:SetText(GetQuestObjectives(id) or ''); detObj:Show()
		trackFS:SetText(IsQuestTracked(id) and 'Untrack' or 'Track')
		vis(trackBtn, true); vis(trackFS, true); vis(abandonBtn, true); vis(abandonFS, true)
	else
		detTitle:Hide(); detDesc:Hide(); detObj:Hide()
		vis(trackBtn, false); vis(trackFS, false); vis(abandonBtn, false); vis(abandonFS, false)
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
		SetGameFrameShown('QuestLogFrame', false)
	elseif event == Events.PANEL_OPENED then
		if arg == 'QuestLogFrame' then setShown(true) end
	elseif event == Events.PANEL_CLOSED then
		if arg == 'QuestLogFrame' then setShown(false) end
	elseif event == Events.QUEST_LOG_UPDATE or event == Events.QUEST_OBJECTIVE_UPDATE then
		if shown then refresh() end
	else
		setShown(false)
	end
end)
for _, e in ipairs({ Events.WORLD_SHOWN, Events.PANEL_OPENED, Events.PANEL_CLOSED,
                     Events.QUEST_LOG_UPDATE, Events.QUEST_OBJECTIVE_UPDATE,
                     Events.LOGIN_SHOWN, Events.CHARSELECT_SHOWN, Events.CHARCREATE_SHOWN }) do
	stage:RegisterEvent(e)
end

print('EmberUI quest log loaded')
