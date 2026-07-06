-- Lua HUD chrome: the toolbar_base art + the 6 interface buttons (character/quests/options on the left,
-- social/inventory/spells on the right), replacing the C++ World chrome (render-skipped via SetHudLuaView;
-- the C++ buttons stay input-active so their panel KEYBINDS keep working -- clicks come here). Offsets are
-- the exact World::updateGuiPositions values relative to the centered, bottom-anchored toolbar_base.

local BUTTONS = {
	{ tex = 'toolbar_character', x = 36,  action = 'equipment' },
	{ tex = 'toolbar_quests',    x = 72,  action = 'quests' },
	{ tex = 'toolbar_options',   x = 108, action = 'options' },
	{ tex = 'toolbar_social',    x = 705, action = 'social' },
	{ tex = 'toolbar_inventory', x = 741, action = 'inventory' },
	{ tex = 'toolbar_spells',    x = 777, action = 'spells' },
}
local BTN_Y = 33

local root = CreateFrame('Frame', 'EmberHudChrome', nil)
root:SetFrameLevel(-100)   -- chrome art must sit UNDER the ActionBar buttons (which ride on it)
root:Hide()

local base = root:CreateTexture()
base:SetTexture('toolbar_base.png')
local bw, bh = GetTextureSize('toolbar_base.png')
if bw <= 0 then bw, bh = 980, 94 end
base:SetSize(bw, bh)
base:SetPoint('TOPLEFT', root, 'TOPLEFT', 0, 0)
root:SetSize(bw, bh)

-- The toolbar is one movable editor element. Its root's TOPLEFT is the sprite origin the ActionBar buttons
-- and the 6 interface buttons are all measured from, so moving it moves the whole cluster as one.
EmberUI._hudChrome = root   -- ActionBar.lua anchors its buttons to this
EmberUI.Layout.Register('actionbar', root, { label = 'Action Bar', anchor = 'BOTTOM', x = 0, y = 0 })

for _, def in ipairs(BUTTONS) do
	local b = CreateFrame('Button', nil, root)
	b:SetTexture(def.tex .. '_idle.png'); b:SetHoverTexture(def.tex .. '_hover.png')
	local w, h = GetTextureSize(def.tex .. '_idle.png')
	b:SetSize(w > 0 and w or 32, h > 0 and h or 32)
	b:SetPoint('TOPLEFT', root, 'TOPLEFT', def.x, BTN_Y)
	b:EnableMouse(true)
	b:SetScript('OnClick', function() ToggleHudPanel(def.action) end)
end

local stage = CreateFrame('Frame', nil, nil)
stage:SetScript('OnEvent', function(_, event)
	if event == Events.WORLD_SHOWN then
		SetHudLuaView(true)   -- idempotent (Minimap.lua calls it too)
		EmberUI.Layout.Place('actionbar')   -- saved anchor/offset (editor-movable); default = bottom-center
		root:Show()
	else
		root:Hide()
	end
end)
for _, e in ipairs({ Events.WORLD_SHOWN, Events.LOGIN_SHOWN, Events.CHARSELECT_SHOWN, Events.CHARCREATE_SHOWN }) do
	stage:RegisterEvent(e)
end

print('EmberUI hud chrome loaded')
