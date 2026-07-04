-- Lua character-creation screen, replacing the C++ CharacterCreation (force-hidden). Layout mirrors
-- CharacterCreation.cpp on the centered main_creation.png: class row (4x create_class at 44+60i,465),
-- gender pair (805/879,465), portrait circle at screen center (the C++ renderAsCircle -> SetCircular)
-- with prev/next arrows (350/670,350), name prompt (380,610), Create (447,686), Cancel/back at the left
-- edge. CreateCharacter validates the name engine-side and returns an error string to display. Class
-- descriptions (the C++ hover tooltips) show as a text block for the SELECTED class instead.

local CLASSES = {
	{ id = 1, desc = 'Divine warriors devoted to honor and piety. They live to defend, protect and serve all believers. Can use Shields and Melee weapons.', diff = 'Beginner', diffColor = { 145, 233, 1 } },
	{ id = 2, desc = 'Powerful arcane spellcasters born with magic in their blood who gradually learn to harness their innate abilities over time. Can use Staves and Wands.', diff = 'Advanced', diffColor = { 215, 63, 19 } },
	{ id = 3, desc = 'A wanderer in the wilds, their fierce independence makes them well suited to adventuring and mischief. Can use Bows and Daggers.', diff = 'Moderate', diffColor = { 226, 218, 0 } },
	{ id = 4, desc = 'Best known as healers, they are members of the divine faith who have the ability to cast priest spells from the gods they worship. Can use Shields, Staves and Maces.', diff = 'Moderate', diffColor = { 226, 218, 0 } },
}
local GENDER_MALE, GENDER_FEMALE = 0, 1

local sw, sh = GetScreenWidth(), GetScreenHeight()
local gw, gh = GetTextureSize('main_creation.png')
if gw <= 0 then gw, gh = 980, 760 end

local screen = CreateFrame('Frame', 'EmberCharCreate', nil)
screen:SetSize(sw, sh); screen:SetPoint('TOPLEFT', 0, 0); screen:Hide()
local bg = screen:CreateTexture(); bg:SetTexture('main_bg.jpg')
bg:SetPoint('TOPLEFT', 0, 0); bg:SetSize(sw, sh)

local panel = CreateFrame('Frame', nil, screen)
panel:SetSize(gw, gh); panel:SetPoint('TOPLEFT', math.floor((sw - gw) / 2), math.floor((sh - gh) / 2))
local art = panel:CreateTexture(); art:SetTexture('main_creation.png'); art:SetPoint('TOPLEFT', 0, 0)

local selClass = 1        -- Paladin default (the C++ default)
local gender = GENDER_MALE
local portraitId = 1
local refresh             -- forward decl

-- selection marker (create_class_press.png) -- one for class, one for gender
local function marker()
	local t = screen:CreateTexture()
	t:SetTexture('create_class_press.png')
	local w, h = GetTextureSize('create_class_press.png')
	t:SetSize(w > 0 and w or 56, h > 0 and h or 56)
	return t
end
local classMark, genderMark = marker(), marker()

-- class row
local classBtns = {}
for i, def in ipairs(CLASSES) do
	local b = CreateFrame('Button', nil, panel)
	b:SetTexture('create_class_idle.png'); b:SetHoverTexture('create_class_hover.png')
	local w, h = GetTextureSize('create_class_idle.png')
	b:SetSize(w > 0 and w or 56, h > 0 and h or 56)
	b:SetPoint('TOPLEFT', panel, 'TOPLEFT', 44 + 60 * (i - 1), 465); b:EnableMouse(true)
	b:SetScript('OnClick', function() selClass = def.id; refresh() end)
	classBtns[i] = b
end

-- class info block (replaces the C++ hover tooltips: always shows the SELECTED class)
local classNameFS = panel:CreateFontString()
classNameFS:SetFont('Helvetica'); classNameFS:SetFontSize(16)
classNameFS:SetPoint('TOPLEFT', panel, 'TOPLEFT', 44, 530)
local classDescFS = panel:CreateFontString()
classDescFS:SetFont('Helvetica'); classDescFS:SetFontSize(13); classDescFS:SetTextColor(220, 220, 220, 255)
classDescFS:SetPoint('TOPLEFT', panel, 'TOPLEFT', 44, 552); classDescFS:SetWidth(280)
local classDiffFS = panel:CreateFontString()
classDiffFS:SetFont('Trebuchet'); classDiffFS:SetFontSize(14)
classDiffFS:SetPoint('TOPLEFT', panel, 'TOPLEFT', 44, 555)   -- repositioned in refresh under the desc

-- gender pair (male right at 879 per the C++ ids; female at 805)
local genderBtns = {}
for _, def in ipairs({ { g = GENDER_FEMALE, x = 805 }, { g = GENDER_MALE, x = 879 } }) do
	local b = CreateFrame('Button', nil, panel)
	b:SetTexture('create_class_idle.png'); b:SetHoverTexture('create_class_hover.png')
	local w, h = GetTextureSize('create_class_idle.png')
	b:SetSize(w > 0 and w or 56, h > 0 and h or 56)
	b:SetPoint('TOPLEFT', panel, 'TOPLEFT', def.x, 465); b:EnableMouse(true)
	b:SetScript('OnClick', function()
		if gender ~= def.g then gender = def.g; portraitId = 1; refresh() end
	end)
	genderBtns[def.g] = b
end

-- portrait circle at screen center (the C++ renderAsCircle radius 105)
local portraitTex = screen:CreateTexture()
portraitTex:SetSize(210, 210)
portraitTex:SetPoint('TOPLEFT', math.floor(sw / 2) - 105, math.floor(sh / 2) - 105)
portraitTex:SetCircular(105)

local function navArrow(tex, x, d)
	local b = CreateFrame('Button', nil, panel)
	b:SetTexture(tex .. '_idle.png'); b:SetHoverTexture(tex .. '_hover.png')
	local w, h = GetTextureSize(tex .. '_idle.png')
	b:SetSize(w > 0 and w or 24, h > 0 and h or 24)
	b:SetPoint('TOPLEFT', panel, 'TOPLEFT', x, 350); b:EnableMouse(true)
	b:SetScript('OnClick', function()
		local n = GetNumPortraits(gender)
		portraitId = portraitId + d
		if portraitId < 1 then portraitId = n end
		if portraitId > n then portraitId = 1 end
		refresh()
	end)
end
navArrow('left_arrow', 350, -1)
navArrow('right_arrow', 670, 1)

-- name prompt
local nameBox = CreateFrame('EditBox', nil, panel)
nameBox:SetSize(200, 24); nameBox:SetPoint('TOPLEFT', panel, 'TOPLEFT', 380, 610)
nameBox:SetMaxLetters(12); nameBox:SetFontSize(18); nameBox:SetTextColor(157, 137, 119, 255)

-- error line (the C++ used a popup)
local errFS = panel:CreateFontString()
errFS:SetFont('Helvetica'); errFS:SetFontSize(14); errFS:SetTextColor(220, 90, 90, 255)
errFS:SetPoint('TOPLEFT', panel, 'TOPLEFT', 380, 660)

local function submit()
	local err = CreateCharacter(nameBox:GetText(), selClass, gender, portraitId)
	errFS:SetText(err or '')
end

local createBtn = CreateFrame('Button', nil, panel)
createBtn:SetTexture('generic_highlight_idle.png'); createBtn:SetHoverTexture('generic_highlight_hover.png')
local cw, ch2 = GetTextureSize('generic_highlight_idle.png')
createBtn:SetSize(cw > 0 and cw or 130, ch2 > 0 and ch2 or 28)
createBtn:SetPoint('TOPLEFT', panel, 'TOPLEFT', 447, 686); createBtn:EnableMouse(true)
createBtn:SetScript('OnClick', submit)
nameBox:SetScript('OnEnterPressed', submit)

local backBtn = CreateFrame('Button', nil, panel)
backBtn:SetTexture('cancel_back_idle.png'); backBtn:SetHoverTexture('cancel_back_hover.png')
local bw2, bh2 = GetTextureSize('cancel_back_idle.png')
backBtn:SetSize(bw2 > 0 and bw2 or 48, bh2 > 0 and bh2 or 48)
backBtn:SetPoint('TOPLEFT', panel, 'TOPLEFT', gw, math.floor(gh / 2) - 24)   -- the C++ right-edge spot; tune live
backBtn:EnableMouse(true)
backBtn:SetScript('OnClick', function() GotoCharSelect() end)

refresh = function()
	-- markers over the selected class + gender
	for i, def in ipairs(CLASSES) do
		if def.id == selClass then
			classMark:ClearAllPoints()
			classMark:SetPoint('TOPLEFT', panel, 'TOPLEFT', 44 + 60 * (i - 1), 465)
			classMark:Show()
		end
	end
	genderMark:ClearAllPoints()
	genderMark:SetPoint('TOPLEFT', panel, 'TOPLEFT', gender == GENDER_MALE and 879 or 805, 465)
	genderMark:Show()

	-- class info
	local def = CLASSES[selClass]
	local name, r, g, b = GetClassInfo(selClass)
	classNameFS:SetText(name); classNameFS:SetTextColor(r, g, b, 255)
	classDescFS:SetText(def.desc)
	classDiffFS:ClearAllPoints()
	classDiffFS:SetPoint('TOPLEFT', panel, 'TOPLEFT', 44, 552 + 70)   -- fixed offset under the wrapped desc
	classDiffFS:SetText('Difficulty: ' .. def.diff)
	classDiffFS:SetTextColor(def.diffColor[1], def.diffColor[2], def.diffColor[3], 255)

	-- portrait (square sheet crop + the circle clip)
	local texName, offY = GetPortraitInfo(portraitId, gender)
	if texName ~= '' then
		local tw, th = GetTextureSize(texName)
		if tw > 0 then
			portraitTex:SetTexture(texName)
			portraitTex:SetTexCoord(0, 1, offY / th, (offY + tw) / th)
			portraitTex:Show()
		end
	end
end

local stage = CreateFrame('Frame', nil, nil)
stage:SetScript('OnEvent', function(_, event)
	if event == Events.CHARCREATE_SHOWN then
		SetGameFrameShown('CharCreateScreen', false)
		selClass = 1
		gender = GENDER_MALE
		portraitId = math.random(1, GetNumPortraits(GENDER_MALE))
		nameBox:SetText(''); errFS:SetText('')
		screen:Show(); refresh()
	else
		screen:Hide()
	end
end)
for _, e in ipairs({ Events.CHARCREATE_SHOWN,
                     Events.LOGIN_SHOWN, Events.CHARSELECT_SHOWN, Events.WORLD_SHOWN }) do
	stage:RegisterEvent(e)
end

print('EmberUI character create loaded')
