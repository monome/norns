local table = require('table')
local Deque = require('container/deque')

--
-- Held(note) class
--
local Held = sky.Device:extend()
Held.EVENT = 'HELD'
Held.HOLD_STATE_EVENT = 'HOLD_STATE'

function Held:new(props)
  Held.super.new(self, props)
  self.debug = props.debug or false
  self._hold = props.hold or false
  self._hold_count = 0
  self._hold_should_reset = false
  self._hold_has_changed = false
  self._ordering = Deque.new()
end

function Held.__newindex(o, k, v)
  -- catch changes to 'hold' property, signal reset when hold is false
  if k == 'hold' then
    rawset(o, '_hold', v)
    if not v then o:reset() end
  else
    rawset(o, k, v)
  end
end

function Held.mk_event(notes)
  return { type = Held.EVENT, notes = notes }
end

function Held.mk_hold_state(state)
  return { type = Held.HOLD_STATE_EVENT, state = state, }
end

function Held.is_match(a, b)
  return (a.ch == b.ch) and (a.note == b.note)
end

function Held:reset()
  self._ordering:clear()
  self._hold_count = 0
  self._hold_should_reset = false
  self._hold_has_changed = true
end

function Held:track_note_off(event)
  if self._hold and self._ordering:contains(event, self.is_match) then
    self._hold_count = self._hold_count - 1
    if self._hold_count <= 0 then
      self._hold_should_reset = true
      return false
    end
  else
    local match = self._ordering:remove(event, self.is_match)
    if match ~= nil then
      self._hold_count = self._hold_count - 1
      return true
    end
    -- print("track off: ")
    -- for i,v in ipairs(self._ordering:to_array()) do
    --   print(i, '\t', sky.to_string(v))
    -- end
    return false
  end
end

function Held:track_note_on(event)
  if self._hold_should_reset then
    self:reset()
  end
  self._ordering:push_back(event)
  self._hold_count = self._hold_count + 1
  -- print("track on: ")
  -- for i,v in ipairs(self._ordering:to_array()) do
  --   print(i, '\t', sky.to_string(v))
  -- end
  return true
end

function Held:process(event, output)
  local changed = false
  local t = event.type

  if t == sky.types.NOTE_ON then
    if event.vel == 0 then
      changed = self:track_note_off(event)
    else
      event.beat = clock.get_beats()
      changed = self:track_note_on(event)
    end
  elseif t == sky.types.NOTE_OFF then
    changed = self:track_note_off(event)
  elseif t == Held.HOLD_STATE_EVENT then
    self.hold = event.state
    changed = self._hold_has_changed
  else
    -- pass unprocessed events
    output(event)
  end

  -- if hold was changed but no notes have occured since then ensure a new held
  -- event goes out with the clock
  if self._hold_has_changed and sky.is_clock(event) then
    self._hold_has_changed = false
    changed = true
  end

  if changed then
    local held = self._ordering:to_array()

    -- debug
    if self.debug then
      print("HELD >>")
      for i, e in ipairs(held) do
	      print(i, sky.to_string(e))
      end
      print("<<")
    end

    output(self.mk_event(held))
  end
end


--
-- Pattern class
--
local Pattern = sky.Device:extend()
Pattern.EVENT = 'PATTERN'
Pattern.builder = {}

function Pattern:new(props)
  Pattern.super.new(self, props)
  self.style = props.syle or 'as_played'
  self.debug = props.debug or false
end

function Pattern.mk_event(value)
  return { type = Pattern.EVENT, value = value }
end

function Pattern:process(event, output, state)
  if event.type == Held.EVENT then
    local builder = self.builder[self.style]
    if builder ~= nil then
      local pattern = builder(event.notes)
      output(self.mk_event(pattern))
      if self.debug then
        print("PAT >>>")
        for i, e in ipairs(pattern) do
          print(i, sky.to_string(e))
        end
        print("<<< PAT")
      end
    end
  else
    output(event)
  end
end

function Pattern.builder.up(notes)
  local cmp = function(a, b)
    return a.note < b.note
  end
  -- MAINT: in-place sort so note order is lost
  if #notes > 1 then
    table.sort(notes, cmp)
  end
  return notes
end

function Pattern.builder.down(notes)
  local cmp = function(a, b)
    return a.note > b.note
  end
  if #notes > 1 then
    table.sort(notes, cmp)
  end
  return notes
end

function Pattern.builder.up_down(notes)
end

function Pattern.builder.up_and_down(notes)
end

function Pattern.builder.converge(notes)
end

function Pattern.builder.diverge(notes)
end

function Pattern.builder.as_played(notes)
  return notes
end

function Pattern.builder.random(notes)
end


--
-- Arp class
--
local Arp = sky.Device:extend()
Arp.ARP_IMMEDIATE_MODE = 'immediate'
Arp.ARP_QUEUE_MODE = 'queue'

function Arp:new(props)
  Arp.super.new(self, props)
  self.mode = props.mode or Arp.ARP_IMMEDIATE_MODE
  self._pattern = nil
  self._next_pattern = nil
  self._step = 1
  self._length = 0
  self._last = nil
end

function Arp:set_pattern(notes)
  self._pattern = notes
  self._step = 1
  self._length = #notes
end

function Arp:queue_pattern(notes)
  self._next_pattern = notes
end

function Arp:switch_pattern()
  --print("switch pattern")
  self:set_pattern(self._next_pattern)
  self._next_pattern = nil
end

function Arp:process(event, output, state)
  if event.type == Pattern.EVENT then
    -- capture and queue up new pattern
    --print("arp got pattern change")
    if self.mode == Arp.ARP_IMMEDIATE_MODE then
      self:set_pattern(event.value)
      --print("set pattern")
      return
    elseif self.mode == Arp.ARP_QUEUE_MODE then
      if self._length <= 0 then
        -- fallback case; nothing is playing, just set_patther
        self:set_pattern(event.value)
        --print("queue immediate pattern")
      else
        self:queue_pattern(event.value)
        --print("queue pattern")
      end
      return
    end
  end

  if sky.is_clock(event) then
    local last = self._last
    if last ~= nil then
      -- kill previous
      local off = sky.mk_note_off(last.note, last.vel, last.ch)
      self._last = nil
      output(off)
    end

    if self._pattern ~= nil and self._length > 0 then
      local n = self._step
      local next = self._pattern[n]
      output(next)
      self._last = next
      n = n + 1
      if n > self._length then
        if self._next_pattern then self:switch_pattern() end
	      self._step = 1
      else
	      self._step = n
      end
    end
  end

  if sky.is_note(event) then
    -- don't pass notes
    return
  end

  -- pass everything else
  output(event)
end

--
-- module
--

return {
  Held = Held,
  Pattern = Pattern,
  Arp = Arp,
  -- constants
  ARP_IMMEDIATE_MODE = Arp.ARP_IMMEDIATE_MODE,
  ARP_QUEUE_MODE = Arp.ARP_QUEUE_MODE,

  -- exported event types
  HELD_EVENT = Held.EVENT,
  PATTERN_EVENT = Pattern.EVENT,
}