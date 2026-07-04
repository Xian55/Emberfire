-- Lua Abilities / spellbook (DISPLAY-ONLY) replacing the C++ Abilities window (force-hidden, mirrored). The
-- live window stays the model (populated by GP_Server_Spellbook); this view reads it via GetNumSpellSlots /
-- GetSpellSlot and renders a VERTICAL list (icon + name + description per row), matching the C++ GameIconList
-- (6 rows, 75px tall, single column, descriptions on). Two tabs: Spellbook (stage 1) and Misc (stage 0).
-- Casting + drag-to-toolbar + spell-point spending are DEFERRED (cast is via the toolbar). Event-driven:
-- PANEL_OPENED/CLOSED('AbilitiesFrame') + SPELLBOOK_UPDATE. NO OnUpdate. Positions from C++ Abilities ctor.
-- v1: description is single-line (no FontString:SetWidth/word-wrap binding yet) — may truncate; tune live.

local LIST_X, LIST_Y, ROW_H, VISIBLE = 25, 126, 75, 6
local ICON_DX, ICON_DY, ICON = 18, 14, 48
local NAME_DX, NAME_DY = 75, 14
local DESC_DX, DESC_DY = 75, 38
local TAB_Y = 70

local W, H = 474, 592
local win = EmberUI.CreateWindow{ name = 'EmberAbilities', width = W, height = H }
local root = win.frame
root:SetPoint('CENTER')
root:Hide()

local function vis(f, v) if v then f:Show() else f:Hide() end end

local currentTab = 1   -- 1 = Spellbook (stage 1), 2 = Misc (stage 0)
local scroll = 0
local refresh   -- forward decl
local function tabStage() return currentTab == 1 and 1 or 0 end

-- tabs: SPELLBOOK / MISC ACTIONS are now real Lua labels (baked abilities.png retired), so the active tab
-- recolors instead of needing a caret overlay. A click region sits under each label.
local TAB_X = { 133, 243 }
local TAB_NAMES = { 'SPELLBOOK', 'MISC ACTIONS' }
local tabFS = {}
for t = 1, 2 do
	local b = CreateFrame('Button', nil, root)
	b:SetSize(120, 24); b:SetPoint('TOPLEFT', root, 'TOPLEFT', TAB_X[t], TAB_Y); b:EnableMouse(true)
	b:SetScript('OnClick', function() currentTab = t; scroll = 0; refresh() end)
	local fs = root:CreateFontString(); fs:SetFont('Ringbearer'); fs:SetFontSize(14)
	fs:SetPoint('TOPLEFT', root, 'TOPLEFT', TAB_X[t], TAB_Y + 3); fs:SetText(TAB_NAMES[t])
	tabFS[t] = fs
end

-- vertical spell-row pool: a row-spanning hit area (so hovering ANYWHERE on the row shows the tooltip) + a
-- child icon TEXTURE (a texture, not the button's own texture, so a hover never replaces/hides the icon) +
-- name + word-wrapped description.
local ROW_W = W - LIST_X * 2
local DESC_W = ROW_W - DESC_DX - 8
local rows = {}
for r = 1, VISIBLE do
	local y = LIST_Y + (r - 1) * ROW_H
	local rowBtn = CreateFrame('Button', nil, root)
	rowBtn:SetSize(ROW_W, ROW_H); rowBtn:SetPoint('TOPLEFT', root, 'TOPLEFT', LIST_X, y); rowBtn:EnableMouse(true)
	local iconTex = rowBtn:CreateTexture()
	iconTex:SetSize(ICON, ICON); iconTex:SetPoint('TOPLEFT', rowBtn, 'TOPLEFT', ICON_DX, ICON_DY)
	local nameFS = root:CreateFontString(); nameFS:SetFont('Ringbearer'); nameFS:SetFontSize(15); EmberUI.SetColor(nameFS, EmberUI.Color.Title)
	nameFS:SetPoint('TOPLEFT', root, 'TOPLEFT', LIST_X + NAME_DX, y + NAME_DY)
	local descFS = root:CreateFontString(); descFS:SetFont('Palatino'); descFS:SetFontSize(12); EmberUI.SetColor(descFS, EmberUI.Color.Body)
	descFS:SetPoint('TOPLEFT', root, 'TOPLEFT', LIST_X + DESC_DX, y + DESC_DY); descFS:SetWidth(DESC_W)
	local rd = { row = rowBtn, icon = iconTex, name = nameFS, desc = descFS, spellId = 0 }
	-- left-drag a spell onto an action slot: put it on the shared cursor; the ActionBar consumes the drop
	-- (PlaceAction) or, if released on nothing, clears it. Greys the row icon while held.
	rowBtn:SetScript('OnMouseDown', function(_, btn)
		if btn ~= 'LeftButton' or EmberUI.HasCursorItem() or rd.spellId == 0 then return end
		EmberUI.PickupItem(GetSpellTexture(rd.spellId), iconTex, { kind = 'spell', id = rd.spellId })
	end)
	rows[r] = rd
end

refresh = function()
	for t = 1, 2 do EmberUI.SetColor(tabFS[t], t == currentTab and EmberUI.Color.Title or EmberUI.Color.Muted) end

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
			row.spellId = spellId
			local tex = GetSpellTexture(spellId)
			if tex and tex ~= '' then row.icon:SetTexture(tex) end
			row.icon:Show()
			row.row:SetTooltipSpell(spellId)   -- tooltip on hovering anywhere in the row
			row.name:SetText(GetSpellName(spellId) or ''); row.name:Show()
			row.desc:SetText(GetSpellDescription(spellId) or ''); row.desc:Show()
		else
			row.spellId = 0
			row.row:SetTooltipSpell(0)
			row.icon:Hide(); row.name:Hide(); row.desc:Hide()
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
