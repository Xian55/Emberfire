-- Lua Abilities / spellbook (DISPLAY-ONLY) replacing the C++ Abilities window (force-hidden, mirrored). The
-- live window stays the model (populated by GP_Server_Spellbook); this view reads it via GetNumSpellSlots /
-- GetSpellSlot and renders the spell icons (GetSpellTexture) with hover tooltips (SetTooltipSpell). Two tabs:
-- Spellbook (stage 1) and Misc (stage 0). Casting + drag-to-toolbar are DEFERRED (cast is via the toolbar;
-- spell-point spending is the deferred Equipment-2b spell side). Event-driven: PANEL_OPENED/CLOSED +
-- SPELLBOOK_UPDATE. NO OnUpdate. NOTE: grid positions are guesses — tune live.

local COLS, ICON, PITCH, VISIBLE_ROWS = 6, 40, 46, 5
local GRID_X, GRID_Y = 40, 86
local VISIBLE = COLS * VISIBLE_ROWS
local TAB_Y = 52

local W, H = GetTextureSize('abilities.png')
if W <= 0 then W, H = 470, 540 end
local root = CreateFrame('Frame', 'EmberAbilities', nil)
root:SetSize(W, H)
root:SetPoint('CENTER')
root:SetMovable(true); root:RegisterForDrag('LeftButton')
root:Hide()

local function vis(f, v) if v then f:Show() else f:Hide() end end
local function texSize(name, dw, dh) local w, h = GetTextureSize(name); if w <= 0 then return dw, dh end return w, h end

local bg = root:CreateTexture()
bg:SetAllPoints(root)
bg:SetTexture('abilities.png')

local currentTab = 1   -- 1 = Spellbook (stage 1), 2 = Misc (stage 0)
local scroll = 0
local refresh   -- forward decl
local function tabStage() return currentTab == 1 and 1 or 0 end

-- tabs
local TABS = { { label = 'Spellbook', x = 40 }, { label = 'Misc', x = 150 } }
local tabFS = {}
for t, def in ipairs(TABS) do
	local b = CreateFrame('Button', nil, root)
	b:SetSize(96, 22); b:SetPoint('TOPLEFT', root, 'TOPLEFT', def.x, TAB_Y); b:EnableMouse(true)
	local fs = root:CreateFontString(); fs:SetFont('Ringbearer'); fs:SetFontSize(14)
	fs:SetPoint('TOPLEFT', root, 'TOPLEFT', def.x, TAB_Y); fs:SetText(def.label)
	b:SetScript('OnClick', function() currentTab = t; scroll = 0; refresh() end)
	tabFS[t] = fs
end

-- spell icon grid
local cells = {}
for c = 1, VISIBLE do
	local col = (c - 1) % COLS
	local row = math.floor((c - 1) / COLS)
	local b = CreateFrame('Button', nil, root)
	b:SetSize(ICON, ICON)
	b:SetPoint('TOPLEFT', root, 'TOPLEFT', GRID_X + col * PITCH, GRID_Y + row * PITCH)
	b:EnableMouse(true); b:SetHoverTexture('gameicon40_hover.png')
	cells[c] = b
end

refresh = function()
	for t, fs in ipairs(tabFS) do
		fs:SetTextColor(t == currentTab and 255 or 150, t == currentTab and 224 or 150, t == currentTab and 120 or 150, 255)
	end

	local stage = tabStage()
	local n = GetNumSpellSlots(stage)
	local maxS = math.max(0, n - VISIBLE)
	if scroll > maxS then scroll = maxS end
	if scroll < 0 then scroll = 0 end

	for c = 1, VISIBLE do
		local idx = scroll + c
		local b = cells[c]
		if idx <= n then
			local spellId = GetSpellSlot(stage, idx)
			local tex = GetSpellTexture(spellId)
			if tex and tex ~= '' then b:SetTexture(tex) end
			b:SetTooltipSpell(spellId)
			vis(b, true)
		else
			b:SetTooltipSpell(0)
			vis(b, false)
		end
	end
end

root:SetScript('OnMouseWheel', function(_, delta) scroll = scroll - delta * COLS; refresh() end)

local shown = false
local function setShown(v)
	if v == shown then return end
	shown = v
	if v then root:Show(); refresh() else root:Hide() end
end

local stage = CreateFrame('Frame', nil, nil)
stage:SetScript('OnEvent', function(_, event, arg)
	if event == Events.WORLD_SHOWN then
		SetGameFrameShown('AbilitiesFrame', false)
	elseif event == Events.PANEL_OPENED then
		if arg == 'AbilitiesFrame' then setShown(true) end
	elseif event == Events.PANEL_CLOSED then
		if arg == 'AbilitiesFrame' then setShown(false) end
	elseif event == Events.SPELLBOOK_UPDATE then
		if shown then refresh() end
	else
		setShown(false)
	end
end)
for _, e in ipairs({ Events.WORLD_SHOWN, Events.PANEL_OPENED, Events.PANEL_CLOSED, Events.SPELLBOOK_UPDATE,
                     Events.LOGIN_SHOWN, Events.CHARSELECT_SHOWN, Events.CHARCREATE_SHOWN }) do
	stage:RegisterEvent(e)
end

print('EmberUI abilities loaded')
