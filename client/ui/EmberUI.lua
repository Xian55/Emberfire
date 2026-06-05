-- Emberfire default UI — core namespace + binding helpers.
-- This is the built-in "FrameXML": it loads before user addons and is fully overridable (its frames are
-- globals an addon can Hide()). Lua = visualization only; all game data/logic comes from the C++ API
-- (getters like UnitHealth, events in the Events table, commands like TargetUnit).

EmberUI = EmberUI or {}

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

print('EmberUI core loaded')
