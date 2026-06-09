-- Lua Abilities / spellbook (DISPLAY-ONLY) replacing the C++ Abilities window (force-hidden, mirrored). The
-- live window stays the model (populated by GP_Server_Spellbook); this view reads it via GetNumSpellSlots /
-- GetSpellSlot and renders a VERTICAL list (icon + name + description per row), matching the C++ GameIconList
-- (6 rows, 75px tall, single column, descriptions on). Two tabs: Spellbook (stage 1) and Misc (stage 0).
-- Casting + drag-to-toolbar + spell-point spending are DEFERRED (cast is via the toolbar). Event-driven:
-- PANEL_OPENED/CLOSED('AbilitiesFrame') + SPELLBOOK_UPDATE. NO OnUpdate. Positions from C++ Abilities ctor.
-- v1: description is single-line (no FontString:SetWidth/word-wrap binding yet) — may truncate; tune live.

local LIST_X, LIST_Y, ROW_H, VISIBLE = 25, 126, 75, 6
local ICON_DX, ICON_DY, ICON = 18, 14, 48
local NAME_DX, NAME_DY = 52, 14
local DESC_DX, DESC_DY = 52, 38
local TAB_Y = 70

local W, H = GetTextureSize('abilities.png')
if W <= 0 then W, H = 470, 540 end
local root = CreateFrame('Frame', 'EmberAbilities', nil)
root:SetSize(W, H)
root:SetPoint('CENTER')
root:SetMovable(true); root:RegisterForDrag('LeftButton')
root:Hide()

local function vis(f, v) if v then f:Show() else f:Hide() end end

local bg = root:CreateTexture()
bg:SetAllPoints(root)
bg:SetTexture('abilities.png')

local currentTab = 1   -- 1 = Spellbook (stage 1), 2 = Misc (stage 0)
local scroll = 0
local refresh   -- forward decl
local function tabStage() return currentTab == 1 and 1 or 0 end

-- tabs: the labels "SPELLBOOK" / "MISC ACTIONS" are printed on abilities.png, so just put invisible click
-- regions over them + a bright caret marking the active tab (don't draw our own labels = no doubling).
local TAB_X = { 133, 243 }
local activeMarker = root:CreateFontString()
activeMarker:SetFont('Trebuchet'); activeMarker:SetFontSize(14); activeMarker:SetTextColor(255, 224, 150, 255)
activeMarker:SetText('>')
for t = 1, 2 do
	local b = CreateFrame('Button', nil, root)
	b:SetSize(100, 24); b:SetPoint('TOPLEFT', root, 'TOPLEFT', TAB_X[t], TAB_Y); b:EnableMouse(true)
	b:SetScript('OnClick', function() currentTab = t; scroll = 0; refresh() end)
end

-- vertical spell-row pool: icon (left) + name + description
local rows = {}
for r = 1, VISIBLE do
	local y = LIST_Y + (r - 1) * ROW_H
	local icon = CreateFrame('Button', nil, root)
	icon:SetSize(ICON, ICON); icon:SetPoint('TOPLEFT', root, 'TOPLEFT', LIST_X + ICON_DX, y + ICON_DY)
	icon:EnableMouse(true); icon:SetHoverTexture('gameicon40_hover.png')
	local nameFS = root:CreateFontString(); nameFS:SetFont('Ringbearer'); nameFS:SetFontSize(15); nameFS:SetTextColor(168, 155, 137, 255)
	nameFS:SetPoint('TOPLEFT', root, 'TOPLEFT', LIST_X + NAME_DX, y + NAME_DY)
	local descFS = root:CreateFontString(); descFS:SetFont('Palatino'); descFS:SetFontSize(12); descFS:SetTextColor(111, 99, 79, 255)
	descFS:SetPoint('TOPLEFT', root, 'TOPLEFT', LIST_X + DESC_DX, y + DESC_DY)
	rows[r] = { icon = icon, name = nameFS, desc = descFS }
end

refresh = function()
	activeMarker:SetPoint('TOPLEFT', root, 'TOPLEFT', TAB_X[currentTab] - 14, TAB_Y + 2)

	local stage = tabStage()
	local n = GetNumSpellSlots(stage)
	local maxS = math.max(0, n - VISIBLE)
	if scroll > maxS then scroll = maxS end
	if scroll < 0 then scroll = 0 end

	for r = 1, VISIBLE do
		local idx = scroll + r
		local row = rows[r]
		if idx <= n then
			local spellId = GetSpellSlot(stage, idx)
			local tex = GetSpellTexture(spellId)
			if tex and tex ~= '' then row.icon:SetTexture(tex) end
			row.icon:SetTooltipSpell(spellId)
			row.name:SetText(GetSpellName(spellId) or ''); row.name:Show()
			row.desc:SetText(GetSpellDescription(spellId) or ''); row.desc:Show()
			vis(row.icon, true)
		else
			row.icon:SetTooltipSpell(0)
			row.name:Hide(); row.desc:Hide()
			vis(row.icon, false)
		end
	end
end

root:SetScript('OnMouseWheel', function(_, delta) scroll = scroll - delta; refresh() end)

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
