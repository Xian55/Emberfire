-- XP bar: fills toward the next level. Driven by PLAYER_XP_UPDATE + a 0.5s poll.

local f = CreateFrame('Frame', 'EmberUI_XPBar', nil)
f:SetSize(404, 14); f:SetPoint('TOPLEFT', 200, 560)

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
d:SetScript('OnUpdate', function(_, dt) acc = acc + dt; if acc >= 0.5 then acc = 0; refresh() end end)
refresh()

EmberUI.XPBar = f

-- Replace the C++ XP bar with this Lua one.
SetGameFrameShown('XPBar', false)

print('EmberUI XP bar loaded')
