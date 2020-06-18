--
-- An adaptation of the linn library by...
--
--   ////\\\\
--   ////\\\\  linn
--   ////\\\\  BY NEAUOIRE
--   \\\\////
--   \\\\////  linn LAYOUT
--   \\\\////
--
--
sky.use('io/grid')

local keys = { 'C','C#','D','D#','E','F','F#','G','G#','A','A#','B' }

local notes = {
  'F4', 'F4#', 'G4', 'G4#', 'A4', 'A4#', 'B4', 'C5', 'C5#', 'D5', 'D5#', 'E5', 'F5', 'F5#', 'G5', 'G5#',
  'C4', 'C4#', 'D4', 'D4#', 'E4', 'F4', 'F4#', 'G4', 'G4#', 'A4', 'A4#', 'B4', 'C5', 'C5#', 'D5', 'D5#',
  'G3', 'G3#', 'A3', 'A3#', 'B3', 'C4', 'C4#', 'D4', 'D4#', 'E4', 'F4', 'F4#', 'G4', 'G4#', 'A4', 'A4#',
  'D3', 'D3#', 'E3', 'F3', 'F3#', 'G3', 'G3#', 'A3', 'A3#', 'B3', 'C4', 'C4#', 'D4', 'D4#', 'E4', 'F4',
  'A2', 'A2#', 'B2', 'C3', 'C3#', 'D3', 'D3#', 'E3', 'F3', 'F3#', 'G3', 'G3#', 'A3', 'A3#', 'B3', 'C4',
  'E2', 'F2', 'F2#', 'G2', 'G2#', 'A2', 'A2#', 'B2', 'C3', 'C3#', 'D3', 'D3#', 'E3', 'F3', 'F3#', 'G3',
  'B1', 'C2', 'C2#', 'D2', 'D2#', 'E2', 'F2', 'F2#', 'G2', 'G2#', 'A2', 'A2#', 'B2', 'C3', 'C3#', 'D3',
  'F1#', 'G1', 'G1#', 'A1', 'A1#', 'B1', 'C2', 'C2#', 'D2', 'D2#', 'E2', 'F2', 'F2#', 'G2', 'G2#', 'A2',
}

local function index_of(list, value)
  for i = 1, #list do
    if list[i] == value then return i end
  end
  return -1
end

local function note_at(i)
  local n = notes[i]
  local k = n:sub(1, 1)
  local o = tonumber(n:sub(2, 2))
  local s = n:match('#')
  local p = n:gsub(o,'')
  local v = index_of(keys, p) + (12 * (o + 2)) - 1
  local l = 0

  if p == 'C' then
    l = 15
  elseif s then
    l = 2
  else
    l = 6
  end

  return { i = i, k = k, o = o, s = s, v = v, l = l, p = p }
end

local function pos_at(id)
  return { x = ((id-1) % 16) + 1, y = math.floor(id / 16) + 1 }
end

local function id_at(x, y)
  return ((y-1) * 16) + x
end

--
-- linnGesture
--

local linnGesture = sky.Device:extend()

function linnGesture:new(props)
  linnGesture.super.new(self, props)
  self.ch = props.ch or 1
  self.vel = props.vel or 127
  self.focus = { x = 0, y = 0 }
end

function linnGesture:process(event, output, props)
  event.linnGesture = self
  local note = note_at(id_at(event.x, event.y))
  if event.z == 1 then
    self.focus.x = event.x
    self.focus.y = event.y
    output(sky.mk_note_on(note.v, self.vel, self.ch))
  else
    self.focus.x = 0
    self.focus.y = 0
    output(sky.mk_note_off(note.v, self.vel, self.ch))
  end
end

--
-- linnRender
--

local linnRender = sky.Object:extend()

function linnRender:new(props)
  linnRender.super.new(self, props)
  self.key_level = props.key_level or 15
end

function linnRender:render(event, props)
  if sky.is_type(event, sky.GRID_KEY_EVENT) and event.linnGesture then
    if event.z == 1 then
      props.grid:led(event.x, event.y, self.key_level)
    else
      local note = note_at(id_at(event.x, event.y))
      props.grid:led(event.x, event.y, note.l)
    end
  elseif sky.is_type(event, sky.SCRIPT_REDRAW_EVENT) then
    for i = 1, 128 do
      local pos = pos_at(i)
      local note = note_at(i)
      props.grid:led(pos.x, pos.y, note.l)
    end
  end
end

--
-- module
--

return {
  linnGesture = linnGesture,
  linnRender = linnRender,
}