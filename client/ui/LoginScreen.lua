-- Lua login screen — display parity with the original C++ Login. Replaces the C++ Login (force-hidden,
-- rollback-safe). The C++ Login still exists as the stage object and runs the actual auth/connect (via
-- SubmitLogin -> Login::performLogin); Lua is the View.
--
-- Layout mirrors Login.cpp: the GUI art (main_login.png, 699x606) is centered at ((sW-699)/2, (sH-750)/3),
-- and the fields/buttons sit at the same art-relative offsets the C++ used.

local sw, sh = GetScreenWidth(), GetScreenHeight()
local px = (sw - 699) / 2          -- C++: (sWf - guiWidth)/2
local py = (sh - 750) / 3          -- C++: (sHf - 750)/3   (literal 750, not the art's 606)

-- Root: full-screen, holds the stretched background. Hidden until LOGIN_SHOWN.
local screen = CreateFrame('Frame', 'EmberUI_LoginScreen', nil)
screen:SetSize(sw, sh); screen:SetPoint('TOPLEFT', 0, 0); screen:Hide()

local bg = screen:CreateTexture(); bg:SetTexture('main_bg.jpg')
bg:SetPoint('TOPLEFT', 0, 0); bg:SetSize(sw, sh)   -- stretch to the render size (no black bar)

-- Login GUI panel (the framed art). Children anchor to its top-left, same as the C++ holder.
local panel = CreateFrame('Frame', 'EmberUI_LoginPanel', screen)
panel:SetSize(699, 606); panel:SetPoint('TOPLEFT', px, py)
local art = panel:CreateTexture(); art:SetTexture('main_login.png'); art:SetPoint('TOPLEFT', 0, 0)

-- Account field: Palatino 20, RGBA(220,194,171) — these are the LuaEditBox defaults, so no overrides needed.
local user = CreateFrame('EditBox', nil, panel)
user:SetSize(300, 26); user:SetPoint('TOPLEFT', 180, 400); user:SetMaxLetters(45)

-- Password field: Palatino 24, RGBA(157,137,119), masked.
local pass = CreateFrame('EditBox', nil, panel)
pass:SetSize(400, 30); pass:SetPoint('TOPLEFT', 180, 480); pass:SetMaxLetters(45)
pass:SetPassword(true); pass:SetFontSize(24); pass:SetTextColor(157, 137, 119, 255)

-- Remember tickbox (initial ticked = true, like the C++). Toggles the sprite on click.
local remembered = true
local remember = CreateFrame('Button', nil, panel)
remember:SetSize(32, 32); remember:SetPoint('TOPLEFT', 260, 580); remember:SetTexture('tick_box_full.png')
remember:SetScript('OnClick', function()
	remembered = not remembered
	remember:SetTexture(remembered and 'tick_box_full.png' or 'tick_box_empty.png')
end)

-- Login button (login_idle.png, 288x103). Return-to-login is covered by the fields' OnEnter.
local login = CreateFrame('Button', nil, panel)
login:SetTexture('login_idle.png'); login:SetHoverTexture('login_hover.png')
login:SetSize(288, 103); login:SetPoint('TOPLEFT', 203, 649)

-- Status line (the C++ used timed popups; we show a small line under the button).
local status = panel:CreateFontString(); status:SetPoint('TOPLEFT', 203, 762)

local function submit()
	status:SetText('Connecting...')
	SubmitLogin(user:GetText(), pass:GetText(), remembered)
end
login:SetScript('OnClick', submit)
user:SetScript('OnEnter', submit)
pass:SetScript('OnEnter', submit)

-- Status follows the C++ login result (auth/connect failures + server validate failures).
local res = CreateFrame('Frame', nil, nil)
res:SetScript('OnEvent', function(_, _, msg) status:SetText(msg) end)
res:RegisterEvent(Events.LOGIN_RESULT)

-- Show only on the login stage; hide elsewhere. Force-hide the C++ login when we take over.
local stage = CreateFrame('Frame', nil, nil)
stage:SetScript('OnEvent', function(_, event)
	if event == Events.LOGIN_SHOWN then
		user:SetText(GetSavedLogin())
		status:SetText('')
		screen:Show()
		SetGameFrameShown('LoginScreen', false)
	else
		screen:Hide()
	end
end)
stage:RegisterEvent(Events.LOGIN_SHOWN)
stage:RegisterEvent(Events.CHARSELECT_SHOWN)
stage:RegisterEvent(Events.CHARCREATE_SHOWN)
stage:RegisterEvent(Events.WORLD_SHOWN)

EmberUI.LoginScreen = screen

print('EmberUI login screen loaded')
