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
--
-- Seam handling: the 1px border line is baked into the edge/corner art. uiScale rounds a region's POSITION
-- (ui_up) but not its SIZE, so an edge that merely abuts a corner drifts +/-1px at the shared seam -- the
-- line then doubles ("crossing") or gaps ("missing pixels") depending on the window's exact dimensions.
-- Fix: draw center + each edge at FULL span so an edge is one continuous stretched sprite with no interior
-- seam, then lay the opaque corners on top LAST to cap the ends crisply. No abutting seam => no drift.
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
	-- center, then the four edges spanning the full width/height (running UNDER the corner zones)...
	region(S.c, k,     k,     cw, ch)
	region(S.t, 0,     0,     w,  k );  region(S.b, 0, h - k, w, k )   -- top/bottom edges, full width
	region(S.l, 0,     0,     k,  h );  region(S.r, w - k, 0,  k, h )  -- left/right edges, full height
	-- ...then the opaque corners on top, capping each edge end with the crisp L-junction.
	region(S.tl, 0,     0,     k,  k );  region(S.tr, w - k, 0,     k, k)
	region(S.bl, 0,     h - k, k,  k );  region(S.br, w - k, h - k, k, k)
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

-- ---------------------------------------------------------------------------------------------------
-- EmberUI.Layout -- lightweight, WoW-style anchor layout for the Lua UI. Positions resolve once at build
-- time via SetPoint; because the anchors are PARENT-RELATIVE the engine still re-solves them every frame,
-- so children track a moved/resized window for free. There is no per-frame relayout pass and no immediate-
-- mode rebuild -- this stays inside the retained, event-driven model. It replaces hand-computed pixel
-- offsets (the source of the seam / overlap / off-center bugs) with named row/column/grid math in ONE place.
-- ---------------------------------------------------------------------------------------------------
EmberUI.Layout = {}

-- Accept either a raw frame handle (userdata) or a wrapper table exposing `.frame` (CreateItemButton, etc.).
local function frameOf(region)
	if type(region) == 'table' then return region.frame or region end
	return region
end

-- Grid(opts) -> function(i) -> x, y : the top-left of the i-th cell (1-based, row-major) in a `cols`-wide
-- grid. opts: cols (required); size|pitch (cell pitch); x, y (origin, default 0,0); pitchY (default pitch).
-- Kills the duplicated `(col*pitch, row*pitch)` math in Inventory / Equipment / quest-reward grids.
function EmberUI.Layout.Grid(opts)
	local cols   = opts.cols
	local pitch  = opts.pitch or opts.size or 0
	local pitchY = opts.pitchY or pitch
	local ox, oy = opts.x or 0, opts.y or 0
	return function(i)
		local c = (i - 1) % cols
		local r = math.floor((i - 1) / cols)
		return ox + c * pitch, oy + r * pitchY
	end
end

-- Stack(parent, opts) -> stack : anchor regions one after another along an axis, advancing by each item's
-- extent + gap. dir = 'y' (default, top-to-bottom) or 'x' (left-to-right). opts: x, y (origin); gap; step
-- (default extent when :Add omits one). Returns { Add(region[, extent]) -> region, Reset([x,y]),
-- Cursor() -> x,y }. Removes per-row `Y0 + (i-1)*ROWH` offsets (quest list, stat rows, prog labels).
function EmberUI.Layout.Stack(parent, opts)
	opts = opts or {}
	local horiz = opts.dir == 'x'
	local x0, y0 = opts.x or 0, opts.y or 0
	local gap, step = opts.gap or 0, opts.step or 0
	local st = { _x = x0, _y = y0 }
	function st.Add(region, extent)
		frameOf(region):SetPoint('TOPLEFT', parent, 'TOPLEFT', st._x, st._y)
		local adv = (extent or step) + gap
		if horiz then st._x = st._x + adv else st._y = st._y + adv end
		return region
	end
	function st.Reset(x, y) st._x = x or x0; st._y = y or y0 end
	function st.Cursor() return st._x, st._y end
	return st
end

-- Row / Column: Stack presets for the horizontal / vertical axes.
function EmberUI.Layout.Row(parent, opts)    opts = opts or {}; opts.dir = 'x'; return EmberUI.Layout.Stack(parent, opts) end
function EmberUI.Layout.Column(parent, opts) opts = opts or {}; opts.dir = 'y'; return EmberUI.Layout.Stack(parent, opts) end

-- CenterText(fs, parent[, dx, dy]): center a FontString in `parent` via the engine's width-aware CENTER
-- anchor, so callers stop hand-picking an x that only lines up for one string length (the off-center
-- zone-title class of bug). dx/dy nudge from the center.
function EmberUI.Layout.CenterText(fs, parent, dx, dy)
	fs:SetPoint('CENTER', parent, 'CENTER', dx or 0, dy or 0)
	return fs
end

-- ---------------------------------------------------------------------------------------------------
-- Layout registry -- the movable-element model behind the LOTRO-style layout editor (LayoutEditor.lua).
-- Each editable HUD element registers once with a stable id; its screen ANCHOR (one of the 9 points) +
-- (x,y) offset persist account-wide via SaveUISetting (keys lay_<id>_a/_x/_y, plus _det for a detachable
-- cast bar). Modules call Place(id) wherever they used to hardcode a root SetPoint (including their
-- WORLD_SHOWN re-anchor), so a saved layout and live editor edits both win over the old baked coords.
-- ---------------------------------------------------------------------------------------------------

-- 9 anchor points, index 0..8 (matches the engine's pointToInt ordering used by SetPoint).
EmberUI.Layout.ANCHORS = { 'TOPLEFT', 'TOP', 'TOPRIGHT', 'LEFT', 'CENTER', 'RIGHT', 'BOTTOMLEFT', 'BOTTOM', 'BOTTOMRIGHT' }
local ANCHORS = EmberUI.Layout.ANCHORS
local ANCHOR_IDX = {}
for i, n in ipairs(ANCHORS) do ANCHOR_IDX[n] = i - 1 end   -- name -> 0-based index
EmberUI.Layout.ANCHOR_IDX = ANCHOR_IDX

EmberUI.Layout._registry = {}
EmberUI.Layout._order = {}   -- registration order, so the editor iterates deterministically

-- Where an anchor point sits within a box of size (w,h): TL={0,0}, CENTER={w/2,h/2}, BR={w,h}.
local function anchorWithin(name, w, h)
	local ax, ay = 0, 0
	if name == 'TOP' or name == 'CENTER' or name == 'BOTTOM' then ax = w / 2
	elseif name == 'TOPRIGHT' or name == 'RIGHT' or name == 'BOTTOMRIGHT' then ax = w end
	if name == 'LEFT' or name == 'CENTER' or name == 'RIGHT' then ay = h / 2
	elseif name == 'BOTTOMLEFT' or name == 'BOTTOM' or name == 'BOTTOMRIGHT' then ay = h end
	return ax, ay
end
EmberUI.Layout.AnchorWithin = anchorWithin

-- Apply an element's current state to the engine. Screen-relative (3-arg SetPoint) unless a relTo frame is
-- set AND the element is not detached -- i.e. a cast bar attached to its unit frame, which then tracks it.
function EmberUI.Layout.Place(id)
	local e = EmberUI.Layout._registry[id]
	if not e then return end
	e.frame:ClearAllPoints()
	if e.relTo and not e.det then
		e.frame:SetPoint(e.anchor, e.relTo, e.anchor, e.x, e.y)   -- attached: same point on the relTo frame
	else
		e.frame:SetPoint(e.anchor, e.x, e.y)                      -- screen-relative
	end
end

-- Register (or re-register) an editable element. opts = { label, anchor, x, y, relTo?, detachable? }.
-- Reads any saved anchor/offset (falls back to opts), records the defaults for Reset, then Places.
function EmberUI.Layout.Register(id, frame, opts)
	local defA = ANCHOR_IDX[opts.anchor or 'TOPLEFT'] or 0
	local defX, defY = math.floor(opts.x or 0), math.floor(opts.y or 0)
	local a = GetUISetting('lay_' .. id .. '_a', defA)
	if a < 0 or a > 8 then a = defA end
	local e = {
		id = id, frame = frame, label = opts.label or id,
		anchor = ANCHORS[a + 1] or 'TOPLEFT',
		x = GetUISetting('lay_' .. id .. '_x', defX),
		y = GetUISetting('lay_' .. id .. '_y', defY),
		defA = defA, defX = defX, defY = defY,
		relTo = opts.relTo, detachable = opts.detachable or false,
		det = opts.detachable and (GetUISetting('lay_' .. id .. '_det', 0) ~= 0) or false,
	}
	if not EmberUI.Layout._registry[id] then EmberUI.Layout._order[#EmberUI.Layout._order + 1] = id end
	EmberUI.Layout._registry[id] = e
	EmberUI.Layout.Place(id)
	return e
end

-- Persist + apply. `anchor` may be a name or a 0-based index; `det` (optional) only matters for detachable ids.
function EmberUI.Layout.Set(id, anchor, x, y, det)
	local e = EmberUI.Layout._registry[id]
	if not e then return end
	if type(anchor) == 'number' then anchor = ANCHORS[anchor + 1] end
	if anchor then e.anchor = anchor end
	if x then e.x = math.floor(x) end                 -- 0 is truthy in Lua, so x=0 still applies
	if y then e.y = math.floor(y) end
	if det ~= nil and e.detachable then e.det = det and true or false end
	SaveUISetting('lay_' .. id .. '_a', ANCHOR_IDX[e.anchor] or 0)
	SaveUISetting('lay_' .. id .. '_x', e.x)
	SaveUISetting('lay_' .. id .. '_y', e.y)
	if e.detachable then SaveUISetting('lay_' .. id .. '_det', e.det and 1 or 0) end
	EmberUI.Layout.Place(id)
end

-- The raw registry entry for an id (for the editor to read anchor/x/y/det/label/frame). Do not mutate directly.
function EmberUI.Layout.Get(id) return EmberUI.Layout._registry[id] end

-- Set offset + apply WITHOUT persisting -- for live editor drag (Set() persists on drop).
function EmberUI.Layout.Move(id, x, y)
	local e = EmberUI.Layout._registry[id]
	if not e then return end
	e.x = math.floor(x); e.y = math.floor(y)
	EmberUI.Layout.Place(id)
end

-- Current on-screen top-left of a registered element (design px), for children that lay out off it.
function EmberUI.Layout.Resolve(id)
	local e = EmberUI.Layout._registry[id]
	if not e then return 0, 0 end
	return e.frame:GetLeft(), e.frame:GetTop()
end

-- Reference box (left,top,w,h) an element is anchored against: the relTo frame while attached, else the screen.
local function referenceBox(e)
	if e.relTo and not e.det then
		return e.relTo:GetLeft(), e.relTo:GetTop(), e.relTo:GetWidth(), e.relTo:GetHeight()
	end
	return 0, 0, GetScreenWidth(), GetScreenHeight()
end

-- Recompute (x,y) so the element does NOT visually move when its anchor (or attach state) changes -- LOTRO
-- keep-in-place. Offset = (element's anchor point) - (reference's matching anchor point), in screen space.
local function rebase(e, newAnchor, newDet)
	local left, top = e.frame:GetLeft(), e.frame:GetTop()
	local w, h = e.frame:GetWidth(), e.frame:GetHeight()
	local ex, ey = anchorWithin(newAnchor, w, h)
	local savedDet = e.det
	if newDet ~= nil then e.det = newDet end            -- referenceBox reads e.det
	local rleft, rtop, rw, rh = referenceBox(e)
	e.det = savedDet
	local rx, ry = anchorWithin(newAnchor, rw, rh)
	return (left + ex) - (rleft + rx), (top + ey) - (rtop + ry)
end

-- Change anchor, keeping the element in place. `newAnchor` = name or index.
function EmberUI.Layout.RebaseAnchor(id, newAnchor)
	local e = EmberUI.Layout._registry[id]
	if not e then return end
	if type(newAnchor) == 'number' then newAnchor = ANCHORS[newAnchor + 1] end
	local nx, ny = rebase(e, newAnchor, nil)
	EmberUI.Layout.Set(id, newAnchor, nx, ny)
end

-- Detach / re-attach a detachable element (cast bar), keeping it in place across the reference swap.
function EmberUI.Layout.SetDetached(id, det)
	local e = EmberUI.Layout._registry[id]
	if not e or not e.detachable then return end
	det = det and true or false
	local nx, ny = rebase(e, e.anchor, det)
	EmberUI.Layout.Set(id, e.anchor, nx, ny, det)
end

-- Persist the element's CURRENT on-screen position as an offset under its current anchor. Used after a raw
-- engine drag (unit-frame Lock/Unlock) so the drop is captured without discarding the chosen anchor.
function EmberUI.Layout.CapturePosition(id)
	local e = EmberUI.Layout._registry[id]
	if not e then return end
	local nx, ny = rebase(e, e.anchor, nil)
	EmberUI.Layout.Set(id, e.anchor, nx, ny)
end

-- Restore an element to its registered defaults (also re-attaches a detachable one).
function EmberUI.Layout.Reset(id)
	local e = EmberUI.Layout._registry[id]
	if not e then return end
	EmberUI.Layout.Set(id, e.defA, e.defX, e.defY, false)
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
