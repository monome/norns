--- elementary cellular automata
-- @classmod elca
-- @alias CA

local CA = {}

CA.__index = CA

CA.NUM_STATES = 128
CA.BOUND_LOW = 0
CA.BOUND_HIGH = 1
CA.BOUND_WRAP = 2

--- constructor
-- @treturn elca
function CA.new()
   local ca = setmetatable({}, CA)
   
    ca.state = {}
    for i=1,CA.NUM_STATES do
       ca.state[i] = 0
    end
    
    ca.bound_mode_l = CA.BOUND_LOW
    ca.bound_mode_r = CA.BOUND_LOW
    ca.bound_l = 1
    ca.bound_r = 128
    
    ca.rule = 34
    ca.offset = 0
    return ca
end 

--- update all state
function CA:update() 
   local newState = {}
   for i=1,CA.NUM_STATES do
      local l, c, r = self:neighbors(i)
      local code = CA.code(l, c, r)
      if (self.rule & (1 << code)) > 0 then newState[i] = 1
      else newState[i] = 0 end
   end
   self.state = newState
end

--- helper: return three values to use for neighbor code
-- @tparam number i
-- @return tuple of left, right, center
function CA:neighbors(i)
   local l
   local r

   if i <= self.bound_l then
      if self.bound_mode_l == CA.BOUND_LOW then l = 0
      elseif self.bound_mode_l == CA.BOUND_HIGH then l = 1 
      else l = self.state[self.bound_r] end
   else
      l = self.state[i-1]
   end
   
   -- right neighbor
   if i >= CA.NUM_STATES then
      if self.bound_mode_r == CA.BOUND_LOW then r = 0
      elseif self.bound_mode_r == CA.BOUND_HIGH then r = 1 
      else r = self.state[self.bound_l] end
   else
      r = self.state[i+1]
   end
   
--   print("get_neighbors ", i, ":", l, self.state[i], r)
   return l, self.state[i], r
end

--- get binary code for value given cell and neighbors
-- @tparam number l left cell
-- @tparam number c center cell
-- @tparam number r right cell
function CA.code(l, c, r)
   local n = 0
   if l > 0 then n = n | 4 end
   if c > 0 then n = n | 2 end   
   if r > 0 then n = n | 1 end
   return n
end

--- return 8 cells at current offset
-- @tparam number n
-- @treturn table table with 8 binary values
function CA:window(n)
   if n == nil then n = CA.NUM_STATES end
   local w = {}
   for i=1,n do
      w[i] = self.state[i + self.offset]
   end
   return w
end 

--- change current state at index,
--- and update the rule to that which would have produced the new state
function CA:set_rule_by_state(val, l, c, r)
   local code = CA.code(l, c, r)
   print("code: ", code)
   if val > 0 then self.rule = self.rule | (1<<code)
   else self.rule = self.rule & (~(1<<code)) end
end

--- clear all states
function CA:clear()
   for i=1,CA.NUM_STATES do self.state[i] = 0 end
end

-- TODO: maybe setter methods that clamp stuff

return CA
