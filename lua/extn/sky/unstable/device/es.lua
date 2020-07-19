sky.use('io/grid')
local Deque = require('container/deque')

ES_DEFAULT_BOUNDS = {1,1,16,8}
ES_DEFAULT_ROOT_NOTE = 48

local function pack_xy(x, y)
  return x << 8 | y
end

local function unpack_x(packed)
  return packed >> 8
end

local function unpack_y(packed)
  return 0x0F & packed
end

local function note_for(root, x, y)
  return root + x + (y * 5)
end

local function build_note_map(root, width, height)
  local map = {}
  for x = 0, width-1 do
    for y = 0, height-1 do
      local note_num = note_for(root, x, y)
      local locations = map[note_num]
      if locations == nil then
        locations = {}
        map[note_num] = locations
      end
      table.insert(locations, {x, y})
    end
  end

  return function(note_num)
    return map[note_num]
  end
end

--
-- esNoteGesture
--
local esNoteGesture = sky.Device:extend()
esNoteGesture.EVENT = esNoteGesture

function esNoteGesture:new(props)
  esNoteGesture.super.new(self, props)
  self.root = props.root or ES_DEFAULT_ROOT_NOTE-- midi note number
  self.vel = props.vel or 127
  self.ch = props.ch or 1
end

function esNoteGesture:process(event, output, props)
  -- mark event as being processed
  event.esNoteGesture = self
  -- invert y and shift to zero based coordinates
  local x = event.local_x - 1
  local y = props.region.bounds[4] - event.local_y
  --print("note key: ", x, y)
  if event.z == 1 then
    local e = sky.mk_note_on(note_for(self.root, x, y), self.vel, self.ch)
    output(e)
  else
    local e = sky.mk_note_off(note_for(self.root, x, y), self.vel, self.ch)
    output(e)
  end
end

--
-- esShapeGesture
--
local esShapeGesture = sky.Device:extend()
esShapeGesture.EVENT = 'ES_SHAPE'

function esShapeGesture:new(props)
  esShapeGesture.super.new(self, props)
  self._held = Deque.new()
end

function esShapeGesture.match_key_event(a, b)
  return (a.local_x == b.local_x) and (a.local_y == b.local_y)
end

function esShapeGesture:mk_event(shape)
  return { type = esShapeGesture.EVENT, shape = shape }
end

function esShapeGesture:search(root, second, third)
  local next = self._held:ipairs()
  local _, root = next()
  local _, second = next()
  local _, third = next()
  -- NB: grid y coordinate is inverted; increases from top to bottom
  -- FIXME: this seems inefficient, there is many cases of redundent tests
  if (root.local_x == second.local_x) and ((root.local_y - 1) == second.local_y) then
    -- vertical
    if third and (second.local_x == third.local_x) and ((second.local_y - 1) == third.local_y) then
      return self:mk_event(5) -- triple
    else
      return self:mk_event(1) -- double
    end
  elseif ((root.local_x + 1) == second.local_x) and ((root.local_y - 1) == second.local_y) then
    -- diagonal up
    if third and ((second.local_x + 1) == third.local_x) and ((second.local_y - 1) == third.local_y) then
      return self:mk_event(6)
    else
      return self:mk_event(2)
    end
  elseif ((root.local_x + 1) == second.local_x) and (root.local_y == second.local_y) then
    -- horizontal
    if third and ((second.local_x + 1) == third.local_x) and (second.local_y == third.local_y) then
      return self:mk_event(7)
    else
      return self:mk_event(3)
    end
  elseif ((root.local_x + 1) == second.local_x) and ((root.local_y + 1) == second.local_y) then
    -- diagonal down
    if third and ((second.local_x + 1) == third.local_x) and ((second.local_y + 1) == third.local_y) then
      return self:mk_event(8)
    else
      return self:mk_event(4)
    end
  end

  return nil
end

function esShapeGesture:process(event, output, props)
  -- TODO: incorperate timing; optionally ensure match only if the chord occurs
  -- quickly enough
  local h = self._held
  if event.z == 1 then
    h:push_back(event)
    local c = h:count()
    if (c == 2) or (c == 3) then
      --print('do shape search')
      local shape = self:search()
      if shape then
        output(shape)
      end
    end
  else
    h:remove(event, self.match_key_event)
  end
  output(event)
end


--
-- esNoteRender
--
local esNoteRender = sky.Object:extend()

function esNoteRender:new(props)
  esNoteRender.super.new(self, props)
  -- led brightness levels
  self.note_level = props.note_level or 7
  self.key_level = props.key_level or 15
  -- position
  self.root = props.root or ES_DEFAULT_ROOT_NOTE
  self:set_bounds(props.bounds or ES_DEFAULT_BOUNDS)
  -- held grid key state
  self._key_held = {}
  self._note_held = {}
end

function esNoteRender:set_bounds(bounds)
  self.bounds = bounds
  self.width = bounds[3] - (bounds[1] - 1)
  self.height = bounds[4] - (bounds[2] - 1)
  self._mapper = build_note_map(self.root, self.width, self.height)
end

function esNoteRender:render(event, props)
  if sky.is_type(event, sky.GRID_KEY_EVENT) and event.esNoteGesture then
    local k = pack_xy(event.x, event.y)
    if event.z == 1 then self._key_held[k] = true else self._key_held[k] = nil end
    props.grid:led(event.x, event.y, event.z * self.key_level)
  elseif sky.is_note(event) then
    -- whether on or off
    local state = 1
    if event.type == sky.types.NOTE_OFF then state = 0 end

    -- light up all candidate locations
    local locations = self._mapper(event.note)
    if locations then
      for _, l in ipairs(locations) do
        local x, y = self.bounds[1] + l[1], self.height - l[2]
        if self._key_held[pack_xy(x, y)] ~= nil then
          props.grid:led(x, y, self.key_level)
        else
          props.grid:led(x, y, state * self.note_level)
        end
      end
    end
  end
end

--
-- module
--

return {
  esNoteGesture = esNoteGesture,
  esShapeGesture = esShapeGesture,
  esNoteRender = esNoteRender,

  -- events
  ES_SHAPE_EVENT = esShapeGesture.EVENT,
}




