--- high-resolution timer API;
-- @module timer
-- @alias Timer_mt

print('timer.lua')
require 'norns'
norns.version.timer = '0.0.3'

local Timer = {}
Timer.__index = Timer

Timer.numTimers = 32
Timer.timers = {}

--- constructor;
 -- @param id : identifier (integer)
function Timer.new(props)
   local t = {}
   if props then
      if props.id then
	 t.props = props
      end
   else
      t.props = {}
   end
   t.props.time = nil
   t.props.count = nil
   t.props.callback = nil
   t.props.initialStage = nil
   setmetatable(t, Timer)
   return t
end

--- start a timer
-- @param time - (optional) time period between ticks (seconds.) by default, re-use the last period
-- @param count - (optional) number of ticks. infinite by default
-- @param stage - (optional) initial stage number (1-based.) 1 by default
function Timer:start(time, count, stage)   
   local vargs = {}
   if time then self.props.time = time end
   if count then self.props.count = count end
   if stage then self.initialStage = stage end
   self.isRunning = true
   for k,v in pairs(vargs) do print(k,v) end
   -- if any arguments are nil, C glue should use default behaviors
   timer_start(self.id, time, count, stage) -- C function
end

--- stop a timer
function Timer:stop()
   timer_stop(self.id) -- C function
   self.isRunning = false
end

for i=1,Timer.numTimers do
   Timer.timers[i] = Timer.new(i)
end

Timer.__newindex = function(self, idx, val)
   if idx == "time" then
      self.props.time = val
      if self.isRunning then	 
	 print ("timer setter calling .start: ", self, idx, val)
	 self.start(self, self.time, self.count, self.stage)
      end
   elseif props.idx
      -- no other property setters are allowed until C glue supports them
   -- use Timer:start() explicitly to restart with different configuration
   else -- FIXME: dunno if this is even necessary / a good idea to allow
      self.rawset(self, idx, val)
   end
   
end

--- class custom .__index; 
-- [] accessor returns one of the static timer objects
Timer.__index = function(self, idx)
   if type(idx) == "number" then
      -- print("class meta: .__index ("..idx..")")
      return Timer.timers[idx]
   elseif self.props.idx then
      return self.props.idx
   else
      return rawget(self, idx)
   end
end

setmetatable(Timer, Timer)

--- Global Functions
-- @section globals

--- callback on timer tick from C;
norns.timer = function(idx, stage)
   if Timer.timers[idx] then
      if Timer.timers[idx].callback then
	 Timer.timers[idx].callback(stage)
      end
   end   
end


return Timer
