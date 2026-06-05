-- EmberHealthBar — sample addon for the Emberfire Lua UI layer.
-- Shows the local player's health as a StatusBar that tracks the UNIT_HEALTH event,
-- with its position remembered in SavedVariables. Edit + /reload to see changes live.

EmberHealthBarDB = EmberHealthBarDB or {}
local DB = EmberHealthBarDB
DB.x = DB.x or 40
DB.y = DB.y or 90

local f = CreateFrame('Frame', 'EmberHealthBar', nil)
f:SetSize(220, 26)
f:SetPoint('TOPLEFT', DB.x, DB.y)

-- Drag to reposition (left button); the engine moves the frame, we just persist where it lands.
f:SetMovable(true)
f:RegisterForDrag('LeftButton')
f:SetScript('OnDragStop', function(self)
	DB.x = self:GetLeft(); DB.y = self:GetTop()
end)

local bar = CreateFrame('StatusBar', nil, f)
bar:SetStatusBarTexture('xp_bar.png')
bar:SetStatusBarColor(210, 50, 50, 255)   -- red
bar:SetAllPoints(f)                        -- stretch to fill the frame (two-point anchor; tracks resize)
bar:SetMinMaxValues(0, 1)
bar:SetValue(1)

local label = f:CreateFontString()
label:SetPoint('LEFT', bar, 'LEFT', 8, 0)  -- anchored relative to the bar (relativeTo)
label:SetText('HP')

-- Primitives demo: an icon anchored to the RIGHT of the frame, cropped to the texture's left half, faded.
local icon = f:CreateTexture()
icon:SetSize(26, 26)
icon:SetTexture('xp_bar.png')
icon:SetPoint('LEFT', f, 'RIGHT', 4, 0)    -- relativeTo + relativePoint
icon:SetTexCoord(0, 0.5, 0, 1)             -- show only the left half of the source texture
icon:SetAlpha(0.8)

local function refresh()
	local hp = UnitHealth('player')
	local mx = UnitHealthMax('player')
	if mx <= 0 then mx = 1 end
	bar:SetMinMaxValues(0, mx)
	bar:SetValue(hp)
	label:SetText('HP  ' .. hp .. ' / ' .. mx)
end

-- Update the instant health changes...
f:SetScript('OnEvent', refresh)
f:RegisterEvent('UNIT_HEALTH')

-- ...and poll twice a second too (max-health changes don't fire UNIT_HEALTH).
local acc = 0
f:SetScript('OnUpdate', function(self, dt)
	acc = acc + dt
	if acc >= 0.5 then acc = 0; refresh() end
end)

-- Only show once the player is actually in the world — there is no player on the login / character
-- screens or during the loading screen, so the bar would otherwise sit at "HP 0 / 1". Show on
-- PLAYER_LOGIN (loading done), hide on the glue stages.
f:Hide()
local stage = CreateFrame('Frame', nil, nil)
stage:SetScript('OnEvent', function(_, event)
	if event == Events.PLAYER_LOGIN then f:Show(); refresh() else f:Hide() end
end)
stage:RegisterEvent(Events.PLAYER_LOGIN)
stage:RegisterEvent(Events.LOGIN_SHOWN)
stage:RegisterEvent(Events.CHARSELECT_SHOWN)
stage:RegisterEvent(Events.CHARCREATE_SHOWN)

-- Primitives introspection demo (GetName/GetWidth/GetParent/IsVisible).
print('EmberHealthBar loaded: name=' .. tostring(f:GetName())
	.. ' size=' .. f:GetWidth() .. 'x' .. f:GetHeight()
	.. ' barParent=' .. tostring(bar:GetParent():GetName())
	.. ' type=' .. bar:GetObjectType())
