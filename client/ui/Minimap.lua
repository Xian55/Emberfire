-- Lua minimap chrome: the frame art, zone/channel labels and the two buttons move here; the GPU map
-- composite (render texture + circular mask + dots) STAYS in the C++ Minimap (flipped to luaView by
-- SetHudLuaView -- it keeps drawing the map circle at its own corner + (3,45), so this frame must sit at
-- the same screen spot: (sw-260, 15), from World::updateGuiPositions). The "Map" keybind also stays C++.
-- Minimap button click = the channel menu (ChangeChatChannel); gold pouch = RecoverMailLoot, visibility
-- polled (no event exists). Labels poll at 1Hz (zone/channel setters are C++-internal).

local root = CreateFrame('Frame', 'EmberMinimap', nil)
root:Hide()

local frameArt = root:CreateTexture()
frameArt:SetTexture('minimap.png')
local fw, fh = GetTextureSize('minimap.png')
if fw <= 0 then fw, fh = 256, 330 end
frameArt:SetSize(fw, fh)
frameArt:SetPoint('TOPLEFT', root, 'TOPLEFT', 0, 0)
root:SetSize(fw, fh)

local zone = root:CreateFontString()
zone:SetFont('Ringbearer'); zone:SetFontSize(18); zone:SetTextColor(201, 179, 149, 255)
local channel = root:CreateFontString()
channel:SetFont('Ringbearer'); channel:SetFontSize(18); channel:SetTextColor(201, 179, 149, 255)

-- centered labels: no text-width metric in Lua, so anchor near-center with a rough half-width offset
local function centerLabel(fs, text, y)
	fs:ClearAllPoints()
	fs:SetPoint('TOPLEFT', root, 'TOPLEFT', math.floor(fw / 2) - math.floor(#text * 9 / 2), y)
	fs:SetText(text)
end

local mapBtn = CreateFrame('Button', nil, root)
mapBtn:SetTexture('minimap_button_idle.png'); mapBtn:SetHoverTexture('minimap_button_hover.png')
local mw, mh = GetTextureSize('minimap_button_idle.png')
mapBtn:SetSize(mw > 0 and mw or 32, mh > 0 and mh or 32)
mapBtn:SetPoint('TOPLEFT', root, 'TOPLEFT', 101, 254)
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
pouch:SetPoint('TOPLEFT', root, 'TOPLEFT', -5, 240)
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
