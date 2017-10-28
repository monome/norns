local Timer = {}

Timer.numTimers = 32
Timer.timers = {}

--- instance metatable for custom setter
local mt = {}
mt.__newindex = function(self, idx, val)
   if idx == 'time' then
      self.time = val
      if self.isRunning then
	 self.start(self, self.time, self.count, self.stage)
      end      
   elseif idx == 'id' then
   -- not allowed to set id
   else
      return rawset(self, idx, value)
   end
end

--- start a timer running
-- @param time - (optional) time period between ticks (seconds.) by default, re-use the last period
-- @param count - (optional) number of ticks. infinite by default
-- @param stage - (optional) initial stage number (1-based.) 1 by default
function mt:start(time, count, stage)
   -- if any of the arguments are missing, use default behaviors for them (described above)
   -- also set those fields to nil (so, use defaults on next call as well)
   local vargs = {}
   if time then
      self.time = time
      vargs[0] = time
   else self.time = nil end
   
   if count then
      self.count = count
      vargs[1] = count
   else self.count = nil end
   
   if stage then
      self.initialStage = stage
      vargs[3] = stage
   else self.stage = nil end
   
   self.isRunning = true
   timer_start(self.id, unpack(vargs))   
end

 function mt:stop()
   timer_stop(self.id)
   self.isRunning = false
end

function Timer.new(id)
   local t = {}
   t.id = id
   t.time = nil
   t.count = nil
   t.callback = nil
   t.initialStage = nil
   setmetatable(t, mt)
   return t
end

for i=1,Timer.numTimers do
   Timer.timers[i] = Timer.new(i)
end

--- class metatble for fetching timers
Timer.__index = function(self, idx)
   if type(idx) == "number" then
      print("class meta: .__index ("..idx..")")
      return Timer.timers[idx]
   else
      return rawget(self, idx)
   end
end

setmetatable(Timer, Timer)

return Timer
