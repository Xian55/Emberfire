-- Unit frames: instantiate player / target / party from the shared template (UnitFrameTemplate.lua),
-- then wire the refresh events + stage lifecycle. The widget itself lives in the template; this file only
-- decides WHICH frames exist, where, and how they differ.
--
-- player + target share the LARGE layout and differ only by `mirror` (portrait left vs right + reverse art).
-- party 1-3 share the COMPACT layout. Layout numbers are in portrait-left coords; the template mirrors them.

-- Large layout (player/target). Coords from UnitFrame.cpp's LocalPlayer style; the template mirrors for target.
local LARGE = {
	portrait = { 44, 73, 39 },                          -- center x, center y, radius
	hp = { 74, 39 }, mp = { 84, 70 },
	hpPct = { 91, 46 }, mpPct = { 91, 74 },
	hpTxt = { 215, 45 }, mpTxt = { 215, 73 },
	lvl = { 71, 105 }, name = { 66, 8 },
	combat = { 73, 108 }, leader = { 16, 40 }, repairPos = { 15, 105 }, elite = { 110, -2 },
	auras = { 95, 108 }, auraSize = 30, castOffset = { 50, 200 },
}

-- Compact layout (party). Coords from UnitFrame.cpp's PartyMember style.
local COMPACT = {
	portrait = { 30, 36, 26 },
	hp = { 45, 11 }, mp = { 55, 33 },
	hpPct = { 66, 13 }, mpPct = { 66, 33 },
	hpTxt = { 165, 13 }, mpTxt = { 165, 33 },
	lvl = { 51, 54 }, name = { 80, -13 },
	combat = { 53, 57 }, leader = { 10, 14 },
	auras = { 76, 58 }, auraSize = 19,
}

-- Lay frames out from the real art dimensions so they never overlap (sizes vary by texture).
local MARGIN = 16
local lw, lh = GetTextureSize('unit_frame.png');       if lw <= 0 then lw, lh = 300, 130 end
local pw, ph = GetTextureSize('unit_frame_party.png');  if pw <= 0 then pw, ph = 230, 70 end
local PX, PY = 40, 20                                    -- player top-left

EmberUI.PlayerFrame = EmberUI.CreateUnitFrame{
	token = 'player', x = PX, y = PY, layout = LARGE, mirror = false,
	art = { frame = 'unit_frame.png', hp = 'unit_frame_hp.png', mp = 'unit_frame_mp.png' },
	show = { leader = true, repair = true, castbar = true, movable = true },
}

EmberUI.TargetFrame = EmberUI.CreateUnitFrame{
	token = 'target', x = PX + lw + MARGIN, y = PY, layout = LARGE, mirror = true,
	art = { frame = 'unit_frame_reverse.png', hp = 'unit_frame_hp_reverse.png', mp = 'unit_frame_mp_reverse.png' },
	show = { name = true, castbar = true, elite = true, movable = true },
}

EmberUI.PartyFrames = {}
for i = 1, 3 do
	EmberUI.PartyFrames[i] = EmberUI.CreateUnitFrame{
		token = 'party' .. i, x = PX, y = PY + lh + MARGIN + (i - 1) * (ph + 8), layout = COMPACT, mirror = false,
		art = { frame = 'unit_frame_party.png', hp = 'unit_frame_hp_party.png', mp = 'unit_frame_mp_party.png' },
		show = { name = true, leader = true },
	}
end

-- Refresh on the unit events + a 0.5s poll fallback.
local function refreshAll()
	if not EmberUI.inWorld then return end
	EmberUI.PlayerFrame.refresh()
	EmberUI.TargetFrame.refresh()
	for i = 1, 3 do
		if PartyMemberExists(i) then EmberUI.PartyFrames[i].refresh()
		else EmberUI.PartyFrames[i].frame:Hide() end
	end
end

-- Lock/Unlock from the right-click menu (Lua-handled entries; social actions are handled C++-side).
local menu = CreateFrame('Frame', nil, nil)
menu:SetScript('OnEvent', function(_, _, line)
	local t = EmberUI._menuToken
	if not t then return end
	if line == 'Unlock' then EmberUI.SetFrameLocked(t, false)
	elseif line == 'Lock' then EmberUI.SetFrameLocked(t, true) end
end)
menu:RegisterEvent(Events.CONTEXT_MENU_CLICK)

local d = CreateFrame('Frame', nil, nil)
d:SetScript('OnEvent', refreshAll)
for _, e in ipairs({ Events.UNIT_HEALTH, Events.UNIT_MAXHEALTH, Events.UNIT_POWER, Events.UNIT_MAXPOWER,
                     Events.UNIT_LEVEL, Events.UNIT_AURA, Events.UNIT_SPELLCAST_START, Events.UNIT_SPELLCAST_STOP,
                     Events.PLAYER_TARGET_CHANGED, Events.GROUP_ROSTER_UPDATE }) do
	d:RegisterEvent(e)
end
local acc = 0
d:SetScript('OnUpdate', function(_, dt) if EmberUI.inWorld then acc = acc + dt; if acc >= 0.5 then acc = 0; refreshAll() end end end)

-- Stage lifecycle: retire the C++ frames in-world, gate the Lua HUD on PLAYER_LOGIN, hide on glue screens.
local function hideAll()
	EmberUI.PlayerFrame.frame:Hide(); EmberUI.TargetFrame.frame:Hide()
	-- cast bars are top-level frames now (not children), so hide them explicitly on leaving the world
	if EmberUI.PlayerFrame.cast then EmberUI.PlayerFrame.cast.frame:Hide() end
	if EmberUI.TargetFrame.cast then EmberUI.TargetFrame.cast.frame:Hide() end
	for i = 1, 3 do EmberUI.PartyFrames[i].frame:Hide() end
end

local stage = CreateFrame('Frame', nil, nil)
stage:SetScript('OnEvent', function(_, event)
	if event == Events.WORLD_SHOWN then
		for _, n in ipairs({ 'PlayerFrame', 'TargetFrame', 'Party1Frame', 'Party2Frame', 'Party3Frame', 'CastBar' }) do
			SetGameFrameShown(n, false)
		end
	elseif event == Events.PLAYER_LOGIN then
		EmberUI.inWorld = true; refreshAll()
	else
		EmberUI.inWorld = false; hideAll()
	end
end)
stage:RegisterEvent(Events.WORLD_SHOWN)
stage:RegisterEvent(Events.PLAYER_LOGIN)
stage:RegisterEvent(Events.LOGIN_SHOWN)
stage:RegisterEvent(Events.CHARSELECT_SHOWN)
stage:RegisterEvent(Events.CHARCREATE_SHOWN)

print('EmberUI unit frames loaded')
