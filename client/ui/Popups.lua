-- Lua timed popups (toasts). Replaces the C++ TimedMessageBox: Application::spawnTimedPopup now fires
-- UI_POPUP with "<seconds>|<text>", and clearPopups fires UI_POPUP_CLEAR. Shown upper-center, auto-hidden
-- after the duration (or on clear). Loaded last so it renders above the rest of the default UI.

local sw, sh = GetScreenWidth(), GetScreenHeight()

local f = CreateFrame('Frame', 'EmberUI_Popup', nil)
f:SetSize(260, 40); f:SetPoint('CENTER', 0, -150); f:Hide()
f:SetFrameLevel(100)   -- toasts above the HUD (but below the loading screen at 200)
local txt = f:CreateFontString(); txt:SetPoint('TOPLEFT', 0, 0)

local remaining = 0

local d = CreateFrame('Frame', nil, nil)
d:SetScript('OnEvent', function(_, event, arg)
	if event == Events.UI_POPUP then
		local sep = arg:find('|', 1, true)
		local secs = sep and tonumber(arg:sub(1, sep - 1)) or 3
		txt:SetText(sep and arg:sub(sep + 1) or arg)
		remaining = secs
		f:Show()
	elseif event == Events.UI_POPUP_CLEAR then
		remaining = 0
		f:Hide()
	end
end)
d:RegisterEvent(Events.UI_POPUP)
d:RegisterEvent(Events.UI_POPUP_CLEAR)

d:SetScript('OnUpdate', function(_, dt)
	if remaining > 0 then
		remaining = remaining - dt
		if remaining <= 0 then remaining = 0; f:Hide() end
	end
end)

print('EmberUI popups loaded')
