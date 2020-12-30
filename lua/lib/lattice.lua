--- module for creating a lattice of patterns based on a single fast "superclock"
--
-- @module Lattice
-- @release v1.0.0
-- @author tyleretters & ezra

local Lattice, Pattern = {}, {}

--- instantiate a new lattice
-- @tparam[opt] table args optional named attributes are:
-- - "meter" (number) of quarter notes per measure, defaults to 4
-- - "ppqn" (number) the number of pulses per quarter note of this superclock, defaults to 96
-- - "callback" (function) callback to be called on the downbeat
-- @treturn table a new lattice
function Lattice:new(args)
  local l = setmetatable({}, { __index = Lattice })
  local args = args == nil and {} or args
  local defaults = {
    meter = 4,
    ppqn = 96,
    callback = false
  }
  for k, v in pairs(defaults) do
    l[k] = args[k] == nil and v or args[k]
  end
  l.enabled = false
  l.transport = 0
  l.superclock_id = nil
  l.patterns = {}
  return l
end

--- start running the lattice
function Lattice:start()
  if self.superclock_id == nil then
    self.superclock_id = clock.run(self.pulse, self)
  end
  self.enabled = true
end

--- stop the lattice
function Lattice:stop()
  self.enabled = false
end

--- toggle the lattice
function Lattice:toggle()
  self.enabled = not self.enabled
end

--- destroy the lattice
function Lattice:destroy()
  self:stop()
  clock.cancel(self.superclock_id)
  self.patterns = {}
end

--- destroy a pattern
-- @tparam table a pattern from this lattice
function Lattice:destroy_pattern(pattern)
  self.patterns[pattern.id] = nil
end

--- set the meter of the lattice
-- @tparam number meter the meter the lattice counts
function Lattice:set_meter(meter)
  self.meter = meter
end

--- set the ppqn of the lattice
-- @tparam number ppqn the pulses per quarter note
function Lattice:set_ppqn(ppqn)
  self.ppqn = ppqn
end

--- set the callback of the downbeat
-- @tparam function the callback
function Lattice:set_callback(fn)
  self.callback = fn
end

--- advance all patterns in this lattice a single by pulse
-- @tparam table s this lattice
function Lattice.pulse(s)
  while true do
    clock.sync(1/s.ppqn)
    if s.enabled then
      s.transport = s.transport + 1
      if type(s.callback) == "function" and s.transport % (s.ppqn * s.meter) == 1 then
        s.callback(s.transport)
      end
      for id, pattern in pairs(s.patterns) do
        if pattern.enabled then
          pattern.phase = pattern.phase + 1
          if pattern.phase > (pattern.division * s.ppqn * s.meter) then
            pattern.phase = pattern.phase - (pattern.division * s.ppqn * s.meter)
            if type(pattern.callback) == "function" then
              pattern.callback()
            end
          end
        end
      end
    end
  end
end 

--- factory method to add a new pattern to this lattice
-- @tparam[opt] table args optional named attributes are:
-- - "division" (number) the division of the pattern
-- - "callback" (function) function called on each step of this division
-- - "enabled" (boolean) is this pattern enabled
-- @treturn table a new pattern
function Lattice:new_pattern(args)
  local args = args == nil and {} or args
  local defaults = {
    division = 1/4,
    callback = false,
    enabled = true
  }
  for k, v in pairs(defaults) do
    args[k] = args[k] == nil and v or args[k]
  end
  local id = #self.patterns + 1
  args.id = id
  local pattern = Pattern:new(args)
  self.patterns[id] = pattern
  return pattern
end

--- "private" method to instantiate a new pattern, only called by Lattice:new_pattern()
-- @treturn table a new pattern
function Pattern:new(args)
  local p = setmetatable({}, { __index = Pattern })
  p.id = args.id
  p.division = args.division
  p.callback = args.callback
  p.enabled = args.enabled
  p.phase = 0
  return p
end

--- start the pattern
function Pattern:start()
  self.enabled = true
end

--- stop the pattern
function Pattern:stop()
  self.enabled = false
end

--- toggle the pattern
function Pattern:toggle()
  self.enabled = not self.enabled
end

--- set the division of the pattern
-- @tparam number n the division of the pattern
function Pattern:set_division(n)
   self.division = n
end

--- set the callback for this pattern
-- @tparam function the callback
function Pattern:set_callback(fn)
  self.callback = fn
end

return Lattice