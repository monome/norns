--- high-resolution metro API;
-- @module metro
-- @alias Metro_mt
require 'norns'
norns.version.metro = '0.0.3'

local Metro = {}
Metro.__index = Metro

Metro.numMetros = 32
Metro.metros = {}

--- constructor;
 -- @param id : identifier (integer)
function Metro.new(id)
   local t = {}
   t.props = {}
   t.props.id = id
   t.props.time = 1
   t.props.count = -1
   t.props.callback = nil
   t.props.initStage = 1
   setmetatable(t, Metro)
   return t
end

--- start a metro
-- @param time - (optional) time period between ticks (seconds.) by default, re-use the last period
-- @param count - (optional) number of ticks. infinite by default
-- @param stage - (optional) initial stage number (1-based.) 1 by default
function Metro:start(time, count, stage)   
   local vargs = {}
   if time then self.props.time = time end
   if count then self.props.count = count end
   if stage then self.initStage = stage end
   self.isRunning = true
   metro_start(self.props.id, self.props.time, self.props.count, self.props.initStage) -- C function
end

--- stop a metro
function Metro:stop()
   metro_stop(self.props.id) -- C function
   self.isRunning = false
end

for i=1,Metro.numMetros do
   Metro.metros[i] = Metro.new(i)
end

Metro.__newindex = function(self, idx, val)
   if idx == "time" then
      self.props.time = val
      --[[
	 FIXME: would like to change time of a running metro.
here we restart it when setting the time property; 
problem is that `isRunning` prop is not updated when a finite metro is finished on its own.
(we could track of its stage in lua and unset the flag, or have an additional callback.)

another method would be to add a time setter to the C API (requiring mutex, et al.)

anyway for now, user must explicitly restart.
      --]]
--      if self.isRunning then	 
--	 print ("metro setter calling .start: ", self, idx, val)
      --	 self.start(self, self.props.time, self.props.count, self.props.stage)
      --   end
   elseif idx == 'count' then self.props.count = val
   elseif idx == 'initStage' then self.props.initStage = val
   else -- FIXME: dunno if this is even necessary / a good idea to allow
      rawset(self, idx, val)
   end
   
end

--- class custom .__index; 
-- [] accessor returns one of the static metro objects
Metro.__index = function(self, idx)
   if type(idx) == "number" then
      -- print("class meta: .__index ("..idx..")")
      return Metro.metros[idx]
   elseif idx == "start" then return Metro.start
   elseif idx == "stop" then return Metro.stop
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

setmetatable(Metro, Metro)

--- Global Functions
-- @section globals

--- callback on metro tick from C;
norns.metro = function(idx, stage)
   if Metro.metros[idx] then
      if Metro.metros[idx].callback then
	 Metro.metros[idx].callback(stage)
      end
   end   
end


return Metro
