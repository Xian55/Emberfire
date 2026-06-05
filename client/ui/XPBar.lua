-- XP bar: fills toward the next level. Driven by PLAYER_XP_UPDATE + a 0.5s poll.
-- Visible only in-world (the frame manager is persistent across all screens now).

local f = CreateFrame('Frame', 'EmberUI_XPBar', nil)
f:SetSize(404, 14); f:SetPoint('TOPLEFT', 200, 560)
f:Hide()   -- hidden until WORLD_SHOWN

local bar = CreateFrame('StatusBar', nil, f)
bar:SetStatusBarTexture('xp_bar.png'); bar:SetStatusBarColor(150, 0, 200, 255)
bar:SetSize(404, 14); bar:SetPoint('TOPLEFT', 0, 0)

local function refresh()
	local mx = GetMaxXP(); if mx <= 0 then mx = 1 end
	bar:SetMinMaxValues(0, mx); bar:SetValue(GetXP())
end

local d = CreateFrame('Frame', nil, nil)
d:SetScript('OnEvent', refresh)
d:RegisterEvent(Events.PLAYER_XP_UPDATE)
local acc = 0
d:SetScript('OnUpdate', function(_, dt) if EmberUI.inWorld then acc = acc + dt; if acc >= 0.5 then acc = 0; refresh() end end end)

EmberUI.XPBar = f

-- Retire the C++ XP bar when the world stage exists (during loading); reveal the Lua one when the player
-- is actually in the world (PLAYER_LOGIN); hide on the glue stages.
local stage = CreateFrame('Frame', nil, nil)
stage:SetScript('OnEvent', function(_, event)
	if event == Events.WORLD_SHOWN then
		SetGameFrameShown('XPBar', false)
	elseif event == Events.PLAYER_LOGIN then
		f:Show(); refresh()
	else
		f:Hide()
	end
end)
stage:RegisterEvent(Events.WORLD_SHOWN)
stage:RegisterEvent(Events.PLAYER_LOGIN)
stage:RegisterEvent(Events.LOGIN_SHOWN)
stage:RegisterEvent(Events.CHARSELECT_SHOWN)
stage:RegisterEvent(Events.CHARCREATE_SHOWN)

print('EmberUI XP bar loaded')
