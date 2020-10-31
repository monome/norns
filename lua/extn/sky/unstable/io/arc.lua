local math = require('math')
local util = require('util')
local filters = require('filters')

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

function ArcInput.is_enc(event)
  return sky.is_type(event, ArcInput.ARC_ENC_EVENT)
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
  if sky.is_type(event, ArcInput.ARC_ENC_EVENT) and event.n == self.which then
    local next, normalized = self:value(event.delta)
    if next ~= self._value then
      self._value = next
      output(self.mk_dial(self.which, next, normalized, event.arc))
    end
  elseif sky.is_init(event) then
    output(event) -- allow other devices to get the init event
    local value, normalized = self:value()
    output(self.mk_dial(self.which, value, normalized))
  else
    -- if not an arc enc event pass on through
    output(event)
  end
end

function ArcDialGesture:value(delta)
  local range = self:range()

  if delta == nil then
    return self._value, self._value / range
  end

  local inc = range / self.steps
  local change = inc * delta * self.scale
  local v = util.clamp(self._value + change, self.min, self.max)
  return v, v / range
end

function ArcDialGesture:range()
  return math.abs(self.max - self.min)
end

function ArcDialGesture.mk_dial(n, value, normalized, arc)
  return {
    type = ArcDialGesture.ARC_DIAL_EVENT,
    arc = arc,
    n = n,
    value = value,
    normalized = normalized,
  }
end

function ArcDialGesture.is_dial(event)
  return sky.is_type(event, ArcDialGesture.ARC_DIAL_EVENT)
end

--
-- ArcDialSmoother
--

local ArcDialSmoother = sky.Device:extend()
ArcDialSmoother.STEP_EVENT = 'ARC_DIAL_SMOOTHER_STEP'

function ArcDialSmoother:new(props)
  ArcDialSmoother.super.new(self, props)
  self.which = props.which or 1
  self._time = props.time or 1
  self._sr = props.sr or 5
  self._scheduler = nil
  self._smoother = filters.smoother.new(self._time, self._sr)
  self._smoothing = false
  self._target = nil

  self._step = { type = self.STEP_EVENT }
end

function ArcDialSmoother:device_inserted(chain)
  if self._scheduler ~= nil then
    error('ArcDialSmoother: one instance cannot be used in multiple chains at the same time')
  end
  self._scheduler = chain:scheduler(self)
end

function ArcDialSmoother:device_removed(chain)
  self._scheduler = nil
end

function ArcDialSmoother:set_time(t)
  self._time = t
  self._smoother:set_time(t)
end

function ArcDialSmoother:do_step(x)
  local e = sky.clone(self._target)
  e.target = e.normalized
  e.normalized = self._smoother:next(x)
  return e
end

function ArcDialSmoother:process(event, output)
  if sky.is_type(event, self.STEP_EVENT) then
    local next = self:do_step()
    if self._smoothing then
      self._scheduler:sleep(1 / self._sr, self._step)
      self._smoothing = math.abs(next.normalized - self._target.normalized) > self._smoother.EPSILON -- 0.001
    end
    output(next)
    return
  end

  if ArcDialGesture.is_dial(event) and event.n == self.which then
    self._target = sky.clone(event)
    if not self._smoothing then
      output(self:do_step(self._target.normalized))
      if self._time > 0 then
        self._smoothing = true
        self._scheduler:sleep(1 / self._sr, self._step)
      end
    else
      self._smoother:set_target(self._target.normalized)
    end

    return
  end

  output(event)
end

--
-- ArcDialRender
--
local ArcDialRender = sky.Object:extend()

local TWO_PI = math.pi * 2
local SLIM_WIDTH = TWO_PI / 64

function ArcDialRender:new(props)
  ArcDialRender.super.new(self, props)
  self.which = props.which or 1
  self.where = props.where or props.which
  self.width = props.width or SLIM_WIDTH
  self.level = props.level or 8
  if props.target_indicator ~= nil then self.target_indicator = props.target_indicator
  else self.target_indicator = true end
  self.target_level = props.target_level or props.level or 15
  self.mode = props.mode or 'pointer'
end

function ArcDialRender:clear(props, which)
  props.arc:segment(which, 0, TWO_PI, 0)
end

local function round(x)
  return math.floor(x + 0.5)
end

function ArcDialRender:render(event, props)
  if sky.is_type(event, ArcDialGesture.ARC_DIAL_EVENT) and event.n == self.which then
    local which = props.position or self.where
    self:clear(props, which)
    local point = round(64 * event.normalized)
    if self.mode == 'pointer' then
      props.arc:led(which, point + 1, self.level)
    elseif self.mode == 'segment' then
      -- NB: clamp angle to just below TWO_PI otherwise start and end angle are the
      -- same and nothing is draw by the segment method.
      local v = util.clamp(event.normalized * TWO_PI, 0 , TWO_PI - 0.001)
      props.arc:segment(which, 0 + SLIM_WIDTH, v + SLIM_WIDTH, self.level)
    elseif self.mode == 'range' then
      local w = self.width / 2
      local p = event.normalized * TWO_PI + SLIM_WIDTH
      props.arc:segment(which, p - w, p + w, self.level)
    end

    -- point indicator for smoothed dials
    if event.target ~= nil and self.target_indicator then
      point = round(64 * event.target)
      props.arc:led(which, point + 1, self.target_level)
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

function ArcDisplay.null_render()
  local Null = sky.Object:extend()
  function Null:render(...) end
  return Null
end

return {
  ArcInput = ArcInput,
  ArcDialGesture = ArcDialGesture,
  ArcDialSmoother = ArcDialSmoother,
  ArcDialRender = ArcDialRender,
  ArcDisplay = ArcDisplay,

  -- constants
  TWO_PI = TWO_PI,

  ARC_DIAL_EVENT = ARC_DIAL_EVENT,
  ARC_ENC_EVENT = ARC_ENC_EVENT,
}




