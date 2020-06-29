local DefaultTable = require('container/defaulttable')
local Deque = require('container/deque')

--
-- TODO: generalize to a tracker of some value over a lifetime
local Tracker = sky.Object:extend()

local function note_id(event)
  -- assumes event is a note
  return sky.to_id(event.ch, event.num)
end

local function ch_of(event)
  return event.ch
end

function Tracker:new(key_builder, value_extractor)
  Tracker.super.new(self)
  self.key_builder = key_builder or function(k) return k end
  self.value_extractor = value_extractor or function(v) return v end
  self.things = DefaultTable.new()
end

function Tracker:start(thing)
  local k = self.key_builder(thing)
  local v = self.value_extractor(thing)
  self.things[k] = v
  return v
end

function Tracker:get(thing)
  return self.things[self.key_builder(thing)]
end

function Tracker:stop(thing)
  local k = self.key_builder(thing)
  local v = self.things[k]
  self.things[k] = nil
  return v
end

return {
  Tracker = Tracker,

  -- helpers
  note_id = note_id,
  ch_of = ch_of,
}