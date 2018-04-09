--- Paramset class
-- @module paramset

local Paramset = {}

--- print
-- @param p table to print
Paramset.print = function(p)
  for k,v in pairs(p) do
    print(k.." = "..v:get())
  end
end

--- write to disk
-- @param p table
-- @param filename relative to data_dir
Paramset.write = function(p, filename) 
  local fd=io.open(data_dir .. filename,"w+")
  io.output(fd)
  for k,v in pairs(p) do
    io.write(k..","..v:get().."\n")
    --print(k..","..v:get())
  end
  io.close(fd)
end

--- read from disk
-- @param p table
-- @param filename relative to data_dir
Paramset.read = function(p, filename)
  local fd=io.open(data_dir .. filename,"r")
  if fd then
    io.close(fd)
    for line in io.lines(data_dir .. filename) do
      --print(line)
      k,v = line:match("([^,]+),([^,]+)")
      p[k]:set(tonumber(v))
    end 
  end 
end

--- bang all params
-- @param p table
Paramset.bang = function(p)
  for k,v in pairs(p) do
    v:bang()
  end
end 

return Paramset
