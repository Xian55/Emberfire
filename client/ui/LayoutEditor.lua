-- Lua Layout Editor: a LOTRO-style "edit mode" for the HUD. Toggle with /editui (the C++ command fires
-- EDIT_MODE_TOGGLE). Draws a labeled bounding box over every element registered with EmberUI.Layout; drag a
-- box to move the element, or select it and set a screen ANCHOR (one of the 9 points) + X/Y offset in the
-- inspector. Cast bars can be detached from their unit frame. All changes persist via the layout registry
-- (account-wide config). Boxes are polled to the element geometry each frame, so a hidden element (no target,
-- not casting) still shows its box. Nothing here writes game state -- it only moves frames.

local Layout = EmberUI.Layout
EmberUI.LayoutEditor = {}

local BOX_FILL     = { 60, 130, 240, 55 }    -- idle box tint (r,g,b,a)
local BOX_FILL_SEL = { 250, 180, 60, 70 }    -- selected box tint
local BORDER       = { 90, 160, 255, 220 }
local BORDER_SEL   = { 255, 200, 90, 255 }
local GUIDE_DIM    = { 255, 255, 255, 40 }    -- always-on screen-center guides
local GUIDE_SNAP   = { 90, 200, 255, 205 }    -- highlighted guide while an edge/center is snapped
local SNAP = 8                                -- snap distance (design px)

local active = false
local snapping = true
local selected = nil
local boxes = {}          -- id -> { frame, fill, edges = {T,B,L,R}, label }
local built = false
local drag = nil          -- { id, cx, cy, ox, oy } while dragging a box
local insp                -- inspector window (EmberUI.CreateWindow table)
local refreshInspector    -- forward decl (assigned in buildInspector)

local function tint(tex, c) tex:SetVertexColor(c[1], c[2], c[3], c[4] or 255) end

-- ---- box overlay -------------------------------------------------------------------------------

local function selectId(id)
	selected = id
	for bid, b in pairs(boxes) do
		local on = (bid == id)
		tint(b.fill, on and BOX_FILL_SEL or BOX_FILL)
		for _, e in ipairs(b.edges) do tint(e, on and BORDER_SEL or BORDER) end
	end
	if refreshInspector then refreshInspector() end
end

local function buildBox(id)
	local e = Layout.Get(id); if not e then return end
	local box = CreateFrame('Frame', nil, nil)
	box:SetFrameLevel(5000)   -- above the HUD, below the inspector window
	box:EnableMouse(true)
	local fill = box:CreateTexture(); fill:SetTexture('white.png'); fill:SetAllPoints(box); tint(fill, BOX_FILL)
	local edges = {}
	for i = 1, 4 do local ed = box:CreateTexture(); ed:SetTexture('white.png'); tint(ed, BORDER); edges[i] = ed end
	local label = box:CreateFontString(); label:SetFont('Arial'); label:SetFontSize(12)
	label:SetTextColor(255, 255, 255, 255); label:SetPoint('CENTER', box, 'CENTER', 0, 0); label:SetText(e.label)
	box:Hide()

	box:SetScript('OnMouseDown', function(_, btn)
		if btn ~= 'LeftButton' then return end
		selectId(id)
		local cx, cy = GetCursorPosition()
		local en = Layout.Get(id)
		drag = { id = id, cx = cx, cy = cy, ox = en.x, oy = en.y }
	end)
	box:SetScript('OnMouseUp', function(_, btn)
		if btn ~= 'LeftButton' or not drag then return end
		local en = Layout.Get(drag.id)
		Layout.Set(drag.id, nil, en.x, en.y)   -- persist the final offset under the current anchor
		drag = nil
	end)
	boxes[id] = { frame = box, fill = fill, edges = edges, label = label }
end

-- Position a box over its element in screen space (polled each tick, so it works even when the element is
-- hidden -- GetLeft/Top/Width/Height report the layout rect regardless of visibility).
local function placeBox(id)
	local b = boxes[id]; local e = Layout.Get(id); if not b or not e then return end
	local left, top = e.frame:GetLeft(), e.frame:GetTop()
	local w, h = e.frame:GetWidth(), e.frame:GetHeight()
	if w <= 0 then w = 1 end
	if h <= 0 then h = 1 end
	b.frame:ClearAllPoints(); b.frame:SetPoint('TOPLEFT', left, top); b.frame:SetSize(w, h)
	local T, B, L, R = b.edges[1], b.edges[2], b.edges[3], b.edges[4]
	T:ClearAllPoints(); T:SetPoint('TOPLEFT', b.frame, 'TOPLEFT', 0, 0);     T:SetSize(w, 1)
	B:ClearAllPoints(); B:SetPoint('TOPLEFT', b.frame, 'TOPLEFT', 0, h - 1); B:SetSize(w, 1)
	L:ClearAllPoints(); L:SetPoint('TOPLEFT', b.frame, 'TOPLEFT', 0, 0);     L:SetSize(1, h)
	R:ClearAllPoints(); R:SetPoint('TOPLEFT', b.frame, 'TOPLEFT', w - 1, 0); R:SetSize(1, h)
end

-- ---- inspector ---------------------------------------------------------------------------------

-- a small clickable chip (white bg + centered label + OnMouseDown), reused for steppers / detach / reset
local function chip(parent, x, y, w, h, text, onClick)
	local c = CreateFrame('Frame', nil, parent)
	c:SetSize(w, h); c:SetPoint('TOPLEFT', parent, 'TOPLEFT', x, y); c:EnableMouse(true)
	local bg = c:CreateTexture(); bg:SetTexture('white.png'); bg:SetAllPoints(c); bg:SetVertexColor(70, 70, 82, 210)
	local fs = c:CreateFontString(); fs:SetFont('Arial'); fs:SetFontSize(13); fs:SetTextColor(240, 240, 240, 255)
	fs:SetPoint('CENTER', c, 'CENTER', 0, 0); fs:SetText(text)
	c:SetScript('OnMouseDown', function() onClick() end)
	return { frame = c, label = fs }   -- frame handles are userdata; keep refs in a Lua table, never as fields on the handle
end

local cells, selLabel, xVal, yVal, detachChip

local function nudge(dx, dy)
	if not selected then return end
	local e = Layout.Get(selected)
	Layout.Set(selected, nil, e.x + dx, e.y + dy)
	refreshInspector()
end

local function buildInspector()
	insp = EmberUI.CreateWindow{ name = 'EmberLayoutInspector', width = 236, height = 318, title = 'Layout Editor',
		closable = true, onClose = function() EmberUI.LayoutEditor.Toggle() end }
	local root = insp.frame
	root:SetPoint('TOPLEFT', 60, 130)
	root:SetFrameLevel(9000)   -- above the boxes so its controls stay clickable

	selLabel = root:CreateFontString(); selLabel:SetFont('Palatino'); selLabel:SetFontSize(14)
	EmberUI.SetColor(selLabel, EmberUI.Color.Title); selLabel:SetPoint('TOPLEFT', root, 'TOPLEFT', 20, 34)

	-- 3x3 anchor picker (index 0..8 maps row-major to the 9 anchor points)
	local GX, GY, CELL, GAP = 20, 58, 26, 4
	cells = {}
	for i = 0, 8 do
		local r, c = math.floor(i / 3), i % 3
		local cell = CreateFrame('Frame', nil, root)
		cell:SetSize(CELL, CELL)
		cell:SetPoint('TOPLEFT', root, 'TOPLEFT', GX + c * (CELL + GAP), GY + r * (CELL + GAP))
		cell:EnableMouse(true)
		local f = cell:CreateTexture(); f:SetTexture('white.png'); f:SetAllPoints(cell); f:SetVertexColor(80, 80, 92, 200)
		cell:SetScript('OnMouseDown', function() if selected then Layout.RebaseAnchor(selected, i); refreshInspector() end end)
		cells[i] = { frame = cell, fill = f }
	end
	local anchorHint = root:CreateFontString(); anchorHint:SetFont('Arial'); anchorHint:SetFontSize(11)
	EmberUI.SetColor(anchorHint, EmberUI.Color.Muted); anchorHint:SetPoint('TOPLEFT', root, 'TOPLEFT', 116, 62)
	anchorHint:SetText('Anchor')

	-- X / Y steppers
	local function axisRow(letter, y, get, apply)
		local lab = root:CreateFontString(); lab:SetFont('Palatino'); lab:SetFontSize(13)
		EmberUI.SetColor(lab, EmberUI.Color.Detail); lab:SetPoint('TOPLEFT', root, 'TOPLEFT', 20, y + 4); lab:SetText(letter)
		chip(root, 36,  y, 26, 22, '-10', function() apply(-10) end)
		chip(root, 64,  y, 22, 22, '-',   function() apply(-1) end)
		local val = root:CreateFontString(); val:SetFont('Arial'); val:SetFontSize(14); val:SetTextColor(255, 255, 255, 255)
		val:SetPoint('TOPLEFT', root, 'TOPLEFT', 96, y + 4)
		chip(root, 150, y, 22, 22, '+',   function() apply(1) end)
		chip(root, 174, y, 30, 22, '+10', function() apply(10) end)
		return val
	end
	xVal = axisRow('X', 156, function() return selected and Layout.Get(selected).x end, function(d) nudge(d, 0) end)
	yVal = axisRow('Y', 184, function() return selected and Layout.Get(selected).y end, function(d) nudge(0, d) end)

	-- detach (cast bars only) + reset
	detachChip = chip(root, 20, 218, 96, 24, 'Detach', function()
		if not selected then return end
		local e = Layout.Get(selected)
		if e.detachable then Layout.SetDetached(selected, not e.det); refreshInspector() end
	end)
	chip(root, 124, 218, 90, 24, 'Reset', function() if selected then Layout.Reset(selected); refreshInspector() end end)

	-- snap toggle (drag-time edge/center snapping + guide lines)
	local snapChip
	snapChip = chip(root, 20, 248, 110, 22, 'Snap: On', function()
		snapping = not snapping
		snapChip.label:SetText(snapping and 'Snap: On' or 'Snap: Off')
	end)

	local hint = root:CreateFontString(); hint:SetFont('Arial'); hint:SetFontSize(11)
	EmberUI.SetColor(hint, EmberUI.Color.Muted); hint:SetPoint('TOPLEFT', root, 'TOPLEFT', 20, 280)
	hint:SetText('Drag a box to move. /editui to close.')

	refreshInspector = function()
		if not insp then return end
		if not selected then selLabel:SetText('Select an element'); detachChip.frame:Hide(); return end
		local e = Layout.Get(selected)
		selLabel:SetText(e.label)
		xVal:SetText(tostring(e.x)); yVal:SetText(tostring(e.y))
		local cur = Layout.ANCHOR_IDX[e.anchor]
		for i = 0, 8 do
			local on = (cur == i)
			cells[i].fill:SetVertexColor(on and 250 or 80, on and 180 or 80, on and 60 or 92, on and 235 or 200)
		end
		if e.detachable then detachChip.frame:Show(); detachChip.label:SetText(e.det and 'Attach' or 'Detach')
		else detachChip.frame:Hide() end
	end
end

-- ---- guides + snapping -------------------------------------------------------------------------

local guideFrame, vCenter, hCenter, vSnap, hSnap

local function buildGuides()
	guideFrame = CreateFrame('Frame', nil, nil)   -- not mouseable, so guide lines never block box clicks
	guideFrame:SetFrameLevel(8000)                 -- above boxes (5000), below the inspector (9000)
	local function line(col) local t = guideFrame:CreateTexture(); t:SetTexture('white.png'); tint(t, col); return t end
	vCenter, hCenter = line(GUIDE_DIM), line(GUIDE_DIM)
	vSnap, hSnap = line(GUIDE_SNAP), line(GUIDE_SNAP)
end

-- always-on vertical + horizontal screen-center guides
local function layoutCenterGuides()
	local sw, sh = GetScreenWidth(), GetScreenHeight()
	vCenter:ClearAllPoints(); vCenter:SetPoint('TOPLEFT', math.floor(sw / 2), 0); vCenter:SetSize(1, sh); vCenter:Show()
	hCenter:ClearAllPoints(); hCenter:SetPoint('TOPLEFT', 0, math.floor(sh / 2)); hCenter:SetSize(sw, 1); hCenter:Show()
end

-- nearest snap on one axis: match the element's {near edge, center, far edge} to targets {0, mid, size}.
-- Returns (delta to add to the offset, matched target coord for the guide line) or nil,nil.
local function snapAxis(points, targets)
	local bestD, bestDelta, bestTarget = SNAP + 1, nil, nil
	for _, p in ipairs(points) do
		for _, t in ipairs(targets) do
			local d = math.abs(p - t)
			if d <= SNAP and d < bestD then bestD = d; bestDelta = t - p; bestTarget = t end
		end
	end
	return bestDelta, bestTarget
end

local function showSnapGuide(gx, gy)
	if not vSnap then return end
	local sw, sh = GetScreenWidth(), GetScreenHeight()
	if gx then vSnap:ClearAllPoints(); vSnap:SetPoint('TOPLEFT', math.floor(gx), 0); vSnap:SetSize(1, sh); vSnap:Show() else vSnap:Hide() end
	if gy then hSnap:ClearAllPoints(); hSnap:SetPoint('TOPLEFT', 0, math.floor(gy)); hSnap:SetSize(sw, 1); hSnap:Show() else hSnap:Hide() end
end

-- ---- lifecycle ---------------------------------------------------------------------------------

local function buildAll()
	if built then return end
	for _, id in ipairs(Layout._order) do buildBox(id) end
	buildInspector()
	buildGuides()
	built = true   -- only after a FULL successful build, so a partial failure retries instead of half-activating
end

local poll = CreateFrame('Frame', nil, nil)
poll:SetScript('OnUpdate', function()
	if not active then return end
	if drag then
		local cx, cy = GetCursorPosition()
		local rx, ry = drag.ox + (cx - drag.cx), drag.oy + (cy - drag.cy)
		Layout.Move(drag.id, rx, ry)
		local gx, gy
		if snapping then
			local e = Layout.Get(drag.id)
			local left, top = e.frame:GetLeft(), e.frame:GetTop()
			local w, h = e.frame:GetWidth(), e.frame:GetHeight()
			local sw, sh = GetScreenWidth(), GetScreenHeight()
			local dx; dx, gx = snapAxis({ left, left + w / 2, left + w }, { 0, sw / 2, sw })
			local dy; dy, gy = snapAxis({ top, top + h / 2, top + h }, { 0, sh / 2, sh })
			if dx or dy then Layout.Move(drag.id, rx + (dx or 0), ry + (dy or 0)) end   -- offset is a screen translation, so snap delta maps 1:1
		end
		showSnapGuide(gx, gy)
		if refreshInspector then refreshInspector() end
	else
		showSnapGuide(nil, nil)
	end
	for id in pairs(boxes) do placeBox(id) end
end)

local function setActive(v)
	if v == active then return end
	if v and not EmberUI.inWorld then return end
	if v then buildAll() end
	active = v
	if v then
		for _, b in pairs(boxes) do b.frame:Show() end
		insp.frame:Show()
		layoutCenterGuides(); showSnapGuide(nil, nil); guideFrame:Show()
		selectId(nil)
	else
		for _, b in pairs(boxes) do b.frame:Hide() end
		if insp then insp.frame:Hide() end
		if guideFrame then guideFrame:Hide() end
		drag = nil; selected = nil
	end
end

function EmberUI.LayoutEditor.Toggle()
	setActive(not active)
end

local stage = CreateFrame('Frame', nil, nil)
stage:SetScript('OnEvent', function(_, event)
	if event == Events.EDIT_MODE_TOGGLE then
		EmberUI.LayoutEditor.Toggle()
	elseif active then
		setActive(false)   -- leaving the world closes the editor
	end
end)
stage:RegisterEvent(Events.EDIT_MODE_TOGGLE)
stage:RegisterEvent(Events.LOGIN_SHOWN)
stage:RegisterEvent(Events.CHARSELECT_SHOWN)
stage:RegisterEvent(Events.CHARCREATE_SHOWN)

print('EmberUI layout editor loaded')
