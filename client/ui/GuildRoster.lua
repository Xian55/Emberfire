-- Lua Guild Roster: member list replacing the C++ GuildRoster (force-hidden, mirrored). The live window stays
-- the model (populated by GP_Server_GuildRoster; onOpen requests it); this view reads it via GetGuildMember /
-- GetGuildName / GetGuildMotd and drives actions (promote/demote/kick/whisper/invite). Right-click a member for
-- the context menu (gated by your rank). Event-driven: PANEL_OPENED/CLOSED('GuildRosterFrame') show/hide,
-- GUILD_ROSTER_UPDATE refreshes. NO OnUpdate. NOTE: positions copied from C++ GuildRoster::setGuild — tune live.
-- v1: MOTD is display-only (no Lua text-prompt binding yet); kick sends the C++ packet but the server opcode is
-- a placeholder (TODO server-side).

local NAME_X, LVL_X, CLASS_X, LIST_Y, ROWH = 35, 183, 301, 130, 18
local VISIBLE = 15
local TITLE_X, TITLE_Y = 40, 95
local MOTD_X, MOTD_Y = 40, 475
local ONLINE_X, ONLINE_Y = 311, 427
local TICK_X, TICK_Y = 155, 425

local W, H = 457, 615
local win = EmberUI.CreateWindow{ name = 'EmberGuildRoster', width = W, height = H, title = 'Guild' }
local root = win.frame
root:SetPoint('CENTER')
root:Hide()

local function vis(f, v) if v then f:Show() else f:Hide() end end
local function texSize(name, dw, dh) local w, h = GetTextureSize(name); if w <= 0 then return dw, dh end return w, h end

local titleFS = root:CreateFontString()
titleFS:SetFont('Fontin'); titleFS:SetFontSize(16); titleFS:SetTextColor(223, 168, 2, 255)
titleFS:SetPoint('TOPLEFT', root, 'TOPLEFT', TITLE_X, TITLE_Y)
local onlineFS = root:CreateFontString()
onlineFS:SetFont('Ringbearer'); onlineFS:SetFontSize(16); onlineFS:SetTextColor(255, 255, 255, 255)
onlineFS:SetPoint('TOPLEFT', root, 'TOPLEFT', ONLINE_X, ONLINE_Y)
local motdFS = root:CreateFontString()
motdFS:SetFont('Helvetica'); motdFS:SetFontSize(14); motdFS:SetTextColor(200, 200, 200, 255)
motdFS:SetPoint('TOPLEFT', root, 'TOPLEFT', MOTD_X, MOTD_Y)

local showOffline = true
local scroll = 0
local refresh   -- forward decl

-- right-click member menu (gated by our rank: Officer=2 / Leader=3 can promote/demote/kick lower ranks)
local function memberMenu(m)
	local items = {}
	if m.online then
		items[#items + 1] = { text = 'Whisper', onClick = function() WhisperPlayer(m.name) end }
		items[#items + 1] = { text = 'Invite',  onClick = function() InviteToParty(m.name) end }
	end
	local lr = GetGuildLocalRank()
	if lr >= 2 and lr > m.rank then
		items[#items + 1] = { text = 'Promote', onClick = function() GuildPromote(m.guid) end }
		if m.rank > 0 then
			items[#items + 1] = { text = 'Demote', onClick = function() GuildDemote(m.guid) end }
		end
		items[#items + 1] = { text = 'Kick', color = { 220, 90, 90 }, onClick = function() GuildKick(m.guid) end }
	end
	if #items == 0 then return end
	items[#items + 1] = { text = 'Cancel' }
	EmberUI.ShowMenu(items)
end

-- row pool
local rows = {}
for r = 1, VISIBLE do
	local y = LIST_Y + (r - 1) * ROWH
	local nameFS = root:CreateFontString(); nameFS:SetFont('Helvetica'); nameFS:SetFontSize(14)
	nameFS:SetPoint('TOPLEFT', root, 'TOPLEFT', NAME_X, y)
	local lvlFS = root:CreateFontString(); lvlFS:SetFont('Helvetica'); lvlFS:SetFontSize(14)
	lvlFS:SetPoint('TOPLEFT', root, 'TOPLEFT', LVL_X, y)
	local clsFS = root:CreateFontString(); clsFS:SetFont('Helvetica'); clsFS:SetFontSize(14)
	clsFS:SetPoint('TOPLEFT', root, 'TOPLEFT', CLASS_X, y)
	local hit = CreateFrame('Button', nil, root)
	hit:SetSize(360, ROWH); hit:SetPoint('TOPLEFT', root, 'TOPLEFT', NAME_X, y); hit:EnableMouse(true)
	local row = { name = nameFS, lvl = lvlFS, cls = clsFS, hit = hit }
	hit:SetScript('OnMouseUp', function(_, btn)
		if btn == 'RightButton' and row._member then memberMenu(row._member) end
	end)
	rows[r] = row
end

-- show-offline toggle
local tw, th = texSize('tick_box_full.png', 18, 18)
local tick = CreateFrame('Button', nil, root)
tick:SetSize(tw, th); tick:SetPoint('TOPLEFT', root, 'TOPLEFT', TICK_X, TICK_Y); tick:EnableMouse(true)
local function refreshTick() tick:SetTexture(showOffline and 'tick_box_full.png' or 'tick_box_empty.png') end
tick:SetScript('OnClick', function() showOffline = not showOffline; refreshTick(); refresh() end)

refresh = function()
	titleFS:SetText(GetGuildName())
	motdFS:SetText(GetGuildMotd())

	local list, onlineN = {}, 0
	local total = GetNumGuildMembers()
	for i = 1, total do
		local name, level, online, guid, cls, cr, cg, cb, rankName, rank = GetGuildMember(i)
		if online then onlineN = onlineN + 1 end
		if showOffline or online then
			list[#list + 1] = { name = name, level = level, online = online, guid = guid,
			                    cls = cls, cr = cr, cg = cg, cb = cb, rank = rank }
		end
	end
	onlineFS:SetText(tostring(onlineN))

	local maxS = math.max(0, #list - VISIBLE)
	if scroll > maxS then scroll = maxS end
	if scroll < 0 then scroll = 0 end

	for r = 1, VISIBLE do
		local m = list[scroll + r]
		local row = rows[r]
		if m then
			local a = m.online and 255 or 128
			row.name:SetText(m.name); row.name:SetTextColor(223, 168, 2, a); row.name:Show()
			row.lvl:SetText('lv. ' .. m.level); row.lvl:SetTextColor(255, 255, 255, a); row.lvl:Show()
			row.cls:SetText(m.cls); row.cls:SetTextColor(m.cr, m.cg, m.cb, a); row.cls:Show()
			row._member = m; row.hit:Show()
		else
			row.name:Hide(); row.lvl:Hide(); row.cls:Hide(); row._member = nil; row.hit:Hide()
		end
	end
end

root:SetScript('OnMouseWheel', function(_, delta) scroll = scroll - delta; refresh() end)

local shown = false
local function setShown(v)
	if v == shown then return end
	shown = v
	if v then refreshTick(); root:Show(); refresh() else root:Hide(); EmberUI.HideMenu() end
end

local stage = CreateFrame('Frame', nil, nil)
stage:SetScript('OnEvent', function(_, event, arg)
	if event == Events.WORLD_SHOWN then
		SetGameFrameShown('GuildRosterFrame', false)
	elseif event == Events.PANEL_OPENED then
		if arg == 'GuildRosterFrame' then setShown(true) end
	elseif event == Events.PANEL_CLOSED then
		if arg == 'GuildRosterFrame' then setShown(false) end
	elseif event == Events.GUILD_ROSTER_UPDATE then
		if shown then refresh() end
	else
		setShown(false)
	end
end)
for _, e in ipairs({ Events.WORLD_SHOWN, Events.PANEL_OPENED, Events.PANEL_CLOSED, Events.GUILD_ROSTER_UPDATE,
                     Events.LOGIN_SHOWN, Events.CHARSELECT_SHOWN, Events.CHARCREATE_SHOWN }) do
	stage:RegisterEvent(e)
end

print('EmberUI guild roster loaded')
