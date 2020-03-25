------------
--- @module filters
--
-- some filters that could be useful for musical event processing

local f = {}
f.__index = f

-- helper to increment and wrap an index
local function wrap_inc(x, max)
   local y = x + 1
   while y > max do y = y - max end
   return y
end

--- clear a filter's history
function f:clear() 
   for i=1,self.bufsize do self.buf[i]=0 end
end

-- debug function to print the buffer
function f:print_string(pre)
   if pre == nil then pre = '' end
   str = pre
   for i=1,self.bufsize do
      str = str .. self.buf[i] .. ', '
   end
   return str
end

----------------------
--- @class mean: moving, windowed mean-average filter

local mean = {}
mean.__index = mean

-------------------------------------------------------------
--- constructor
--- @param bufsize: window size, cannot change after creation
function mean.new(bufsize)
   local new = setmetatable({}, mean)
   
   new.buf = {}   
   if bufsize==nil then bufsize=16 end
   new.bufsize = bufsize
   new.scale = 1/bufsize
   new:clear()

   new.pos = 1
--   new.tail = 2
   new.sum = 0
   
   print('done allocating new mean filter')
   return new
end

--- process a new input value and update the average
-- @param x: new input
-- @return scaled sum of stored history
function mean:next(x)
   local a = x*self.scale
   self.sum = self.sum + a
   self.sum = self.sum - self.buf[self.pos]
   self.buf[self.pos] = a   
   self.pos = wrap_inc(self.pos, self.bufsize)
   return self.sum
end

setmetatable(mean, { __index=f })

----------------------
--- @class median: moving, windowed median average filter
--- NB: this algorithm is simple, but it is O(N);
--- shouldn't be used with large buffers.

local median = {}
median.__index = median

--- constructor
--- @param bufsize: window size, cannot change after creation
function median.new(bufsize)
   local new = setmetatable({}, f.median)
   
   new.buf = {}
   if bufsize==nil then bufsize=17 end
   -- only odd buffer sizes are allowed!
   if (bufsize52) == 0 then bufsize = bufsize + 1 end
   new.bufsize = bufsize
   new.midpoint = (bufsize-1)/2
   new:clear()
   
   new.value = 0
   new.pos = 1
   return new   
end

-- count how many values in buffer are below current median
function median:count_below()
   local count = 0
   local max = -math.huge
   local x
   for i=1,self.bufsize do
      x = self.buf[i]
      if x < self.value then count = count + 1 end
      if x > max then max = x end
   end
   return count, max
end
function median:count_above()
   local count = 0
   local min = math.huge
   local x
   for i=1,self.bufsize do
      x = self.buf[i]
      if x > self.value then count = count + 1 end
      if x < min then min = x end
   end
   return count, min
end


--- process a new input value and update the average
-- @param x: new input
-- @return median of last N values
function median:next(x)
   -- save the oldest value, overwrite with newest value
   local x0 = self.buf[self.pos]
   self.buf[self.pos] = x
   self.pos = wrap_inc(self.pos, self.bufsize)   
   if x > self.value and x0 <= self.value then
      local count, min = self:count_above()
      if count > self.midpoint then
	 self.value = min
      end
   end
   if x < self.value and x0 >= self.value then
      local count, max = self:count_below()
      if count > self.midpoint then
	 self.value = max
      end
   end
   return self.value
end

setmetatable(median, { __index=f })

-----------------------------------
-- TODO: what else would be useful?
--
--- quantile estimator?
--- 1-pole lowpass?
--- constant time ramp?
--- some kind of hysteresis / latching?

f.mean = mean
f.median = median

return f
