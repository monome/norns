--- module for creating a lattice of patterns based on a single fast "superclock"
--
-- @module Lattice
-- @release v1.1.0
-- @author tyleretters & ezra & zack

local Lattice, Pattern = {}, {}

--- instantiate a new lattice
-- @tparam[opt] table args optional named attributes are:
-- - "auto" (boolean) turn off "auto" pulses from the norns clock, defaults to true
-- - "meter" (number) of quarter notes per measure, defaults to 4
-- - "ppqn" (number) the number of pulses per quarter note of this superclock, defaults to 96
-- @treturn table a new lattice
function Lattice:new(args)
  local l = setmetatable({}, { __index = Lattice })
  local args = args == nil and {} or args
  l.auto = args.auto == nil and true or args.auto
  l.meter = args.meter == nil and 4 or args.meter
  l.ppqn = args.ppqn == nil and 96 or args.ppqn
  l.enabled = false
  l.transport = 0
  l.superclock_id = nil
  l.pattern_id_counter = 100
  l.patterns = {}
  return l
end

--- start running the lattice
function Lattice:start()
  self.enabled = true
  if self.auto and self.superclock_id == nil then
    self.superclock_id = clock.run(self.auto_pulse, self)
  end
end

--- reset the norns clock and restart lattice
function Lattice:hard_restart()
  -- destroy clock, but not the patterns
  self:stop()
  if self.superclock_id ~= nil then 
    clock.cancel(self.superclock_id)
    self.superclock_id = nil 
  end
  for i, pattern in pairs(self.patterns) do
    self.patterns[i].phase = self.patterns[i].phase_end
  end
  self.transport = 0
  params:set("clock_reset",1)
  self:start()
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
  if self.superclock_id ~= nil then
    clock.cancel(self.superclock_id)
  end
  self.patterns = {}
end

--- set the meter of the lattice
-- @tparam number meter the meter the lattice counts
function Lattice:set_meter(meter)
  self.meter = meter
end

--- use the norns clock to pulse
-- @tparam table s this lattice
function Lattice.auto_pulse(s)
  while true do
    s:pulse()
    clock.sync(1/s.ppqn)
  end
end 

--- advance all patterns in this lattice a single by pulse, call this manually if lattice.auto = false
function Lattice:pulse()
  if self.enabled then
    local ppm = self.ppqn * self.meter
    for id, pattern in pairs(self.patterns) do
      if pattern.enabled then
        pattern.phase = pattern.phase + 1
        if pattern.phase > (pattern.division * ppm) then
          pattern.phase = pattern.phase - (pattern.division * ppm)
          pattern.action(self.transport)
        end
      elseif pattern.flag then
        self.patterns[pattern.id] = nil
      end
    end
    self.transport = self.transport + 1
  end
end

--- factory method to add a new pattern to this lattice
-- @tparam[opt] table args optional named attributes are:
-- - "action" (function) called on each step of this division (lattice.transport is passed as the argument), defaults to a no-op
-- - "division" (number) the division of the pattern, defaults to 1/4
-- - "enabled" (boolean) is this pattern enabled, defaults to true
-- @treturn table a new pattern
function Lattice:new_pattern(args)
  self.pattern_id_counter = self.pattern_id_counter + 1
  local args = args == nil and {} or args
  args.id = self.pattern_id_counter
  args.action = args.action == nil and function(t) return end or args.action
  args.division = args.division == nil and 1/4 or args.division
  args.enabled = args.enabled == nil and true or args.enabled
  args.phase_end = args.division * self.ppqn * self.meter
  local pattern = Pattern:new(args)
  self.patterns[self.pattern_id_counter] = pattern
  return pattern
end

--- "private" method to instantiate a new pattern, only called by Lattice:new_pattern()
-- @treturn table a new pattern
function Pattern:new(args)
  local p = setmetatable({}, { __index = Pattern })
  p.id = args.id
  p.division = args.division
  p.action = args.action
  p.enabled = args.enabled
  p.phase = args.phase_end
  p.phase_end = args.phase_end
  p.flag = false
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

--- flag the pattern to be destroyed
function Pattern:destroy()
  self.enabled = false
  self.flag = true
end

--- set the division of the pattern
-- @tparam number n the division of the pattern
function Pattern:set_division(n)
   self.division = n
end

--- set the action for this pattern
-- @tparam function the action
function Pattern:set_action(fn)
  self.action = fn
end

return Lattice