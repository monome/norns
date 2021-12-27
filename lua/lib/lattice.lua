--- module for creating a lattice of patterns based on a single fast "superclock"
--
-- @module Lattice
-- @release v1.2.1
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
  l.pattern_ordering={}
  return l
end

--- start running the lattice
function Lattice:start()
  self.enabled = true
  if self.auto and self.superclock_id == nil then
    self.superclock_id = clock.run(self.auto_pulse, self)
  end
end

--- reset the norns clock without restarting lattice
function Lattice:reset()
  -- destroy clock, but not the patterns
  self:stop()
  if self.superclock_id ~= nil then 
    clock.cancel(self.superclock_id)
    self.superclock_id = nil 
  end
  for i, pattern in pairs(self.patterns) do
    pattern.phase = pattern.division * self.ppqn * self.meter * (1-pattern.delay)
    pattern.downbeat = false
  end
  self.transport = 0
  params:set("clock_reset",1)
end

--- reset the norns clock and restart lattice
function Lattice:hard_restart()
  self:reset()
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
  self.pattern_ordering={}
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
    local flagged=false
    for _, id in ipairs(self.pattern_ordering) do
      local pattern=self.patterns[id]
      if pattern.enabled then
        pattern.phase = pattern.phase + 1
        local swing_val = (2*pattern.swing/100)
        if not pattern.downbeat then 
          swing_val = (2*(100-pattern.swing)/100)
        end
        if pattern.phase > (pattern.division * ppm)*swing_val then
          pattern.phase = pattern.phase - (pattern.division * ppm)
          if pattern.delay_new ~= nil then
            pattern.phase = pattern.phase - (pattern.division*ppm)*(1-(pattern.delay-pattern.delay_new))
            pattern.delay = pattern.delay_new
            pattern.delay_new = nil
          end
          pattern.action(self.transport)
          pattern.downbeat = not pattern.downbeat
        end
      elseif pattern.flag then
        self.patterns[pattern.id] = nil
	flagged=true
      end
    end
    if flagged then
       self:order_patterns()
    end
    self.transport = self.transport + 1
  end
end

--- factory method to add a new pattern to this lattice
-- @tparam[opt] table args optional named attributes are:
-- - "action" (function) called on each step of this division (lattice.transport is passed as the argument), defaults to a no-op
-- - "division" (number) the division of the pattern, defaults to 1/4
-- - "enabled" (boolean) is this pattern enabled, defaults to true
-- - "swing" (number) is the percentage of swing (0 - 100%), defaults to 50
-- - "delay" (number) specifies amount of delay, as fraction of division (0.0 - 1.0), defaults to 0
-- @treturn table a new pattern
function Lattice:new_pattern(args)
  self.pattern_id_counter = self.pattern_id_counter + 1
  local args = args == nil and {} or args
  args.id = self.pattern_id_counter
  args.action = args.action == nil and function(t) return end or args.action
  args.division = args.division == nil and 1/4 or args.division
  args.enabled = args.enabled == nil and true or args.enabled
  args.phase = args.division * self.ppqn * self.meter
  args.swing = args.swing == nil and 50 or args.swing
  args.delay = args.delay == nil and 0 or args.delay
  local pattern = Pattern:new(args)
  self.patterns[self.pattern_id_counter] = pattern
  self:order_patterns()
  return pattern
end

-- "private" method to keep numerical order of the pattern ids
-- for use when pulsing
function Lattice:order_patterns()
  self.pattern_ordering={}
  for id, pattern in pairs(self.patterns) do
	table.insert(self.pattern_ordering,id)
  end
  table.sort(self.pattern_ordering)
end

--- "private" method to instantiate a new pattern, only called by Lattice:new_pattern()
-- @treturn table a new pattern
function Pattern:new(args)
  local p = setmetatable({}, { __index = Pattern })
  p.id = args.id
  p.division = args.division
  p.action = args.action
  p.enabled = args.enabled
  p.flag = false
  p.swing = args.swing
  p.downbeat = false
  p.delay = args.delay
  p.phase = args.phase * (1-args.delay)
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

--- set the swing of the pattern
-- @tparam number the swing value 0-100%
function Pattern:set_swing(swing)
  self.swing=util.clamp(swing,0,100)
end

-- set the delay for this pattern
-- @tparam fraction of the time between beats to delay (0-1)
function Pattern:set_delay(delay)
  self.delay_new = util.clamp(delay,0,1)
end

return Lattice
