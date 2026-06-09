-- Lua level-up toast, replacing the C++ LevelupNotify (force-hidden). On PLAYER_LEVEL_UP (fired by
-- World::onLevelTo AFTER the Level var updates) it shows the rank name (player_exp_levels, lowercased --
-- via GetPlayerRankName) over levelup_notify.png, fading in 1s, solid 3s, fading out (the C++ curve).

local root = CreateFrame('Frame', 'EmberLevelupNotify', nil)
root:Hide()

local art = root:CreateTexture()
art:SetTexture('levelup_notify.png')
local aw, ah = GetTextureSize('levelup_notify.png')
if aw <= 0 then aw, ah = 600, 120 end
art:SetSize(aw, ah)
art:SetPoint('TOPLEFT', root, 'TOPLEFT', 0, 0)
root:SetSize(aw, ah)

local txt = root:CreateFontString()
txt:SetFont('Ringbearer'); txt:SetFontSize(44)
-- the C++ centers the text at x+286 (absolute-center mode); approximate around that anchor
txt:SetPoint('TOPLEFT', root, 'TOPLEFT', 180, 45)
txt:SetWidth(0)

local phase, t = nil, 0   -- phase: 'in' (1s) -> 'solid' (3s) -> 'out' (1s)

local function setAlpha(a)
	art:SetAlpha(a)
	txt:SetTextColor(232, 205, 182, math.floor(255 * a))
end

root:SetScript('OnUpdate', function(_, dt)
	if not phase then return end
	t = t + dt
	if phase == 'in' then
		setAlpha(math.min(1, t))
		if t >= 1 then phase, t = 'solid', 0 end
	elseif phase == 'solid' then
		setAlpha(1)
		if t >= 3 then phase, t = 'out', 0 end
	else
		setAlpha(math.max(0, 1 - t))
		if t >= 1 then phase = nil; root:Hide() end
	end
end)

local stage = CreateFrame('Frame', nil, nil)
stage:SetScript('OnEvent', function(_, event)
	if event == Events.PLAYER_LEVEL_UP then
		local sw, sh = GetScreenWidth(), GetScreenHeight()
		root:ClearAllPoints()
		root:SetPoint('TOPLEFT', math.floor(sw / 2 - aw / 2), math.floor(sh / 2) + 100)
		txt:SetText(string.lower(GetPlayerRankName() or ''))
		phase, t = 'in', 0
		setAlpha(0)
		root:Show()
	elseif event == Events.WORLD_SHOWN then
		SetGameFrameShown('LevelupNotifyFrame', false)
	else
		phase = nil
		root:Hide()
	end
end)
for _, e in ipairs({ Events.PLAYER_LEVEL_UP, Events.WORLD_SHOWN,
                     Events.LOGIN_SHOWN, Events.CHARSELECT_SHOWN, Events.CHARCREATE_SHOWN }) do
	stage:RegisterEvent(e)
end

print('EmberUI levelup notify loaded')
