-- Lua character-selection screen, replacing the C++ CharacterSelection (force-hidden; its data/enter/delete
-- flows stay C++ and are driven via GetCharacterInfo / EnterWorldWithCharacter / DeleteCharacter -- the
-- pending-self stash and dev AutoLogin live engine-side). Layout mirrors CharacterSelection.cpp: 3 slots a
-- page at (27, 125+90i) on the centered main_selection.png, portraits square-cropped from the sheet via the
-- GetPortraitInfo offset. Click a slot = confirm + enter; right-click = delete menu + confirm.

local SLOT_X, SLOT_Y0, SLOT_H = 27, 125, 90

local sw, sh = GetScreenWidth(), GetScreenHeight()
local gw, gh = GetTextureSize('main_selection.png')
if gw <= 0 then gw, gh = 460, 560 end

local screen = CreateFrame('Frame', 'EmberCharSelect', nil)
screen:SetSize(sw, sh); screen:SetPoint('TOPLEFT', 0, 0); screen:Hide()
local bg = screen:CreateTexture(); bg:SetTexture('main_bg.jpg')
bg:SetPoint('TOPLEFT', 0, 0); bg:SetSize(sw, sh)

local panel = CreateFrame('Frame', nil, screen)
panel:SetSize(gw, gh); panel:SetPoint('TOPLEFT', math.floor((sw - gw) / 2), math.floor((sh - gh) / 2))
local art = panel:CreateTexture(); art:SetTexture('main_selection.png'); art:SetAllPoints(panel)   -- fill the (scaled) panel so the frame art tracks the slots at any UI scale

local scroll = 0
local refresh   -- forward decl
local pendingConfirm = nil   -- { code, action='enter'|'delete', idx }

local function setPortrait(tex, id, gender)
	local name, offY = GetPortraitInfo(id, gender)
	if name == '' then tex:Hide(); return end
	local tw, th = GetTextureSize(name)
	if tw <= 0 then tex:Hide(); return end
	tex:SetTexture(name)
	tex:SetTexCoord(0, 1, offY / th, (offY + tw) / th)   -- square crop at the sheet's face offset
	tex:Show()
end

local slots = {}
for i = 1, 3 do
	local y = SLOT_Y0 + (i - 1) * SLOT_H
	local btn = CreateFrame('Button', nil, panel)
	btn:SetTexture('char_slot_idle.png'); btn:SetHoverTexture('char_slot_hover.png')
	local bw, bh = GetTextureSize('char_slot_idle.png')
	btn:SetSize(bw > 0 and bw or 400, bh > 0 and bh or 80)
	btn:SetPoint('TOPLEFT', panel, 'TOPLEFT', SLOT_X, y); btn:EnableMouse(true)

	local portrait = btn:CreateTexture()
	portrait:SetSize(55, 55); portrait:SetPoint('TOPLEFT', btn, 'TOPLEFT', 17, 15)

	local name = panel:CreateFontString()
	name:SetFont('Ringbearer'); name:SetFontSize(22); name:SetTextColor(221, 197, 185, 255)
	name:SetPoint('TOPLEFT', panel, 'TOPLEFT', SLOT_X + 88, y + 17)

	local sub = panel:CreateFontString()
	sub:SetFont('Cambria'); sub:SetFontSize(15); EmberUI.SetColor(sub, EmberUI.Color.Muted)
	sub:SetPoint('TOPLEFT', panel, 'TOPLEFT', SLOT_X + 88, y + 45)

	local slot = { btn = btn, portrait = portrait, name = name, sub = sub }
	btn:SetScript('OnMouseUp', function(_, mbtn)
		if not slot._idx then return end
		if mbtn == 'RightButton' then
			EmberUI.ShowMenu({
				{ text = 'Delete ' .. slot._name, color = { 220, 90, 90 }, onClick = function()
					pendingConfirm = { code = ShowConfirm('Are you sure you want to DELETE ' .. slot._name .. '?'),
					                   action = 'delete', idx = slot._idx }
				end },
				{ text = 'Cancel' },
			})
		else
			pendingConfirm = { code = ShowConfirm('Connect with ' .. slot._name .. '?'),
			                   action = 'enter', idx = slot._idx }
		end
	end)
	slots[i] = slot
end

local pageFS = panel:CreateFontString()
pageFS:SetFont('Palatino'); pageFS:SetFontSize(18); EmberUI.SetColor(pageFS, EmberUI.Color.Muted)
pageFS:SetPoint('TOPLEFT', panel, 'TOPLEFT', 190, 415)

local function navArrow(tex, x, d)
	local b = CreateFrame('Button', nil, panel)
	b:SetTexture(tex .. '_idle.png'); b:SetHoverTexture(tex .. '_hover.png')
	local w, h = GetTextureSize(tex .. '_idle.png')
	b:SetSize(w > 0 and w or 24, h > 0 and h or 24)
	b:SetPoint('TOPLEFT', panel, 'TOPLEFT', x, 415); b:EnableMouse(true)
	b:SetScript('OnClick', function()
		local n = GetNumCharacters()
		local s = scroll + d * 3
		if s >= 0 and s < n then scroll = s end
		refresh()
	end)
	return b
end
local leftBtn = navArrow('left_arrow', 50, -1)
local rightBtn = navArrow('right_arrow', 360, 1)

local createBtn = CreateFrame('Button', nil, panel)
createBtn:SetTexture('create_character_idle.png'); createBtn:SetHoverTexture('create_character_hover.png')
local cw, ch = GetTextureSize('create_character_idle.png')
createBtn:SetSize(cw > 0 and cw or 180, ch > 0 and ch or 50)
createBtn:SetPoint('TOPLEFT', panel, 'TOPLEFT', 137, 480); createBtn:EnableMouse(true)
createBtn:SetScript('OnClick', function() GotoCharCreate() end)

refresh = function()
	local n = GetNumCharacters()
	if scroll >= n then scroll = math.max(0, n - 3) end
	for i = 1, 3 do
		local idx = scroll + i
		local s = slots[i]
		if idx <= n then
			local name, classId, level, portrait, gender = GetCharacterInfo(idx)
			s._idx, s._name = idx, name
			local className = select(1, GetClassInfo(classId))
			s.name:SetText(string.lower(name)); s.name:Show()
			s.sub:SetText(string.upper('Level ' .. level .. ' ' .. className)); s.sub:Show()
			setPortrait(s.portrait, portrait, gender)
			s.btn:Show()
		else
			s._idx = nil
			s.btn:Hide(); s.portrait:Hide(); s.name:Hide(); s.sub:Hide()
		end
	end
	pageFS:SetText('PAGE ' .. (1 + math.floor(scroll / 3)))
	if scroll >= 3 then leftBtn:Show() else leftBtn:Hide() end
	if scroll + 3 < n then rightBtn:Show() else rightBtn:Hide() end
end

-- drain the confirm dialogs (enter / delete)
local poll = CreateFrame('Frame', nil, nil)
poll:SetScript('OnUpdate', function()
	if not pendingConfirm then return end
	local code, yes = PopConfirm()
	if code == 0 then return end
	if code == pendingConfirm.code then
		if yes then
			if pendingConfirm.action == 'enter' then EnterWorldWithCharacter(pendingConfirm.idx)
			else DeleteCharacter(pendingConfirm.idx) end
		end
		pendingConfirm = nil
	end
end)

local stage = CreateFrame('Frame', nil, nil)
stage:SetScript('OnEvent', function(_, event)
	if event == Events.CHARSELECT_SHOWN then
		SetGameFrameShown('CharSelectScreen', false)
		scroll = 0
		screen:Show(); refresh()
	elseif event == Events.CHARLIST_UPDATE then
		refresh()
	else
		screen:Hide(); pendingConfirm = nil
	end
end)
for _, e in ipairs({ Events.CHARSELECT_SHOWN, Events.CHARLIST_UPDATE,
                     Events.LOGIN_SHOWN, Events.CHARCREATE_SHOWN, Events.WORLD_SHOWN }) do
	stage:RegisterEvent(e)
end

print('EmberUI character select loaded')
