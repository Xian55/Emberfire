-- Player + target unit frames: name, level, health bar, mana bar; click to target.
-- Built entirely from the C++ API (getters + events + the TargetUnit command).

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
		if token == 'target' then
			if UnitExists('target') then f:Show() else f:Hide(); return end
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
	refresh()
	return { frame = f, hp = hp, mp = mp, name = name, lvl = lvl, refresh = refresh }
end

EmberUI.PlayerFrame = makeUnitFrame('player', 40, 20)
EmberUI.TargetFrame = makeUnitFrame('target', 280, 20)
EmberUI.TargetFrame.frame:Hide()   -- no target at start

-- Replace the C++ player/target unit frames with these Lua ones (hidden, not destroyed — rollback-safe).
SetGameFrameShown('PlayerFrame', false)
SetGameFrameShown('TargetFrame', false)

print('EmberUI unit frames loaded')
