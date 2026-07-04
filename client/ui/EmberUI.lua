-- Emberfire default UI — core namespace + binding helpers.
-- This is the built-in "FrameXML": it loads before user addons and is fully overridable (its frames are
-- globals an addon can Hide()). Lua = visualization only; all game data/logic comes from the C++ API
-- (getters like UnitHealth, events in the Events table, commands like TargetUnit).

EmberUI = EmberUI or {}

-- True only while the player is in the world (set by the stage events). The HUD modules gate their
-- visibility on this, since the frame manager is now persistent across the login/character screens.
EmberUI.inWorld = false

-- Shared text palette. Single source of truth for parchment-panel text colors so contrast stays consistent
-- and future re-tuning is one edit. The old inline values sat in the ~95-150 luminance band (too dark on the
-- tan/parchment art); these are lifted to ~180-215 while keeping the warm hue. Sites reference by role:
--   Title  = panel headers / item & character names        Body   = long description / body copy
--   Detail = stat values, bar % / secondary detail text    Muted  = subtitles, inactive tabs, hints
EmberUI.Color = {
	Title  = { 214, 192, 150 },
	Body   = { 206, 186, 160 },
	Detail = { 200, 182, 150 },
	Muted  = { 180, 164, 140 },
	Gold   = { 255, 215, 0 },
	White  = { 255, 255, 255 },
	Warn   = { 230, 90, 90 },
}

-- SetColor(fontString, roleTable[, alpha]): apply an EmberUI.Color entry to a FontString (alpha default 255).
function EmberUI.SetColor(fs, c, a)
	fs:SetTextColor(c[1], c[2], c[3], a or 255)
end

-- ---------------------------------------------------------------------------------------------------
-- 9-slice window chrome. Replaces per-panel baked background PNGs (equipment.png, bank.png, ...) with a
-- shared, resolution-independent frame built from tiny slices (content-ember/window_*.png): the border
-- stays crisp at any UI scale, only the interior stretches. Mirrors the C++ ExpandableWindow slice math
-- (corners 1:1, edges stretched on one axis, center on both) but composed from Lua CreateTexture regions
-- since LuaTexture:SetSize already stretches a sprite. See EmberUI.CreateWindow.
-- ---------------------------------------------------------------------------------------------------

-- Slice asset set + corner size (design pixels). Corner art is square; edges tile along their long axis.
-- Loaded via the content-ember overlay (loose PNGs), so these names resolve without a content-zip repack.
EmberUI.Slice = {
	corner = 20,
	tl = 'window_tl.png', t = 'window_t.png', tr = 'window_tr.png',
	l  = 'window_l.png',  c = 'window_c.png', r  = 'window_r.png',
	bl = 'window_bl.png', b = 'window_b.png', br = 'window_br.png',
	divider   = 'window_divider.png',
	closeIdle = 'window_close_idle.png', closeHover = 'window_close_hover.png',
}

-- Slot-cell skin for item buttons. The baked panel PNGs used to draw the empty-cell frames behind the item
-- grids; on the plain 9-slice frames that art is gone, so CreateItemButton paints this recessed cell instead.
-- (LuaButton draws its base texture at natural size, so this is authored at the common 40px slot size.)
EmberUI._slotBg = 'slot40.png'

-- Build the 9-slice backdrop as children of `frame` (w x h design pixels). Created first so it draws
-- behind later content. All regions are TOPLEFT-anchored offsets from the frame.
local function buildBackdrop(frame, w, h)
	local S = EmberUI.Slice
	local k = S.corner
	local cw, ch = w - 2 * k, h - 2 * k
	local function region(tex, x, y, rw, rh)
		local t = frame:CreateTexture()
		t:SetTexture(tex)
		t:SetPoint('TOPLEFT', frame, 'TOPLEFT', x, y)
		t:SetSize(rw, rh)
		return t
	end
	region(S.tl, 0,     0,     k,  k );  region(S.t, k,     0,     cw, k );  region(S.tr, w - k, 0,     k,  k )
	region(S.l,  0,     k,     k,  ch);  region(S.c, k,     k,     cw, ch);  region(S.r,  w - k, k,     k,  ch)
	region(S.bl, 0,     h - k, k,  k );  region(S.b, k,     h - k, cw, k );  region(S.br, w - k, h - k, k,  k )
end

-- CreateWindow(opts) -> window table. opts:
--   name?    global frame name              parent?    parent frame (default top-level)
--   width, height   (design pixels, required)
--   title?   header text                    titleFont? (default 'Ringbearer')   titleSize? (default 16)
--   closable?  add a top-right close button  onClose?  fn (default: hide the frame)
--   movable?   drag the frame (default true) inset?    content padding from the edge (default corner size)
-- Returns { frame, content, title?, close?, SetTitle(str), AddDivider(y[,x,len]), Show(), Hide() }.
-- `frame` is the root (parent your widgets here, or anchor against `content` for the inset region).
function EmberUI.CreateWindow(opts)
	local w, h = opts.width, opts.height
	local root = CreateFrame('Frame', opts.name, opts.parent)
	root:SetSize(w, h)
	if opts.movable ~= false then root:SetMovable(true); root:RegisterForDrag('LeftButton') end

	buildBackdrop(root, w, h)

	local inset = opts.inset or EmberUI.Slice.corner
	local content = CreateFrame('Frame', nil, root)
	content:SetPoint('TOPLEFT', root, 'TOPLEFT', inset, inset)
	content:SetSize(w - 2 * inset, h - 2 * inset)

	local win = { frame = root, content = content }

	if opts.title then
		local fs = root:CreateFontString()
		fs:SetFont(opts.titleFont or 'Ringbearer'); fs:SetFontSize(opts.titleSize or 16)
		EmberUI.SetColor(fs, EmberUI.Color.Title)
		fs:SetPoint('TOPLEFT', root, 'TOPLEFT', inset + 2, 6)
		fs:SetText(opts.title)
		win.title = fs
	end

	function win.SetTitle(str) if win.title then win.title:SetText(str) end end

	if opts.closable then
		local btn = CreateFrame('Button', nil, root)
		btn:SetTexture(EmberUI.Slice.closeIdle); btn:SetHoverTexture(EmberUI.Slice.closeHover)
		btn:SetSize(20, 20)
		btn:SetPoint('TOPLEFT', root, 'TOPLEFT', w - 24, 6)
		btn:EnableMouse(true)
		btn:SetScript('OnClick', function()
			if opts.onClose then opts.onClose() else root:Hide() end
		end)
		win.close = btn
	end

	-- AddDivider(y[, x, len]): a 2px horizontal rule at design-y from the frame top; default full inner width.
	function win.AddDivider(y, x, len)
		local d = root:CreateTexture()
		d:SetTexture(EmberUI.Slice.divider)
		d:SetPoint('TOPLEFT', root, 'TOPLEFT', x or inset, y)
		d:SetSize(len or (w - 2 * inset), 2)
		return d
	end

	function win.Show() root:Show() end
	function win.Hide() root:Hide() end

	return win
end

-- bind(widget, setterName, getterFn, token, eventName): call widget:<setter>(getter(token)) on the event + now.
function EmberUI.bind(widget, setter, getter, token, event)
	local d = CreateFrame('Frame', nil, nil)
	local function apply() widget[setter](widget, getter(token)) end
	d:SetScript('OnEvent', apply)
	d:RegisterEvent(event)
	apply()
	return d
end

-- bindBar(bar, getter, maxGetter, token, valueEvent[, maxEvent]): drive a StatusBar; event-driven + 0.5s poll
-- (poll catches max-value changes that don't fire an event).
function EmberUI.bindBar(bar, getter, maxGetter, token, valueEvent, maxEvent)
	local function refresh()
		local mx = maxGetter(token); if mx <= 0 then mx = 1 end
		bar:SetMinMaxValues(0, mx); bar:SetValue(getter(token))
	end
	local d = CreateFrame('Frame', nil, nil)
	d:SetScript('OnEvent', refresh)
	d:RegisterEvent(valueEvent); if maxEvent then d:RegisterEvent(maxEvent) end
	local acc = 0
	d:SetScript('OnUpdate', function(_, dt) acc = acc + dt; if acc >= 0.5 then acc = 0; refresh() end end)
	refresh()
	return d
end

-- CreateItemButton(parent, index, size): an item slot composed from primitives (no native widget needed) —
-- optional slot-frame bg (EmberUI._slotBg), an icon Texture, a stack-count FontString, and a Cooldown overlay.
-- Returns { frame, icon, count, cd, index, SetItem(entry, stack), Clear() }. Caller wires scripts on .frame.
function EmberUI.CreateItemButton(parent, index, size)
	size = size or 40
	local b = CreateFrame('Button', nil, parent)
	b:SetSize(size, size)
	if EmberUI._slotBg then b:SetTexture(EmberUI._slotBg) end
	b:SetHoverTexture(EmberUI._slotHover or 'gameicon40_hover.png')   -- reddish edge highlight on mouseover

	local icon = b:CreateTexture()
	icon:SetPoint('TOPLEFT', b, 'TOPLEFT', 2, 2)
	icon:SetSize(size - 4, size - 4)

	local cd = CreateFrame('Cooldown', nil, b)
	cd:SetPoint('TOPLEFT', b, 'TOPLEFT', 2, 2)
	cd:SetSize(size - 4, size - 4)

	local count = b:CreateFontString()
	count:SetFont('Arial'); count:SetFontSize(12)
	count:SetPoint('BOTTOMRIGHT', b, 'BOTTOMRIGHT', -3, -2)

	local obj = { frame = b, icon = icon, count = count, cd = cd, index = index }
	function obj.SetItem(entry, stack)
		if not entry or entry == 0 then obj.Clear(); return end
		local _, tex = GetItemInfo(entry)
		if tex and tex ~= '' then icon:SetTexture(tex) end
		icon:Show()
		if stack and stack > 1 then count:SetText(tostring(stack)); count:Show()
		else count:SetText(''); count:Hide() end
	end
	function obj.Clear()
		icon:Hide(); count:SetText(''); count:Hide()
	end
	obj.Clear()
	return obj
end

-- Cursor item: a reusable "held on the cursor" icon + grey-out of the source slot, for drag-pickup across
-- windows (inventory now; equipment/spellbook/actionbar as they migrate). Call EmberUI.PickupItem on drag
-- start and EmberUI.ClearCursor on drop/cancel; EmberUI.HasCursorItem() reports whether something is held.
EmberUI._held = nil

local function ensureCursor()
	if EmberUI.cursor then return EmberUI.cursor end
	local c = CreateFrame('Frame', 'EmberCursorItem', nil)
	c:SetSize(36, 36)
	c:SetFrameLevel(10000)   -- above everything
	local tex = c:CreateTexture()
	tex:SetAllPoints(c)
	c:Hide()
	c:SetScript('OnUpdate', function()
		if not GetCursorPosition then return end
		local x, y = GetCursorPosition()
		c:SetPoint('TOPLEFT', x - 18, y - 18)   -- center the icon on the cursor
	end)
	-- a frame handle is userdata (not a Lua table) -- store the frame + region in a plain table wrapper.
	EmberUI.cursor = { frame = c, tex = tex }
	return EmberUI.cursor
end

-- iconTexture: the item's icon to ride the cursor; sourceIcon: the source slot's icon Texture to grey;
-- payload: a table describing what is held (e.g. {from='bag', slot=i} / {from='equip', slot=e}) so a DIFFERENT
-- window's slots can react to a cross-window drop (bag <-> equipment).
function EmberUI.PickupItem(iconTexture, sourceIcon, payload)
	if GetCursorPosition and iconTexture and iconTexture ~= '' then
		local c = ensureCursor()
		c.tex:SetTexture(iconTexture); c.tex:Show(); c.frame:Show()
	end
	if sourceIcon then sourceIcon:SetVertexColor(80, 80, 80, 255) end   -- grey overlay on the picked-up item
	EmberUI._held = { source = sourceIcon, payload = payload }
end

function EmberUI.HeldPayload() return EmberUI._held and EmberUI._held.payload end

function EmberUI.ClearCursor()
	if EmberUI.cursor then EmberUI.cursor.frame:Hide() end
	if EmberUI._held and EmberUI._held.source then EmberUI._held.source:SetVertexColor(255, 255, 255, 255) end
	EmberUI._held = nil
end

function EmberUI.HasCursorItem() return EmberUI._held ~= nil end

-- Reusable right-click context menu. items = { { text=, color={r,g,b} (optional), onClick=fn }, ... }.
-- Pops at the cursor; a row click runs onClick + hides; include a "Cancel" row to dismiss. Reused by the
-- guild roster (and future windows that need a right-click menu).
function EmberUI.ShowMenu(items)
	local m = EmberUI._menu
	if not m then
		local f = CreateFrame('Frame', 'EmberContextMenu', nil)
		f:SetFrameLevel(9000)
		local bg = f:CreateTexture(); bg:SetAllPoints(f); bg:SetTexture('game_chat_backdrop.png')
		m = { frame = f, bg = bg, rows = {} }
		EmberUI._menu = m
	end
	local MW, RH, PAD = 132, 19, 5
	local n = #items
	m.frame:SetSize(MW, n * RH + PAD * 2)
	local x, y = GetCursorPosition()
	m.frame:SetPoint('TOPLEFT', x, y)
	for i = 1, n do
		local r = m.rows[i]
		if not r then
			local btn = CreateFrame('Button', nil, m.frame)
			btn:SetSize(MW - PAD * 2, RH); btn:EnableMouse(true)
			-- subtle warm row highlight (no slot-outline art); pcall so an older exe (no SetHoverColor
			-- binding yet) degrades to the texture highlight instead of aborting the menu build.
			if not pcall(function() btn:SetHoverColor(255, 235, 180, 38) end) then
				btn:SetHoverTexture('gameicon40_hover.png')
			end
			local fs = m.frame:CreateFontString(); fs:SetFont('Arial'); fs:SetFontSize(13)
			r = { btn = btn, fs = fs }; m.rows[i] = r
		end
		r.btn:SetPoint('TOPLEFT', m.frame, 'TOPLEFT', PAD, PAD + (i - 1) * RH)
		r.fs:SetPoint('TOPLEFT', m.frame, 'TOPLEFT', PAD + 3, PAD + (i - 1) * RH + 2)
		r.fs:SetText(items[i].text)
		local c = items[i].color
		r.fs:SetTextColor(c and c[1] or 235, c and c[2] or 225, c and c[3] or 200, 255)
		r.fs:Show()
		local cb = items[i].onClick
		r.btn:SetScript('OnClick', function() m.frame:Hide(); if cb then cb() end end)
		r.btn:Show()
	end
	for i = n + 1, #m.rows do m.rows[i].btn:Hide(); m.rows[i].fs:Hide() end
	m.frame:Show()
end

function EmberUI.HideMenu() if EmberUI._menu then EmberUI._menu.frame:Hide() end end

-- Reusable quest-rewards block (QuestOffer + QuestComplete): xp/money text + an optional CHOICE item row +
-- a mandatory item row. Mirrors the C++ QuestRewards (slots from quest_template; class-unusable slots are
-- skipped). choosing=true makes the choice row clickable; GetChoice() returns the chosen ORIGINAL slot
-- (1-based, what CompleteQuest expects) or nil.
function EmberUI.CreateQuestRewards(parent, x, y)
	local obj = { choice = nil, slots = {} }

	local text = parent:CreateFontString()
	text:SetFont('Palatino'); text:SetFontSize(14); EmberUI.SetColor(text, EmberUI.Color.Body)
	text:SetPoint('TOPLEFT', parent, 'TOPLEFT', x, y); text:SetWidth(220)

	-- two rows of 4 item buttons: row 1 = choices, row 2 = mandatory
	local rows = {}
	for r = 1, 2 do
		rows[r] = {}
		for i = 1, 4 do
			local s = EmberUI.CreateItemButton(parent, i, 40)
			s.frame:SetPoint('TOPLEFT', parent, 'TOPLEFT', x + 135 + (i - 1) * 42, y + (r - 1) * 44 - 4)
			s.frame:EnableMouse(true)
			s.frame:Hide()
			rows[r][i] = s
		end
	end

	function obj.SetQuest(questId, choosing)
		obj.choice = nil
		obj.choosing = choosing
		local xp, money = GetQuestRewardInfo(questId)
		local str = ''
		if xp and xp > 0 then str = str .. xp .. 'xp\n' end
		if money and money > 0 then str = str .. 'Gold: ' .. money .. '\n' end
		if str == '' then str = 'No basic reward.' end
		text:SetText(str); text:Show()

		-- gather usable slots (skip class-unusable, like the C++)
		local function fill(rowIdx, isChoice)
			local shown = 0
			for slot = 1, 4 do
				local itemId, count, usable = GetQuestRewardItem(questId, isChoice, slot)
				if itemId ~= 0 and usable then
					shown = shown + 1
					local btn = rows[rowIdx][shown]
					btn.SetItem(itemId, count)
					btn.frame:SetTooltipItemEntry(itemId)
					btn.frame:Show(); btn.icon:Show()
					btn._slot = slot
					if isChoice and choosing then
						btn.icon:SetVertexColor(180, 180, 180, 255)   -- unchosen dim
						btn.frame:SetScript('OnClick', function()
							obj.choice = btn._slot
							for i = 1, 4 do rows[1][i].icon:SetVertexColor(180, 180, 180, 255) end
							btn.icon:SetVertexColor(255, 255, 255, 255)
						end)
					else
						btn.icon:SetVertexColor(255, 255, 255, 255)
						btn.frame:SetScript('OnClick', function() end)
					end
				end
			end
			for i = shown + 1, 4 do
				rows[rowIdx][i].frame:Hide(); rows[rowIdx][i].Clear()
				rows[rowIdx][i].frame:SetTooltipItemEntry(0)
			end
			return shown
		end
		fill(1, true)
		fill(2, false)
	end

	function obj.GetChoice() return obj.choice end
	function obj.Hide()
		text:Hide()
		for r = 1, 2 do for i = 1, 4 do rows[r][i].frame:Hide(); rows[r][i].Clear(); rows[r][i].frame:SetTooltipItemEntry(0) end end
	end

	return obj
end

print('EmberUI core loaded')
