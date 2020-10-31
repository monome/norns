--- Euclidean rhythm (http://en.wikipedia.org/wiki/Euclidean_Rhythm)
-- @module er

er = {}
--- gen
-- @tparam number k : number of pulses
-- @tparam number n : total number of steps
-- @tparam number w : shift amount
-- @treturn table
function er.gen(k, n, w)
   w = w or 0
   -- results array, intially all zero
   local r = {}
   for i=1,n do r[i] = false end

   if k<1 then return r end

   -- using the "bucket method"
   -- for each step in the output, add K to the bucket.
   -- if the bucket overflows, this step contains a pulse.
   local b = n
   for i=1,n do
      if b >= n then
         b = b - n
         local j = i + w
         while (j > n) do j = j - n end
         while (j < 1) do j = j + n end
         r[j] = true
      end
      b = b + k
   end
   return r
end

return er
