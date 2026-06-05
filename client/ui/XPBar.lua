-- XP bar: fills toward the next level. Driven by PLAYER_XP_UPDATE + a 0.5s poll.
-- Visible only in-world (the frame manager is persistent across all screens now).

local W = math.floor(GetScreenWidth() * 0.225)   -- 22.5% of screen width, centered at the bottom

local f = CreateFrame('Frame', 'EmberUI_XPBar', nil)
f:SetSize(W, 14); f:SetPoint('BOTTOM', 0, 0)   -- flush to screen bottom, centered
f:Hide()   -- hidden until WORLD_SHOWN

local bar = CreateFrame('StatusBar', nil, f)
bar:SetStatusBarTexture('xp_bar.png'); bar:SetStatusBarColor(150, 0, 200, 255)
bar:SetSize(W, 14); bar:SetPoint('TOPLEFT', 0, 0)

-- Hover readout: "<pct>% - <cur>/<max> XP", shown above the bar only while the cursor is over it.
-- (The bar is flush to the screen bottom, so the text must sit ABOVE it or it renders off-screen.)
local label = f:CreateFontString()
label:SetFont('Palatino'); label:SetFontSize(12)
label:SetTextColor(255, 255, 255, 255)
label:Hide()

local function refresh()
	local mx = GetMaxXP(); if mx <= 0 then mx = 1 end
	bar:SetMinMaxValues(0, mx); bar:SetValue(GetXP())
end

local function refreshLabel()
	local cur = GetXP()
	local mx = GetMaxXP(); if mx <= 0 then mx = 1 end
	local pct = math.floor(cur * 100 / mx + 0.5)
	label:SetText(string.format('%d%% - %d/%d XP', pct, cur, mx))
	label:SetPoint('CENTER', 0, 0)   -- re-anchor after text (size known) -> centered ON the bar
end

local d = CreateFrame('Frame', nil, nil)
d:SetScript('OnEvent', refresh)
d:RegisterEvent(Events.PLAYER_XP_UPDATE)
local acc = 0
d:SetScript('OnUpdate', function(_, dt)
	if not EmberUI.inWorld then return end
	acc = acc + dt
	if acc >= 0.5 then acc = 0; refresh(); if f:IsMouseOver() then refreshLabel() end end  -- keep readout fresh while hovering
end)

-- Hover is engine-driven (edge-detected OnEnter/OnLeave) — no per-frame polling in addon code.
f:SetScript('OnEnter', function() refreshLabel(); label:Show() end)
f:SetScript('OnLeave', function() label:Hide() end)

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
