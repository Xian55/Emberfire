-- Lua HUD world-buttons, replacing the two C++ World buttons (force-hidden):
-- 1. Spend-XP exclaim (spend_xp_notify) above the action bars -- shows when the server flags unspent
--    points (HasUnspentPoints); click = LaunchSpendExp (opens the Equipment in spend context).
-- 2. Waypoint activate (waypoint_activate) center-screen -- shows while standing on a waypoint with the
--    map closed (IsStandingOnWaypoint); click = QueryWaypoints (the verified op45 travel flow).
-- Both poll their visibility on a short OnUpdate tick (the C++ did the same per frame).

local function worldButton(tex, onClick)
	local b = CreateFrame('Button', 'Ember' .. tex, nil)
	b:SetTexture(tex .. '_idle.png'); b:SetHoverTexture(tex .. '_hover.png')
	local w, h = GetTextureSize(tex .. '_idle.png')
	b:SetSize(w > 0 and w or 32, h > 0 and h or 32)
	b:EnableMouse(true)
	b:SetScript('OnClick', onClick)
	b:Hide()
	return b
end

local spend = worldButton('spend_xp_notify', function() LaunchSpendExp() end)
local waypoint = worldButton('waypoint_activate', function() QueryWaypoints() end)

local inWorld = false
local function relayout()
	local sw, sh = GetScreenWidth(), GetScreenHeight()
	spend:ClearAllPoints();    spend:SetPoint('TOPLEFT', math.floor(sw / 2) - 86, sh - 271)          -- C++ spot
	waypoint:ClearAllPoints(); waypoint:SetPoint('TOPLEFT', math.floor(sw / 2) - 82, math.floor(sh / 2) + 150)
end

local poll = CreateFrame('Frame', nil, nil)
local acc = 0
poll:SetScript('OnUpdate', function(_, dt)
	if not inWorld then return end
	acc = acc + dt
	if acc < 0.25 then return end
	acc = 0
	if HasUnspentPoints() then spend:Show() else spend:Hide() end
	if IsStandingOnWaypoint() then waypoint:Show() else waypoint:Hide() end
end)

local stage = CreateFrame('Frame', nil, nil)
stage:SetScript('OnEvent', function(_, event)
	if event == Events.WORLD_SHOWN then
		SetGameFrameShown('SpendExpButton', false)
		SetGameFrameShown('WaypointButton', false)
		inWorld = true
		relayout()
	else
		inWorld = false
		spend:Hide(); waypoint:Hide()
	end
end)
for _, e in ipairs({ Events.WORLD_SHOWN, Events.LOGIN_SHOWN, Events.CHARSELECT_SHOWN, Events.CHARCREATE_SHOWN }) do
	stage:RegisterEvent(e)
end

print('EmberUI hud buttons loaded')
