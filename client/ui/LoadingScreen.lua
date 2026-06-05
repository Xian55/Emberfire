-- Lua loading screen: a full-screen BLACK cover with a fade transition (mirrors the C++ intro fade):
--   WORLD_SHOWN  -> fade IN to black (cover the screen while the map loads behind it)
--   ...hold black while the map loads...
--   PLAYER_LOGIN -> fade OUT from black (reveal the world; fired when the intro fade completes = map ready)
-- Loaded last + level 200 so it covers the HUD.

local sw, sh = GetScreenWidth(), GetScreenHeight()

local screen = CreateFrame('Frame', 'EmberUI_LoadingScreen', nil)
screen:SetSize(sw, sh); screen:SetPoint('TOPLEFT', 0, 0); screen:Hide()
screen:SetFrameLevel(200)   -- above everything (HUD, popups, addons)

-- Black cover: a StatusBar tinted opaque black (xp_bar.png is solid, so the tint fills). Its alpha is faded.
local black = CreateFrame('StatusBar', nil, screen)
black:SetStatusBarTexture('xp_bar.png'); black:SetStatusBarColor(0, 0, 0, 0)
black:SetSize(sw + 80, sh + 80); black:SetPoint('TOPLEFT', -40, -40)   -- oversize to avoid edge bleed
black:SetMinMaxValues(0, 1); black:SetValue(1)

local FADE_IN  = 3.0   -- per-second alpha rate (~0.33s)
local FADE_OUT = 1.4   -- per-second alpha rate (~0.7s)

local alpha = 0.0
local mode = 'idle'    -- 'in' | 'hold' | 'out' | 'idle'

local function apply() black:SetStatusBarColor(0, 0, 0, math.floor(alpha * 255 + 0.5)) end

local stage = CreateFrame('Frame', nil, nil)
stage:SetScript('OnEvent', function(_, event)
	if event == Events.WORLD_SHOWN then
		alpha = 0.0; mode = 'in'; apply(); screen:Show()
	elseif event == Events.PLAYER_LOGIN then
		mode = 'out'   -- world ready: fade the black out (from wherever the fade-in got to)
	end
end)
stage:RegisterEvent(Events.WORLD_SHOWN)
stage:RegisterEvent(Events.PLAYER_LOGIN)

stage:SetScript('OnUpdate', function(_, dt)
	if mode == 'in' then
		alpha = alpha + dt * FADE_IN
		if alpha >= 1.0 then alpha = 1.0; mode = 'hold' end
		apply()
	elseif mode == 'out' then
		alpha = alpha - dt * FADE_OUT
		if alpha <= 0.0 then alpha = 0.0; mode = 'idle'; screen:Hide() end
		apply()
	end
end)

EmberUI.LoadingScreen = screen
print('EmberUI loading screen loaded')
