-- Lua Game Chat: scrollback + edit box + TABS with per-tab filters, replacing the C++ GameChat's visuals.
-- The C++ GameChat stays the engine (line buffer + channel colors, channel state, "/" commands, packets,
-- item-link tooltips, combat-log buffer) flipped headless via SetChatLuaView; it still watches the world-level
-- open keys (Enter / "/") and fires CHAT_OPEN. WoW-shaped after Blizzard FloatingChatFrame/ChatFrame: each tab
-- has a message-group filter set (ChatFrame_AddMessageGroup-style), tab 2 is the dedicated Combat Log reading
-- the separate combat buffer (categories: my damage / incoming / heals / other). Right-click a tab = filter
-- menu (persisted via SaveUISetting bitmask). Right-click a line = Whisper/Invite. Hover a [item] line = its
-- tooltip. Live "/s /p /g /a /y /w name " channel swap while typing via ChatSwapChannel.

local VISIBLE, ROW_H = 8, 14
local LINES_X, LINES_Y0 = 30, 15   -- C++ lines area: y 7..127 (rel chat origin); 8 rows x 14 bottom-aligned at 127
local TAB_H = 16
local TAB_Y = 0                    -- tabs ride the backdrop's transparent top strip (WoW-style, above the box)

-- channel ids (ChatDefines): 0 Say 1 Yell 2 Whisper 3 Guild 4 Party 5 System 6 All 7 SysCenter 8 XP 9 Warning
local CHAT_GROUPS = {
	{ label = 'Say',     channels = { [0] = true } },
	{ label = 'Yell',    channels = { [1] = true } },
	{ label = 'Whisper', channels = { [2] = true } },
	{ label = 'Guild',   channels = { [3] = true } },
	{ label = 'Party',   channels = { [4] = true } },
	{ label = 'All',     channels = { [6] = true } },
	{ label = 'System',  channels = { [5] = true, [7] = true, [9] = true } },
	{ label = 'XP',      channels = { [8] = true } },
}
local COMBAT_GROUPS = {
	{ label = 'My damage', cats = { [1] = true } },
	{ label = 'Incoming',  cats = { [2] = true } },
	{ label = 'Heals',     cats = { [3] = true } },
	{ label = 'Other',     cats = { [4] = true } },
}

local W, H = GetTextureSize('game_chat_backdrop.png')
if W <= 0 then W, H = 500, 180 end
local root = CreateFrame('Frame', 'EmberGameChat', nil)
root:SetSize(W, H)
root:EnableMouse(true)   -- consume wheel over the chat area
EmberUI.Layout.Register('chat', root, { label = 'Chat', anchor = 'BOTTOMLEFT', x = 0, y = H - 250 })   -- movable in the layout editor
root:Hide()

local bg = root:CreateTexture()
bg:SetAllPoints(root)
bg:SetTexture('game_chat_backdrop.png')

-- ---- tabs (filters persisted as a group bitmask per tab) ----
local tabs = {
	{ name = 'General',    kind = 'chat',   groups = CHAT_GROUPS,   key = 'chat_tab1_groups', scroll = 0 },
	{ name = 'Combat Log', kind = 'combat', groups = COMBAT_GROUPS, key = 'chat_tab2_groups', scroll = 0 },
}
local activeTab = 1
local refresh   -- forward decl

for _, t in ipairs(tabs) do
	local mask = GetUISetting(t.key, -1)
	t.enabled = {}
	for gi = 1, #t.groups do
		-- default: every group on (integer bitops -- ^ is float pow in Lua 5.5)
		t.enabled[gi] = (mask < 0) or (((mask >> (gi - 1)) & 1) == 1)
	end
end

local function saveFilters(t)
	local mask = 0
	for gi = 1, #t.groups do if t.enabled[gi] then mask = mask | (1 << (gi - 1)) end end
	SaveUISetting(t.key, mask)
end

local function filterMenu(t)
	local items = { { text = t.name .. ' filters' } }
	for gi = 1, #t.groups do
		local g = t.groups[gi]
		items[#items + 1] = {
			text = (t.enabled[gi] and '+ ' or '- ') .. g.label,
			color = t.enabled[gi] and { 140, 220, 140 } or { 150, 150, 150 },
			onClick = function() t.enabled[gi] = not t.enabled[gi]; saveFilters(t); refresh() end,
		}
	end
	items[#items + 1] = { text = 'Close' }
	EmberUI.ShowMenu(items)
end

local tabFS = {}
for ti, t in ipairs(tabs) do
	local b = CreateFrame('Button', nil, root)
	b:SetSize(86, TAB_H); b:SetPoint('TOPLEFT', root, 'TOPLEFT', 30 + (ti - 1) * 92, TAB_Y); b:EnableMouse(true)
	b:SetScript('OnMouseUp', function(_, btn)
		if btn == 'RightButton' then filterMenu(t)
		else activeTab = ti; refresh() end
	end)
	local fs = root:CreateFontString()
	fs:SetFont('Arial'); fs:SetFontSize(12)
	fs:SetPoint('TOPLEFT', root, 'TOPLEFT', 34 + (ti - 1) * 92, TAB_Y + 1)
	fs:SetText(t.name)
	tabFS[ti] = fs
end

-- ---- line rows (bottom-up pool; row 1 = oldest visible, row VISIBLE = newest) ----
local rows = {}
for r = 1, VISIBLE do
	local fs = root:CreateFontString()
	fs:SetFont('Helvetica'); fs:SetFontSize(12)
	fs:SetPoint('TOPLEFT', root, 'TOPLEFT', LINES_X, LINES_Y0 + (r - 1) * ROW_H)
	local hit = CreateFrame('Button', nil, root)
	hit:SetSize(W - LINES_X * 2, ROW_H)
	hit:SetPoint('TOPLEFT', root, 'TOPLEFT', LINES_X, LINES_Y0 + (r - 1) * ROW_H)
	hit:EnableMouse(true)
	local row = { fs = fs, hit = hit }
	hit:SetScript('OnMouseUp', function(_, btn)
		if btn ~= 'RightButton' or not row._idx then return end
		local sender = GetChatSender(row._idx)
		if sender == '' then return end
		EmberUI.ShowMenu({
			{ text = sender },
			{ text = 'Whisper', onClick = function() WhisperPlayer(sender) end },
			{ text = 'Invite',  onClick = function() InviteToParty(sender) end },
			{ text = 'Cancel' },
		})
	end)
	rows[r] = row
end

-- ---- input row: channel prefix + edit box (no enter button -- Enter/"/" open it, Enter sends) ----
local prefixFS = root:CreateFontString()
prefixFS:SetFont('Helvetica'); prefixFS:SetFontSize(12)
prefixFS:SetPoint('TOPLEFT', root, 'TOPLEFT', 30, 157)   -- the C++ PromptBox row (attachObj 30,157)

local box = CreateFrame('EditBox', nil, root)
box:SetSize(350, 16); box:SetPoint('TOPLEFT', root, 'TOPLEFT', 80, 155)
box:SetMaxLetters(220); box:SetFontSize(12)

local function refreshPrefix()
	local text, r, g, b = GetChatPrefix()
	prefixFS:SetText(text); prefixFS:SetTextColor(r, g, b, 255)
	box:SetTextColor(r, g, b, 255)
end

local function closeInput()
	box:ClearFocus(); box:SetText('')
end

box:SetScript('OnEnterPressed', function()
	local text = box:GetText()
	if text ~= '' then SubmitChat(text) end
	closeInput(); refreshPrefix()
end)

-- ---- scroll buttons (up/down a notch + jump oldest/newest) ----
local function scrollBy(d)
	local t = tabs[activeTab]
	t.scroll = t.scroll + d
	refresh()
end
local function navButton(tex, x, y, fn)
	local b = CreateFrame('Button', nil, root)
	b:SetTexture(tex .. '_idle.png'); b:SetHoverTexture(tex .. '_hover.png')
	local w, h = GetTextureSize(tex .. '_idle.png')
	b:SetSize(w > 0 and w or 18, h > 0 and h or 18)
	b:SetPoint('TOPLEFT', root, 'TOPLEFT', x, y); b:EnableMouse(true)
	b:SetScript('OnClick', fn)
	return b
end
-- the C++ rail: fullup (5,71), up (5,101), down (5,125), fulldown (5,149)
navButton('game_chat_fullup', 5, 71, function() tabs[activeTab].scroll = 100000; refresh() end)
navButton('game_chat_up', 5, 101, function() scrollBy(1) end)
navButton('game_chat_down', 5, 125, function() scrollBy(-1) end)
navButton('game_chat_fulldown', 5, 149, function() tabs[activeTab].scroll = 0; refresh() end)
root:SetScript('OnMouseWheel', function(_, delta) scrollBy(delta > 0 and 1 or -1) end)

-- ---- refresh: collect the active tab's filtered lines, render the scrolled window ----
local function chatLinePasses(t, channel)
	for gi = 1, #t.groups do
		if t.enabled[gi] and t.groups[gi].channels[channel] then return true end
	end
	return false
end

refresh = function()
	local t = tabs[activeTab]
	for ti = 1, #tabs do
		if ti == activeTab then tabFS[ti]:SetTextColor(255, 224, 150, 255)
		else EmberUI.SetColor(tabFS[ti], EmberUI.Color.Muted) end
	end

	-- visible = { {text, r,g,b, idx(real, chat only), hasLink}, ... } oldest->newest, post-filter
	local visible = {}
	if t.kind == 'chat' then
		local n = GetChatLineCount()
		for i = 1, n do
			local text, r, g, b, hasLink, channel = GetChatLine(i)
			if text ~= '' and chatLinePasses(t, channel) then
				visible[#visible + 1] = { text = text, r = r, g = g, b = b, idx = i, link = hasLink }
			end
		end
	else
		local n = GetCombatLogCount()
		for i = 1, n do
			local text, r, g, b, cat = GetCombatLogLine(i)
			local on = false
			for gi = 1, #t.groups do if t.enabled[gi] and t.groups[gi].cats[cat] then on = true break end end
			if text ~= '' and on then
				visible[#visible + 1] = { text = text, r = r, g = g, b = b }
			end
		end
	end

	local maxScroll = math.max(0, #visible - VISIBLE)
	if t.scroll > maxScroll then t.scroll = maxScroll end
	if t.scroll < 0 then t.scroll = 0 end

	for r = 1, VISIBLE do
		local row = rows[r]
		-- row VISIBLE shows entry (#visible - scroll), row 1 shows (#visible - scroll - VISIBLE + 1)
		local vi = #visible - t.scroll - (VISIBLE - r)
		local e = visible[vi]
		if e then
			row.fs:SetText(e.text); row.fs:SetTextColor(e.r, e.g, e.b, 255); row.fs:Show()
			row._idx = e.idx
			row.hit:SetTooltipChatLink(e.link and e.idx or 0, 'TOP')
			row.hit:Show()
		else
			row._idx = nil
			row.fs:Hide(); row.hit:SetTooltipChatLink(0); row.hit:Hide()
		end
	end
end

-- ---- focus poll (only while focused): live channel swap + prefix + Escape-closed detection ----
local hadFocus = false
local poll = CreateFrame('Frame', nil, nil)
poll:SetScript('OnUpdate', function()
	local has = box:HasFocus()
	if has then
		local text = box:GetText()
		if text ~= '' and ChatSwapChannel(text) then box:SetText('') end
		refreshPrefix()
	elseif hadFocus then
		box:SetText('')   -- lost focus (Escape engine-side / clicked away): drop the draft
	end
	hadFocus = has
end)

-- ---- stage events ----
local shown = false
local function setShown(v)
	if v == shown then return end
	shown = v
	if v then
		EmberUI.Layout.Place('chat')   -- saved anchor/offset (editor-movable); default = bottom-left
		root:Show(); refreshPrefix(); refresh()
	else
		root:Hide(); closeInput()
	end
end

local stage = CreateFrame('Frame', nil, nil)
stage:SetScript('OnEvent', function(_, event, arg)
	if event == Events.WORLD_SHOWN then
		SetChatLuaView(true)   -- C++ chat goes headless (keeps the line/command engine + open keys)
		setShown(true)
	elseif event == Events.LOGIN_SHOWN or event == Events.CHARSELECT_SHOWN or event == Events.CHARCREATE_SHOWN then
		setShown(false)
	elseif event == Events.CHAT_MSG then
		if shown then
			local t = tabs[activeTab]
			if arg == 'combat' then
				if t.kind == 'combat' then refresh() end
			elseif t.kind == 'chat' then
				refresh()
			end
		end
	elseif event == Events.CHAT_OPEN then
		if shown then
			box:SetFocus()
			if arg == 'slash' then box:SetText('/') end
			refreshPrefix()
		end
	end
end)
for _, e in ipairs({ Events.WORLD_SHOWN, Events.LOGIN_SHOWN, Events.CHARSELECT_SHOWN, Events.CHARCREATE_SHOWN,
                     Events.CHAT_MSG, Events.CHAT_OPEN }) do
	stage:RegisterEvent(e)
end

print('EmberUI game chat loaded')
