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
function Timer.new(id)
   local t = {}
   t.props = {}
   t.props.id = id
   t.props.time = 1
   t.props.count = -1
   t.props.callback = nil
   t.props.initStage = 1
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
   if stage then self.initStage = stage end
   self.isRunning = true
   timer_start(self.props.id, self.props.time, self.props.count, self.props.initStage) -- C function
end

--- stop a timer
function Timer:stop()
   timer_stop(self.props.id) -- C function
   self.isRunning = false
end

for i=1,Timer.numTimers do
   Timer.timers[i] = Timer.new(i)
end

Timer.__newindex = function(self, idx, val)
   if idx == "time" then
      self.props.time = val
      --[[
	 FIXME: would like to change time of a running timer.
here we restart it when setting the time property; 
problem is that `isRunning` prop is not updated when a finite timer is finished on its own.
(we could track of its stage in lua and unset the flag, or have an additional callback.)

another method would be to add a time setter to the C API (requiring mutex, et al.)

anyway for now, user must explicitly restart.
      --]]
--      if self.isRunning then	 
--	 print ("timer setter calling .start: ", self, idx, val)
      --	 self.start(self, self.props.time, self.props.count, self.props.stage)
      --   end
   elseif idx == 'count' then self.props.count = val
   elseif idx == 'initStage' then self.props.initStage = val
   else -- FIXME: dunno if this is even necessary / a good idea to allow
      rawset(self, idx, val)
   end
   
end

--- class custom .__index; 
-- [] accessor returns one of the static timer objects
Timer.__index = function(self, idx)
   if type(idx) == "number" then
      -- print("class meta: .__index ("..idx..")")
      return Timer.timers[idx]
   elseif idx == "start" then return Timer.start
   elseif idx == "stop" then return Timer.stop
   elseif idx == 'id' then return self.props.id
   elseif idx == 'count' then return self.props.count
   elseif idx == 'time' then return self.props.time
   elseif idx == 'initStage' then return self.props.initStage
      -- hm, why doesn't this work:
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
