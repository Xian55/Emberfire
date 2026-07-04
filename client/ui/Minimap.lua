-- Lua minimap chrome: the zone/channel labels and the two buttons move here. The frame ART and the GPU
-- map composite (render texture + circular mask + dots) STAY in the C++ Minimap -- the Lua layer renders
-- AFTER the world children, so Lua-drawn frame art would cover the map circle (it must sit under it).
-- This (invisible) root just anchors the labels/buttons at the C++ screen spot: (sw-260, 15). The "Map"
-- keybind also stays C++. Minimap button click = the channel menu (ChangeChatChannel); gold pouch =
-- RecoverMailLoot, visibility polled (no event exists). Labels poll at 1Hz.

local root = CreateFrame('Frame', 'EmberMinimap', nil)
root:Hide()

local fw, fh = GetTextureSize('minimap.png')   -- sized like the art for the centering math, no texture drawn
if fw <= 0 then fw, fh = 256, 330 end
root:SetSize(fw, fh)

local zone = root:CreateFontString()
zone:SetFont('Ringbearer'); zone:SetFontSize(18); zone:SetTextColor(201, 179, 149, 255)
local channel = root:CreateFontString()
channel:SetFont('Ringbearer'); channel:SetFontSize(18); channel:SetTextColor(201, 179, 149, 255)

-- The (invisible, content-less) root frame's corner doesn't resolve to its SetPoint spot, so anchoring the
-- labels/buttons to it left them (and their hit rects) stuck at the screen origin while the C++ map circle
-- drew top-right. Position everything at ABSOLUTE screen coords derived from the same (sw-260,15) origin.
local BASE_X = function() return GetScreenWidth() - 260 end
local BASE_Y = 15

-- centered labels: no text-width metric in Lua, so anchor near-center with a rough half-width offset
local function centerLabel(fs, text, y)
	fs:ClearAllPoints()
	fs:SetPoint('TOPLEFT', BASE_X() + math.floor(fw / 2) - math.floor(#text * 9 / 2), BASE_Y + y)
	fs:SetText(text)
end

local mapBtn = CreateFrame('Button', nil, root)
mapBtn:SetTexture('minimap_button_idle.png'); mapBtn:SetHoverTexture('minimap_button_hover.png')
local mw, mh = GetTextureSize('minimap_button_idle.png')
mapBtn:SetSize(mw > 0 and mw or 32, mh > 0 and mh or 32)
mapBtn:SetPoint('TOPLEFT', BASE_X() + 101, BASE_Y + 254)   -- re-anchored absolutely at WORLD_SHOWN
mapBtn:EnableMouse(true)
mapBtn:SetScript('OnClick', function()
	local n = GetNumChatChannels()
	if n == 0 then return end
	local items = {}
	for i = 1, n do
		local members, cap = GetChatChannelInfo(i)
		items[#items + 1] = {
			text = string.format('Channel #%d (%d/%d)', i, members, cap),
			onClick = function() ChangeChatChannel(i) end,
		}
	end
	items[#items + 1] = { text = 'Cancel' }
	EmberUI.ShowMenu(items)
end)

local pouch = CreateFrame('Button', nil, root)
pouch:SetTexture('gold_pouch_idle.png'); pouch:SetHoverTexture('gold_pouch_hover.png')
local pw, ph = GetTextureSize('gold_pouch_idle.png')
pouch:SetSize(pw > 0 and pw or 32, ph > 0 and ph or 32)
pouch:SetPoint('TOPLEFT', BASE_X() - 5, BASE_Y + 240)
pouch:EnableMouse(true)
pouch:SetScript('OnClick', function() RecoverMailLoot() end)
pouch:Hide()

local inWorld = false
local poll = CreateFrame('Frame', nil, nil)
local acc = 1   -- refresh on the first tick
poll:SetScript('OnUpdate', function(_, dt)
	if not inWorld then return end
	acc = acc + dt
	if acc < 0.5 then return end
	acc = 0
	centerLabel(zone, GetMinimapZone(), 15)
	centerLabel(channel, GetMinimapChannel(), 300)
	if HasMailLoot() then pouch:Show() else pouch:Hide() end
end)

local stage = CreateFrame('Frame', nil, nil)
stage:SetScript('OnEvent', function(_, event)
	if event == Events.WORLD_SHOWN then
		SetHudLuaView(true)   -- minimap chrome + toolbar chrome go headless together
		local sw = GetScreenWidth()
		root:ClearAllPoints()
		root:SetPoint('TOPLEFT', sw - 260, 15)   -- the C++ spot (the map circle is drawn relative to it)
		-- re-anchor the buttons absolutely for the current screen size (labels re-anchor each poll tick)
		mapBtn:ClearAllPoints(); mapBtn:SetPoint('TOPLEFT', (sw - 260) + 101, 15 + 254)
		pouch:ClearAllPoints();  pouch:SetPoint('TOPLEFT', (sw - 260) - 5,  15 + 240)
		inWorld = true
		acc = 1
		root:Show()
	elseif event == Events.ZONE_CHANGED then
		acc = 1   -- refresh the labels next tick
	else
		inWorld = false
		root:Hide()
	end
end)
for _, e in ipairs({ Events.WORLD_SHOWN, Events.ZONE_CHANGED,
                     Events.LOGIN_SHOWN, Events.CHARSELECT_SHOWN, Events.CHARCREATE_SHOWN }) do
	stage:RegisterEvent(e)
end

print('EmberUI minimap loaded')
