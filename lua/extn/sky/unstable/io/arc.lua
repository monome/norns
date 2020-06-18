local math = require('math')

--
-- ArcInput
--
local ArcInput = sky.InputBase:extend()
ArcInput.ARC_ENC_EVENT = 'ARC_ENC'

function ArcInput:new(props)
  ArcInput.super.new(self, props)
  self.arc = props.arc or arc.connect()
  if self.arc then
    self.arc.delta = function(...) self:on_enc_event(...) end
  end
end

function ArcInput:on_enc_event(n, delta)
  if self.chain then
    self.chain:process({
      type = ArcInput.ARC_ENC_EVENT,
      arc = self.arc,
      n = n,
      delta = delta,
    })
  end
end


--
-- ArcDialGesture
--
local ArcDialGesture = sky.Device:extend()
ArcDialGesture.ARC_DIAL_EVENT = 'ARC_DIAL'

function ArcDialGesture:new(props)
  ArcDialGesture.super.new(self, props)
  -- dial properties
  self.which = props.which or 1
  self.initial = props.initial or 0
  self.min = props.min or 0
  self.max = props.max or 1
  self.steps = props.steps or 64
  self.scale = props.scale or 0.25
  -- what is the dial value
  self._value = self.initial
end

function ArcDialGesture:process(event, output, state)
  if sky.is_type(event, ArcInput.ARC_ENC_EVENT) then
    if event.n == self.which then
      local range = self.max - self.min
      local inc = range / self.steps
      local change = inc * event.delta * self.scale
      local next = util.clamp(self._value + change, self.min, self.max)
      if next ~= self._value then
        self._value = next
        output({
          type = self.ARC_DIAL_EVENT,
          arc = event.arc,
          n = event.n,
          value = self._value,
          normalized = self._value / range,
        })
      end
    end
  end
  output(event)
end

--
-- ArcDialRender
--
local ArcDialRender = sky.Object:extend()

TWO_PI = math.pi * 2
SLIM_WIDTH = TWO_PI / 64

function ArcDialRender:new(props)
  ArcDialRender.super.new(self, props)
  self.which = props.which or 1
  self.where = props.where or props.which
  self.width = props.width or SLIM_WIDTH
  self.level = props.level or 8
  self.mode = props.mode or 'pointer'
end

function ArcDialRender:clear(props, which)
  props.arc:segment(which, 0, TWO_PI, 0)
end

function ArcDialRender:render(event, props)
  if sky.is_type(event, ArcDialGesture.ARC_DIAL_EVENT) and event.n == self.which then
    local which = props.position or self.where
    self:clear(props, which)
    local point = math.floor(64 * event.normalized)
    if self.mode == 'pointer' then
      props.arc:led(which, point, self.level)
    elseif self.mode == 'segment' then
      props.arc:segment(which, 0, event.normalized * TWO_PI, self.level)
    elseif self.mode == 'range' then
      local w = self.width / 2
      local p = event.normalized * TWO_PI
      props.arc:segment(which, p - w, p + w, self.level)
    end
  end
end

--
-- ArcDisplay
--
local ArcDisplay = sky.Device:extend()

function ArcDisplay:new(props)
  ArcDisplay.super.new(self, props)
  self.arc = props.arc or arc.connect()
  -- collect up the render objects
  for i,v in ipairs(props) do
    self[i] = v
  end
end

function ArcDisplay:process(event, output, state)
  local props = { arc = self.arc }
  for i, child in ipairs(self) do
    props.position = i
    child:render(event, props)
  end
  self.arc:refresh()
  output(event)
end

return {
  ArcInput = ArcInput,
  ArcDialGesture = ArcDialGesture,
  ArcDialRender = ArcDialRender,
  ArcDisplay = ArcDisplay,

  -- constants
  TWO_PI = TWO_PI,

  ARC_DIAL_EVENT = ARC_DIAL_EVENT,
  ARC_ENC_EVENT = ARC_ENC_EVENT,
}




