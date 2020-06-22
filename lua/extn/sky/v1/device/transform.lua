local util = require 'util'

--
-- Channel
--

local Channel = sky.Device:extend()

function Channel:new(props)
  Channel.super.new(self, props)
  self:set_channel(props.channel)
  self._active = {}
end

function Channel:set_channel(ch)
  if ch == nil then
    self._channel = nil
  else
    self._channel = util.clamp(ch, 1, 16)
  end
end

function Channel:process(event, output)
  if not self.bypass then
    local id = event.correlation
    if sky.is_type(event, sky.types.NOTE_ON) then
      if self._channel then
        self._active[id] = self._channel
        event.ch = self._channel
      end
    elseif sky.is_type(event, sky.types.NOTE_OFF) then
      local prior = self._active[id]
      if prior then
        self._active[id] = nil
        event.ch = prior
      elseif self._channel then
        event.ch = self._channel
      end
    elseif self._channel then
      -- non-note event, just set channel
      event.ch = self._channel
    end
  end
  output(event)
end

--
--
-- ClockDiv
--

local ClockDiv = sky.Device:extend()

function ClockDiv:new(props)
  ClockDiv.super.new(self, props)
  self.div = props.div or 1
  -- TODO: allow targeting of a single channel
  -- TODO: follow START, STOP, RESET events to (re)sync div
  -- TODO: catch assignment to "div" and reset sync? (need to move div
  --       into a props table in order to take advantage of __newindex

  -- TODO: external midi clock sync
end

function ClockDiv:set_div(div)
  self._div = util.clamp(div, 1, 1024)
end

function ClockDiv:process(event, output)
  if event.type == sky.types.CLOCK then
    if (event.stage % self._div) == 0 then
      output(event)
    end
  else
    output(event)
  end
end

--
-- Pitch (midi semitones)
--

local Pitch = sky.Device:extend()

function Pitch:new(props)
  Pitch.super.new(self, props)
  self:set_semitones(props.semitones or 0)
  self._active = {}
end

function Pitch:set_semitones(st)
  self._semitones = util.clamp(st, -127, 127)
end

function Pitch:process(event, output)
  if not self.bypass then
    local id = event.correlation
    if sky.is_type(event, sky.types.NOTE_ON) then
      self._active[id] = self._semitones
      event.note = util.clamp(event.note + self._semitones, 0, 127)
    elseif sky.is_type(event, sky.types.NOTE_OFF) then
      local prior = self._active[id]
      self._active[id] = nil
      local semitones = prior or self._semitones
      event.note = util.clamp(event.note + semitones, 0, 127)
    end
  end
  output(event)
end

return {
  Channel = Channel,
  ClockDiv = ClockDiv,
  Pitch = Pitch,
}