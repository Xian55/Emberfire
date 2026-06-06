-- Player buff/debuff row next to the minimap (replaces the C++ World aura rendering). Uses the shared aura
-- widget with the remaining-time text enabled. Layout mirrors the C++ one: 40px icons, expand left from the
-- minimap, buffs on the top row, debuffs below.

local SIZE, SPACING, MAX = 40, 45, 8

-- Anchor near the minimap (top-right), matching World.cpp: min(sW-260, sW/2+700) - 100.
local function anchorX() return math.min(GetScreenWidth() - 260, GetScreenWidth() / 2 + 700) - 100 end

local holder = CreateFrame('Frame', 'EmberUI_PlayerAuras', nil)
holder:SetSize(1, 1); holder:SetPoint('TOPLEFT', 0, 0)

local buffs, debuffs = {}, {}
local function buildRow(tbl, rowY)
	local ax = anchorX()
	for i = 1, MAX do
		local a = EmberUI.CreateAura(holder, SIZE, { duration = true })
		a.frame:SetPoint('TOPLEFT', ax - SIZE - (i - 1) * SPACING, rowY)   -- expand left
		tbl[i] = a
	end
end
buildRow(buffs, 15)
buildRow(debuffs, 15 + 90)

local function refreshRow(tbl, harmful)
	local n = UnitAuraCount('player', harmful)
	for i = 1, MAX do
		if i <= n then
			local sid, _, rem, dur = UnitAura('player', i, harmful)
			tbl[i]:SetSpell(sid, rem, dur)
		else
			tbl[i]:Hide()
		end
	end
end

local function refresh()
	if not EmberUI.inWorld then return end
	refreshRow(buffs, false)
	refreshRow(debuffs, true)
end

local function hideAll()
	for i = 1, MAX do buffs[i]:Hide(); debuffs[i]:Hide() end
end

local d = CreateFrame('Frame', nil, nil)
d:SetScript('OnEvent', refresh)
d:RegisterEvent(Events.UNIT_AURA)
local acc = 0
d:SetScript('OnUpdate', function(_, dt) if EmberUI.inWorld then acc = acc + dt; if acc >= 0.5 then acc = 0; refresh() end end end)

local stage = CreateFrame('Frame', nil, nil)
stage:SetScript('OnEvent', function(_, event)
	if event == Events.WORLD_SHOWN then
		SetGameFrameShown('PlayerAuras', false)   -- retire the C++ minimap aura row
	elseif event == Events.PLAYER_LOGIN then
		refresh()
	else
		hideAll()
	end
end)
stage:RegisterEvent(Events.WORLD_SHOWN)
stage:RegisterEvent(Events.PLAYER_LOGIN)
stage:RegisterEvent(Events.LOGIN_SHOWN)
stage:RegisterEvent(Events.CHARSELECT_SHOWN)
stage:RegisterEvent(Events.CHARCREATE_SHOWN)

print('EmberUI player auras loaded')
