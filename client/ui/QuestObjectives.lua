-- Lua quest-objectives tracker (top-right banner), replacing the C++ QuestObjectives (force-hidden). Reads
-- the migrated QuestLog getters (GetQuestInfo tracked flag + GetQuestObjectives) and refreshes on
-- QUEST_LOG_UPDATE / QUEST_OBJECTIVE_UPDATE. Clicking a quest title opens the quest log selected on it
-- (OpenQuestLog + EmberUI._selectQuestId, consumed by QuestLog.lua). Colors/sizes from the C++: banner
-- Ringbearer 18, titles PalatinoBold 18 (hover white / done green), objectives PalatinoBold 15.
-- v1 deviation: fixed position (no dodge below an open inventory); per-line height estimated from newlines.

local MAX_TRACKED = 5
local LINE_H = 17

local root = CreateFrame('Frame', 'EmberQuestObjectives', nil)
root:SetSize(330, 400)
root:Hide()

local bg = root:CreateTexture()
bg:SetTexture('objectives_bg.png')
bg:SetPoint('TOPLEFT', root, 'TOPLEFT', -30, 0)
local bgW, bgH = GetTextureSize('objectives_bg.png')
if bgW > 0 then bg:SetSize(bgW, bgH) end

local banner = root:CreateFontString()
banner:SetFont('Ringbearer'); banner:SetFontSize(18); banner:SetTextColor(217, 194, 152, 255)
banner:SetPoint('TOPLEFT', root, 'TOPLEFT', 50, 15)
banner:SetText('Objectives')

local rows = {}
for i = 1, MAX_TRACKED do
	local titleBtn = CreateFrame('Button', nil, root)
	titleBtn:SetSize(280, 20); titleBtn:EnableMouse(true)
	local title = root:CreateFontString()
	title:SetFont('PalatinoBold'); title:SetFontSize(18)
	local obj = root:CreateFontString()
	obj:SetFont('PalatinoBold'); obj:SetFontSize(15); EmberUI.SetColor(obj, EmberUI.Color.Title)
	obj:SetWidth(260)
	local row = { btn = titleBtn, title = title, obj = obj }
	titleBtn:SetScript('OnClick', function()
		if row._id then
			EmberUI.QuestLogSelect(row._id)   -- selects now if the log is open, else on its next show
			OpenQuestLog()
		end
	end)
	titleBtn:SetScript('OnEnter', function() title:SetTextColor(255, 255, 255, 255) end)
	titleBtn:SetScript('OnLeave', function()
		if row._done then title:SetTextColor(80, 220, 80, 255)
		else title:SetTextColor(217, 194, 152, 255) end
	end)
	rows[i] = row
end

local function countLines(s)
	if not s or s == '' then return 0 end
	local n = 1
	for _ in s:gmatch('\n') do n = n + 1 end
	return n
end

local function refresh()
	local sw = GetScreenWidth()
	root:ClearAllPoints()
	root:SetPoint('TOPLEFT', sw - (bgW > 0 and bgW or 330) - 25 + 30, 340)   -- C++: x = sw - bgW - 25 (bg at -30)

	local shownCount = 0
	local y = 45
	local n = GetNumQuests()
	for idx = 1, n do
		local id, title, done, tracked = GetQuestInfo(idx)
		if tracked and shownCount < MAX_TRACKED then
			shownCount = shownCount + 1
			local row = rows[shownCount]
			row._id, row._done = id, done
			local label = title or 'Unknown'
			if done then label = label .. ' (Complete)' end
			row.title:SetText(label)
			if done then row.title:SetTextColor(80, 220, 80, 255) else row.title:SetTextColor(217, 194, 152, 255) end
			row.title:ClearAllPoints(); row.title:SetPoint('TOPLEFT', root, 'TOPLEFT', -14, y)
			row.btn:ClearAllPoints();   row.btn:SetPoint('TOPLEFT', root, 'TOPLEFT', -14, y)
			row.title:Show(); row.btn:Show()
			y = y + 22

			local objStr = GetQuestObjectives(id) or ''
			row.obj:SetText(objStr)
			row.obj:ClearAllPoints(); row.obj:SetPoint('TOPLEFT', root, 'TOPLEFT', 6, y)
			row.obj:Show()
			y = y + countLines(objStr) * LINE_H + 6
		end
	end

	for i = shownCount + 1, MAX_TRACKED do
		rows[i]._id = nil
		rows[i].title:Hide(); rows[i].obj:Hide(); rows[i].btn:Hide()
	end

	if shownCount > 0 and EmberUI.inWorld then root:Show() else root:Hide() end
end

-- exported so the quest log can refresh the tracker right after a track/untrack toggle (no event fires
-- for it -- tracking is client-local)
EmberUI.QuestTrackerRefresh = refresh

local stage = CreateFrame('Frame', nil, nil)
stage:SetScript('OnEvent', function(_, event)
	if event == Events.WORLD_SHOWN then
		SetGameFrameShown('QuestObjectivesFrame', false)
		EmberUI.inWorld = true
		refresh()
	elseif event == Events.QUEST_LOG_UPDATE or event == Events.QUEST_OBJECTIVE_UPDATE then
		refresh()
	else
		EmberUI.inWorld = false
		root:Hide()
	end
end)
for _, e in ipairs({ Events.WORLD_SHOWN, Events.QUEST_LOG_UPDATE, Events.QUEST_OBJECTIVE_UPDATE,
                     Events.LOGIN_SHOWN, Events.CHARSELECT_SHOWN, Events.CHARCREATE_SHOWN }) do
	stage:RegisterEvent(e)
end

print('EmberUI quest objectives loaded')
