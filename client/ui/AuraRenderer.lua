-- Shared aura (buff/debuff) widget, reused by every unit frame (and later the minimap aura bars).
-- An aura = spell icon + a radial clockwise cooldown sweep (time remaining) + hover tooltip (spell name +
-- description). Optionally shows the remaining time as text below the icon.
--
-- EmberUI.CreateAura(parent, size [, opts]) -> aura
--   opts.duration = true  -> show remaining-time text below the icon (off by default)
-- aura:SetSize(size); aura:SetSpell(spellId, remMs, durMs); aura:Show(); aura:Hide()
-- aura.frame is the icon (anchor it with SetPoint).

-- Format ms -> short time string ("12", "1.2k"-> use m for minutes).
local function fmtTime(ms)
	local s = math.floor(ms / 1000)
	if s >= 60 then return math.floor(s / 60) .. 'm' end
	return '' .. s
end

function EmberUI.CreateAura(parent, size, opts)
	opts = opts or {}
	local a = { spellId = 0, hovering = false }

	-- icon (the public anchor target)
	a.frame = parent:CreateTexture()
	a.icon = a.frame
	a.frame:SetSize(size, size)

	-- radial sweep on top of the icon
	a.cd = CreateFrame('Cooldown', nil, parent)
	a.cd:SetSize(size, size)
	a.cd:SetAllPoints(a.frame)

	-- optional remaining-time text below the icon
	if opts.duration then
		a.dur = parent:CreateFontString()
		a.dur:SetFont('Palatino'); a.dur:SetFontSize(11); a.dur:SetTextColor(255, 255, 255, 255)
		a.dur:SetPoint('TOP', a.frame, 'BOTTOM', 0, 1)
		a.dur:Hide()
	end

	-- hover tooltip: re-assert each frame while hovering (the tooltip is cleared per frame); also refresh
	-- the duration text live.
	a.frame:SetScript('OnEnter', function() a.hovering = true end)
	a.frame:SetScript('OnLeave', function() a.hovering = false end)
	a.frame:SetScript('OnUpdate', function()
		if a.spellId == 0 then return end
		if a.hovering then ShowSpellTooltip(a.spellId) end
		if a.dur then a.dur:SetText(fmtTime(a.cd:GetCooldownRemaining())) end
	end)

	function a:SetSize(sz)
		self.frame:SetSize(sz, sz); self.cd:SetSize(sz, sz)
	end

	function a:SetSpell(spellId, remMs, durMs)
		self.spellId = spellId or 0
		self.frame:SetTexture(GetSpellTexture(self.spellId))
		self.cd:SetCooldown(remMs or 0, durMs or 0)
		self.frame:Show(); self.cd:Show()
		if self.dur then
			if remMs and remMs > 0 then self.dur:SetText(fmtTime(remMs)); self.dur:Show() else self.dur:Hide() end
		end
	end

	function a:Show() self.frame:Show(); self.cd:Show(); if self.dur then self.dur:Show() end end
	function a:Hide()
		self.spellId = 0
		self.frame:Hide(); self.cd:Hide(); if self.dur then self.dur:Hide() end
	end

	a:Hide()
	return a
end

print('EmberUI aura renderer loaded')
