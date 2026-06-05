-- Player + target unit frames: name, level, health bar, mana bar; click to target.
-- Built entirely from the C++ API (getters + events + the TargetUnit command).
-- The frame manager is now persistent across all screens, so these frames stay hidden until the player
-- is in the world (WORLD_SHOWN) and hide again on the login / character screens.

local function makeUnitFrame(token, x, y)
	local f = CreateFrame('Button', 'EmberUI_' .. token .. 'Frame', nil)
	f:SetSize(220, 60)
	f:SetPoint('TOPLEFT', x, y)

	local hp = CreateFrame('StatusBar', nil, f)
	hp:SetStatusBarTexture('xp_bar.png'); hp:SetStatusBarColor(210, 50, 50, 255)
	hp:SetSize(200, 18); hp:SetPoint('TOPLEFT', 10, 22)

	local mp = CreateFrame('StatusBar', nil, f)
	mp:SetStatusBarTexture('xp_bar.png'); mp:SetStatusBarColor(40, 90, 220, 255)
	mp:SetSize(200, 12); mp:SetPoint('TOPLEFT', 10, 44)

	local name = f:CreateFontString(); name:SetPoint('TOPLEFT', 10, 2)
	local lvl  = f:CreateFontString(); lvl:SetPoint('TOPLEFT', 180, 2)

	-- One refresh updates everything; driven by the relevant events AND a 0.5s poll, so the frame fills in
	-- as soon as the player's/target's data arrives (which can be after this UI loads).
	local function refresh()
		if not EmberUI.inWorld then f:Hide(); return end
		if token == 'target' then
			if UnitExists('target') then f:Show() else f:Hide(); return end
		else
			f:Show()
		end
		local hpmax = UnitHealthMax(token); if hpmax <= 0 then hpmax = 1 end
		hp:SetMinMaxValues(0, hpmax); hp:SetValue(UnitHealth(token))
		local mpmax = UnitPowerMax(token); if mpmax <= 0 then mpmax = 1 end
		mp:SetMinMaxValues(0, mpmax); mp:SetValue(UnitPower(token))
		name:SetText(UnitName(token))
		lvl:SetText('Lv ' .. UnitLevel(token))
	end

	local d = CreateFrame('Frame', nil, nil)
	d:SetScript('OnEvent', refresh)
	d:RegisterEvent(Events.UNIT_HEALTH);   d:RegisterEvent(Events.UNIT_MAXHEALTH)
	d:RegisterEvent(Events.UNIT_POWER);    d:RegisterEvent(Events.UNIT_MAXPOWER)
	d:RegisterEvent(Events.UNIT_LEVEL);    d:RegisterEvent(Events.PLAYER_TARGET_CHANGED)
	local acc = 0
	d:SetScript('OnUpdate', function(_, dt) acc = acc + dt; if acc >= 0.5 then acc = 0; refresh() end end)

	f:SetScript('OnClick', function() TargetUnit(token) end)
	f:Hide()   -- hidden until WORLD_SHOWN
	return { frame = f, hp = hp, mp = mp, name = name, lvl = lvl, refresh = refresh }
end

EmberUI.PlayerFrame = makeUnitFrame('player', 40, 20)
EmberUI.TargetFrame = makeUnitFrame('target', 280, 20)

-- HUD visibility:
--  * WORLD_SHOWN  fires when the world stage is created (the "Loading" screen is up, player not spawned yet)
--    -> retire the C++ frames now so nothing shows during loading, but keep the Lua HUD hidden too.
--  * PLAYER_LOGIN fires when the player is actually in control (loading done) -> reveal the Lua HUD.
--  * glue stages -> hide.
local stage = CreateFrame('Frame', nil, nil)
stage:SetScript('OnEvent', function(_, event)
	if event == Events.WORLD_SHOWN then
		SetGameFrameShown('PlayerFrame', false)
		SetGameFrameShown('TargetFrame', false)
	elseif event == Events.PLAYER_LOGIN then
		EmberUI.inWorld = true
		EmberUI.PlayerFrame.refresh()
		EmberUI.TargetFrame.refresh()
	else
		EmberUI.inWorld = false
		EmberUI.PlayerFrame.frame:Hide()
		EmberUI.TargetFrame.frame:Hide()
	end
end)
stage:RegisterEvent(Events.WORLD_SHOWN)
stage:RegisterEvent(Events.PLAYER_LOGIN)
stage:RegisterEvent(Events.LOGIN_SHOWN)
stage:RegisterEvent(Events.CHARSELECT_SHOWN)
stage:RegisterEvent(Events.CHARCREATE_SHOWN)

print('EmberUI unit frames loaded')
