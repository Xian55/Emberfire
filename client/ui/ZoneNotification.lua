-- Zone notification banner (the "PILGRIM'S REFUGE / Friendly Territory" splash). The C++ ZoneNotifcation
-- keeps the zone detection + music/minimap logic and fires ZONE_CHANGED("name|type|r|g|b"); this draws the
-- banner with a fade-in -> hold -> fade-out animation (mirrors the original).

local sw = GetScreenWidth()
local FRAME_W, FRAME_H = 699, 130
local fx = math.floor(sw / 2 - FRAME_W / 2)

local banner = CreateFrame('Frame', 'EmberUI_ZoneNotif', nil)
banner:SetSize(FRAME_W, FRAME_H); banner:SetPoint('TOPLEFT', fx, 250); banner:Hide()
banner:SetFrameLevel(90)   -- above the HUD, below popups/loading

local art = banner:CreateTexture(); art:SetTexture('zone_notifaction.png'); art:SetPoint('TOPLEFT', 0, 0)
local title   = banner:CreateFontString(); title:SetFont('Ringbearer'); title:SetFontSize(36)
local caption = banner:CreateFontString(); caption:SetFont('Palatino');  caption:SetFontSize(18)

local alpha, mode, solid = 0, 'idle', 0
local capR, capG, capB = 255, 255, 0

local function applyAlpha()
	local a = math.floor(alpha * 255 + 0.5)
	art:SetVertexColor(255, 255, 255, a)
	title:SetTextColor(232, 205, 182, a)
	caption:SetTextColor(capR, capG, capB, a)
end

local function split(s)
	local t = {}
	for part in string.gmatch(s, '([^|]*)') do t[#t + 1] = part end
	return t
end

local d = CreateFrame('Frame', nil, nil)
d:SetScript('OnEvent', function(_, event, arg)
	if event == Events.ZONE_CHANGED then
		local p = split(arg)
		title:SetText(p[1] ~= '' and p[1] or 'Unknown Zone'); title:SetPoint('CENTER', 0, -34)
		caption:SetText(p[2] or '');                          caption:SetPoint('CENTER', 0, 16)
		capR = tonumber(p[3]) or 255; capG = tonumber(p[4]) or 255; capB = tonumber(p[5]) or 0
		alpha = 0; mode = 'in'; applyAlpha(); banner:Show()
	else
		mode = 'idle'; banner:Hide()   -- leaving world: clear any in-flight banner
	end
end)
d:RegisterEvent(Events.ZONE_CHANGED)
d:RegisterEvent(Events.LOGIN_SHOWN)
d:RegisterEvent(Events.CHARSELECT_SHOWN)

d:SetScript('OnUpdate', function(_, dt)
	if mode == 'in' then
		alpha = alpha + dt * 2.0          -- ~0.5s fade in
		if alpha >= 1 then alpha = 1; mode = 'solid'; solid = 3.0 end
		applyAlpha()
	elseif mode == 'solid' then
		solid = solid - dt
		if solid <= 0 then mode = 'out' end
	elseif mode == 'out' then
		alpha = alpha - dt * 1.0          -- ~1s fade out
		if alpha <= 0 then alpha = 0; mode = 'idle'; banner:Hide() end
		applyAlpha()
	end
end)

print('EmberUI zone notification loaded')
