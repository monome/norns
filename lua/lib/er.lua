--- Euclidean rhythm (http://en.wikipedia.org/wiki/Euclidean_Rhythm)
-- @module er

er = {}
--- gen
-- @tparam number k : number of pulses
-- @tparam number n : total number of steps
-- @treturn table
function er.gen(k, n)
   -- results array, intially all zero
   local r = {}
   for i=1,n do r[i] = false end

   -- using the "bucket method"
   -- for each step in the output, add K to the bucket.
   -- if the bucket overflows, this step contains a pulse.
   local b = n
   for i=1,n do
      if b >= n then
	 b = b - n
	 r[i] = true
      end
      b = b + k
   end
   return r
end

return er
