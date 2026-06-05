-- Reusable unit-frame widget template. ONE builder for every unit frame; player/target use the same
-- (large) layout and differ only by `mirror` (portrait left vs right + reverse art), party uses the same
-- template with a compact layout. Visualization only — all data via the C++ getters/events.
--
-- EmberUI.CreateUnitFrame(cfg) -> { frame, refresh, ... }
--   cfg = {
--     token   = "player"|"target"|"party1"..,
--     x, y,
--     art     = { frame=, hp=, mp= },   -- texture names
--     mirror  = bool,                    -- portrait on the right, layout flipped
--     layout  = <L>,                     -- one of the layout tables in UnitFrames.lua
--     show    = { name=, castbar=, elite=, leader=, repair= },
--   }

local TXT_COL  = { 147, 129, 114 }   -- bar pct/detail text (UnitFrame.cpp)
local LVL_COL  = { 132, 116, 89 }
local AURA_MAX = 8

-- Mirror helpers: a point reflects around the frame width; a box also subtracts its own width.
local function mpoint(fw, x, mirror) return mirror and (fw - x) or x end
local function mbox(fw, x, w, mirror) return mirror and (fw - x - w) or x end

local function fontString(f, x, y, size, col, font)
	local t = f:CreateFontString()
	t:SetFont(font or 'Palatino'); t:SetFontSize(size or 12)
	t:SetTextColor(col[1], col[2], col[3], 255)
	t:SetPoint('TOPLEFT', x, y)
	return t
end

-- ---- sub-builders: each adds one concern to the frame and records its handles on `st` ----

local function buildPortrait(f, fw, L, mirror, st)
	local cx, cy, r = L.portrait[1], L.portrait[2], L.portrait[3]
	st.portrait = f:CreateTexture()
	st.portrait:SetSize(r * 2, r * 2)
	st.portrait:SetPoint('TOPLEFT', mpoint(fw, cx, mirror) - r, cy - r)
end

local function buildBars(f, fw, L, art, mirror, st)
	local function bar(tex, tl)
		local b = CreateFrame('StatusBar', nil, f)
		b:SetStatusBarTexture(tex)
		local w, h = GetTextureSize(tex); if w <= 0 then w, h = 180, 16 end
		b:SetSize(w, h)
		b:SetPoint('TOPLEFT', mbox(fw, tl[1], w, mirror), tl[2])
		b:SetMinMaxValues(0, 1); b:SetValue(1)
		return b
	end
	-- text centered ON the bar (relativeTo) so it mirrors automatically and never lands on the portrait
	local function barText(bar)
		local t = f:CreateFontString(); t:SetFont('Palatino'); t:SetFontSize(12)
		t:SetTextColor(TXT_COL[1], TXT_COL[2], TXT_COL[3], 255)
		t:SetPoint('CENTER', bar, 'CENTER', 0, 0)
		return t
	end
	st.hp = bar(art.hp, L.hp)
	st.mp = bar(art.mp, L.mp)
	st.hpPct = barText(st.hp)
	st.mpPct = barText(st.mp)
	st.hpTxt = barText(st.hp); st.hpTxt:Hide()   -- cur/max replaces pct on hover (same spot)
	st.mpTxt = barText(st.mp); st.mpTxt:Hide()
end

local function buildLevel(f, fw, L, mirror, st)
	local bw, bh = GetTextureSize('unit_frame_level_bg.png'); if bw <= 0 then bw, bh = 33, 33 end
	local cx, cy = mpoint(fw, L.lvl[1], mirror), L.lvl[2]      -- layout point = badge CENTER
	local bg = f:CreateTexture(); bg:SetTexture('unit_frame_level_bg.png'); bg:SetSize(bw, bh)
	bg:SetPoint('TOPLEFT', cx - bw / 2, cy - bh / 2)
	st.lvl = f:CreateFontString(); st.lvl:SetFont('Palatino'); st.lvl:SetFontSize(12)
	st.lvl:SetTextColor(LVL_COL[1], LVL_COL[2], LVL_COL[3], 255)
	st.lvl:SetPoint('CENTER', bg, 'CENTER', 0, 0)             -- number centered on the badge
end

local function buildName(f, fw, L, mirror, st)
	st.name = fontString(f, mpoint(fw, L.name[1], mirror), L.name[2], 14, { 255, 255, 255 }, 'PalatinoBold')
end

local function buildIcons(f, fw, L, mirror, show, st)
	local function icon(tex, pos)
		local w, h = GetTextureSize(tex); if w <= 0 then w = 0 end
		local t = f:CreateTexture(); t:SetTexture(tex)
		t:SetPoint('TOPLEFT', mbox(fw, pos[1], w, mirror), pos[2]); t:Hide()
		return t
	end
	st.combat = icon('unitframe_combat.png', L.combat)
	if show.elite  then st.elite = icon('unit_frame_elite.png', L.elite); st.boss = icon('unit_frame_boss.png', L.elite) end
	if show.leader then st.leader = icon('unitframe_party_leader.png', L.leader) end
	if show.repair then
		st.repairI = icon('broken_item_uframe_icon.png', L.repairPos)
		st.arenaI  = icon('arena_queue_icon.png', L.repairPos)
	end
end

local function buildAuras(f, fw, L, mirror, st)
	local sz = L.auraSize
	local ax0 = mpoint(fw, L.auras[1], mirror)
	local dir = mirror and -1 or 1
	st.auras = {}
	for i = 1, AURA_MAX * 2 do
		local col = (i - 1) % AURA_MAX
		local ax = ax0 + dir * col * sz
		local ay = L.auras[2] + (i > AURA_MAX and sz or 0)   -- debuffs on the 2nd row
		local a = f:CreateTexture(); a:SetSize(sz - 2, sz - 2); a:SetPoint('TOPLEFT', ax, ay); a:Hide()
		local dur = f:CreateFontString(); dur:SetFont('Palatino'); dur:SetFontSize(10); dur:SetTextColor(255, 255, 255, 255)
		dur:SetPoint('TOPLEFT', ax, ay + sz - 10); dur:Hide()
		local slot = { icon = a, dur = dur, spellId = 0 }
		-- hover an aura icon -> show its spell tooltip (driven each frame from the unit OnUpdate)
		a:SetScript('OnEnter', function() st.hoverAura = slot.spellId end)
		a:SetScript('OnLeave', function() if st.hoverAura == slot.spellId then st.hoverAura = nil end end)
		st.auras[i] = slot
	end
end

local function buildCastBar(f, L, st)
	local cf = CreateFrame('Frame', nil, f)
	local cw, ch = GetTextureSize('castbar.png'); if cw <= 0 then cw, ch = 256, 32 end
	cf:SetSize(cw, ch); cf:SetPoint('TOPLEFT', L.castOffset[1], L.castOffset[2])
	local bg = cf:CreateTexture(); bg:SetTexture('castbar.png'); bg:SetAllPoints(cf)
	local fill = CreateFrame('StatusBar', nil, cf)
	fill:SetStatusBarTexture('castbar_fill.png')
	local fw2, fh2 = GetTextureSize('castbar_fill.png'); if fw2 <= 0 then fw2, fh2 = cw, ch end
	fill:SetSize(fw2, fh2); fill:SetPoint('TOPLEFT', 0, 0); fill:SetMinMaxValues(0, 1); fill:SetValue(0)
	local ic = cf:CreateTexture(); ic:SetSize(ch - 4, ch - 4); ic:SetPoint('TOPLEFT', 4, 2)
	local nm = cf:CreateFontString(); nm:SetFont('Palatino'); nm:SetFontSize(12); nm:SetTextColor(255, 255, 255, 255); nm:SetPoint('TOPLEFT', 32, 6)
	cf:Hide()
	st.cast = { frame = cf, fill = fill, icon = ic, name = nm }
end

-- ---- data refresh + per-frame behavior ----

local function makeRefresh(token, st, show)
	return function()
		if not UnitExists(token) then st.frame:Hide(); return end
		st.frame:Show()

		st.hpPct:SetText(math.floor((UnitHealth(token) * 100) / math.max(1, UnitHealthMax(token)) + 0.5) .. '%')
		st.mpPct:SetText(math.floor((UnitPower(token) * 100) / math.max(1, UnitPowerMax(token)) + 0.5) .. '%')
		st.hpTxt:SetText(UnitHealth(token) .. ' / ' .. UnitHealthMax(token))
		st.mpTxt:SetText(UnitPower(token) .. ' / ' .. UnitPowerMax(token))
		st.lvl:SetText('' .. UnitLevel(token))

		if st.name then
			st.name:SetText(string.upper(UnitName(token)))
			local r, g, b = UnitNameColor(token); st.name:SetTextColor(r, g, b, 255)
		end

		local pt = UnitPortraitTexture(token); if pt ~= '' then st.portrait:SetTexture(pt) end

		local function vis(w, on) if w then if on then w:Show() else w:Hide() end end end
		vis(st.combat, UnitFlag(token, 'InCombat') ~= 0)
		vis(st.elite,  UnitFlag(token, 'Elite') ~= 0)
		vis(st.boss,   UnitFlag(token, 'Boss') ~= 0)
		vis(st.leader, UnitIsPartyLeader(token))
		vis(st.arenaI, UnitFlag(token, 'InArenaQueue') ~= 0)

		for filterIdx = 0, 1 do
			local harmful = (filterIdx == 1)
			local n = UnitAuraCount(token, harmful)
			for i = 1, AURA_MAX do
				local slot = st.auras[filterIdx * AURA_MAX + i]
				if i <= n then
					local spellId, _, remMs = UnitAura(token, i, harmful)
					slot.spellId = spellId
					slot.icon:SetTexture(GetSpellTexture(spellId)); slot.icon:Show()
					if remMs and remMs > 0 then slot.dur:SetText(math.ceil(remMs / 1000) .. 's'); slot.dur:Show() else slot.dur:Hide() end
				else
					slot.spellId = 0; slot.icon:Hide(); slot.dur:Hide()
				end
			end
		end

		if st.cast then
			local sid = UnitCastSpell(token)
			if sid ~= 0 then st.cast.icon:SetTexture(GetSpellTexture(sid)); st.cast.name:SetText(GetSpellName(sid)); st.cast.frame:Show()
			else st.cast.frame:Hide() end
		end
	end
end

local function wireScripts(token, st)
	st.hpR, st.mpR, st.hovering, st.hoverAura = 1, 1, false, nil
	st.frame:SetScript('OnUpdate', function(_, dt)
		if not EmberUI.inWorld or not UnitExists(token) then return end
		local hpT = UnitHealth(token) / math.max(1, UnitHealthMax(token))
		local mpT = UnitPower(token)  / math.max(1, UnitPowerMax(token))
		local step = 2 * dt
		st.hpR = (st.hpR < hpT) and math.min(hpT, st.hpR + step) or math.max(hpT, st.hpR - step)
		st.mpR = (st.mpR < mpT) and math.min(mpT, st.mpR + step) or math.max(mpT, st.mpR - step)
		st.hp:SetValue(st.hpR); st.mp:SetValue(st.mpR)
		if st.cast and UnitCastSpell(token) ~= 0 then
			local total = UnitCastTotal(token)
			if total > 0 then st.cast.fill:SetValue(math.min(1, UnitCastElapsed(token) / total)) end
		end
		-- aura icon hover takes priority over the unit tooltip
		if st.hoverAura and st.hoverAura ~= 0 then ShowSpellTooltip(st.hoverAura)
		elseif st.hovering then ShowUnitTooltip(token) end
	end)
	st.frame:SetScript('OnClick', function() TargetUnit(token) end)
	-- open the context menu on right-button RELEASE (matches C++); opening on press would let the following
	-- release immediately close it (menu only stayed up while the button was held). Movable frames append a
	-- Lock/Unlock entry (handled Lua-side via CONTEXT_MENU_CLICK).
	st.frame:SetScript('OnMouseUp', function(_, button)
		if button == 'RightButton' then
			EmberUI._menuToken = token
			UnitContextMenu(token, st.movable and (st.locked and 'Unlock' or 'Lock') or '')
		end
	end)
	-- while unlocked, the engine moves the frame on drag; persist where it lands
	st.frame:SetScript('OnDragStop', function(self)
		SaveUISetting('uf_' .. token .. '_x', self:GetLeft())
		SaveUISetting('uf_' .. token .. '_y', self:GetTop())
	end)
	-- hover swaps pct -> cur/max (same centered spot, so they never overlap)
	st.frame:SetScript('OnEnter', function() st.hovering = true;  st.hpPct:Hide(); st.mpPct:Hide(); st.hpTxt:Show(); st.mpTxt:Show() end)
	st.frame:SetScript('OnLeave', function() st.hovering = false; st.hpTxt:Hide(); st.mpTxt:Hide(); st.hpPct:Show(); st.mpPct:Show() end)
end

-- The one public constructor.
function EmberUI.CreateUnitFrame(cfg)
	local L, art, mirror, show = cfg.layout, cfg.art, cfg.mirror or false, cfg.show or {}
	local fw, fh = GetTextureSize(art.frame); if fw <= 0 then fw, fh = 256, 128 end

	local f = CreateFrame('Button', 'EmberUI_' .. cfg.token .. 'Frame', nil)
	f:SetSize(fw, fh); f:EnableMouse(true)
	-- persisted position (config-backed) overrides the layout default, so a moved frame survives reload/restart
	local sx = GetUISetting('uf_' .. cfg.token .. '_x', cfg.x)
	local sy = GetUISetting('uf_' .. cfg.token .. '_y', cfg.y)
	f:SetPoint('TOPLEFT', sx, sy)
	local bgArt = f:CreateTexture(); bgArt:SetTexture(art.frame); bgArt:SetAllPoints(f)

	local st = { token = cfg.token, frame = f, locked = true, movable = (cfg.show and cfg.show.movable) or false }
	EmberUI._unitFrames = EmberUI._unitFrames or {}
	EmberUI._unitFrames[cfg.token] = st
	buildPortrait(f, fw, L, mirror, st)
	buildBars(f, fw, L, art, mirror, st)
	buildLevel(f, fw, L, mirror, st)
	if show.name then buildName(f, fw, L, mirror, st) end
	buildIcons(f, fw, L, mirror, show, st)
	buildAuras(f, fw, L, mirror, st)
	if show.castbar and L.castOffset then buildCastBar(f, L, st) end

	st.refresh = makeRefresh(cfg.token, st, show)
	wireScripts(cfg.token, st)
	return st
end

-- Lock (fix in place) / unlock (drag freely) a movable frame by token; persists on lock.
function EmberUI.SetFrameLocked(token, locked)
	local st = EmberUI._unitFrames and EmberUI._unitFrames[token]
	if not st or not st.movable then return end
	st.locked = locked
	if locked then
		st.frame:SetMovable(false)
		SaveUISetting('uf_' .. token .. '_x', st.frame:GetLeft())
		SaveUISetting('uf_' .. token .. '_y', st.frame:GetTop())
	else
		st.frame:SetMovable(true)
		st.frame:RegisterForDrag('LeftButton')
	end
end

print('EmberUI unit-frame template loaded')
