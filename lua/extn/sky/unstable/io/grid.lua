local table = require('table')

--
-- GridInput
--
local GridInput = sky.InputBase:extend()
GridInput.GRID_KEY_EVENT = 'GRID_KEY'

function GridInput:new(props)
  GridInput.super.new(self, props)
  self.grid = props.grid or grid.connect()
  if self.grid then
    self.grid.key = function(x, y, s)
      self:on_key_event(x, y, s)
    end
  end
end

function GridInput:on_key_event(x, y, z)
  if self.chain then
    self.chain:process({
      type = GridInput.GRID_KEY_EVENT,
      grid = self.grid,
      x = x,
      y = y,
      z = z,
    })
  end
end

--
-- GridDisplay
--
local GridDisplay = sky.Device:extend()

function GridDisplay:new(props)
  GridDisplay.super.new(self, props)
  self.grid = props.grid or grid.connect()
  for i, child in ipairs(props) do
    self[i] = child
  end
end

function GridDisplay:process(event, output, state)
  local props = { grid = self.grid, bounds = state.bounds }
  for i, child in ipairs(self) do
    props.position = i
    child:render(event, props)
  end
  self.grid:refresh()
  output(event)
end

--
-- GridGestureRegion
--
local GridGestureRegion = sky.Device:extend()

function GridGestureRegion:new(props)
  GridGestureRegion.super.new(self, props)
  self.bounds = props.bounds or {1,1,16,8}
  for i, child in ipairs(self) do
    self[i] = child
  end
end

function GridGestureRegion:in_bounds(key_event)
  return self.bounds[1] <= key_event.x
    and key_event.x <= self.bounds[3]
    and self.bounds[2] <= key_event.y
    and key_event.y <= self.bounds[4]
end

function GridGestureRegion:add_local(key_event)
  key_event.local_x = key_event.x - (self.bounds[1] - 1)
  key_event.local_y = key_event.y - (self.bounds[2] - 1)
  --tab.print(key_event)
  return key_event
end

function GridGestureRegion:process(event, output, state)
  state.bounds = self.bounds
  if sky.is_type(event, GridInput.GRID_KEY_EVENT) and self:in_bounds(event) then
    local props = { region = self }
    local e = self:add_local(event)
    for i, child in ipairs(self) do
      props.position = i
      child:process(e, output, props)
    end
  end
  output(event)
end

return {
  GridInput = GridInput,
  GridDisplay = GridDisplay,
  GridGestureRegion = GridGestureRegion,
  -- exported event types
  GRID_KEY_EVENT = GridInput.GRID_KEY_EVENT,
}




