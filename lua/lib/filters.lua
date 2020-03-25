------------
--- @module filters
--
-- some filters that could be useful for musical event processing

local f = {}
f.__index = f

-- helper
local function wrap_add(x, a, max)
   local y = x + a
   while y > max do y = y - max end
   return y
end


--- clear a filter's history
function f:clear() 
   for i=1,self.bufsize do self.buf[i]=0 end
end

function f:print_string(pre)
   if pre == nil then pre = '' end
   str = pre
   for i=1,self.bufsize do
      str = str .. self.buf[i] .. ', '
   end
   return str
end

function f:get(i) return buf[i] end

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
   self.pos = wrap_add(self.pos, 1, self.bufsize)
   return self.sum
end

setmetatable(mean, { __index=f })

----------------------
---- @class median: moving, windowed median average filter

local median = {}
median.__index = median

--- constructor
--- @param bufsize: window size, cannot change after creation
function median.new(bufsize)
   local new = setmetatable({}, f.median)
   
   new.buf = {}
   if bufsize==nil then bufsize=16 end
   new.bufsize = bufsize
   new:clear()
   
   return new   
end

--- process a new input value and update the average
-- @param x: new input
function median:next(x)
   -- TODO
end

setmetatable(median, { __index=f })

-----------------
-- hysteresis

-- TODO


-----------------
-- 1-pole lowpass

-- TODO


f.mean = mean
f.median = median

return f
