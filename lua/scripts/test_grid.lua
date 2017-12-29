print('test_grid.lua')

g = nil -- grid device
ledVal = 9 -- level of LED output

gridkey = function(x, y, state)
   print("key ", x, y, state)
   assert(g) -- its an error to receive callback if we have no device
   if state > 0 then 
      g:led(x, y, ledVal)
   else
      g:led(x, y, 0)
   end
   g:refresh()
end 
