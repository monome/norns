--- file system
-- @module file

local File = {}

File.state = {}
File.state.out = 0

File.state.resume = function()
  dofile(script_dir .. '../state.lua')
  print("last file loaded: " .. File.state.script)
  sys.script.load()
end

File.state.save = function()
  local fd=io.open(script_dir .. "../state.lua","w+")
  io.output(fd)
  io.write("-- state\n")
  io.write("sys.file.state.script = '" .. File.state.script .. "'\n")
  io.write("sys.file.state.out = '" .. File.state.out .. "'\n")
  io.write("sys.input_left = '" .. sys.input_left .. "'\n")
  io.write("sys.input_right = '" .. sys.input_right .. "'\n")
  io.write("sys.hp = '" .. sys.hp .. "'\n")
  io.close(fd)   
end


--- scan directory, return file list
-- @param directory path to directory
File.scandir = function(directory)
    local i, t, popen = 0, {}, io.popen
    local pfile = popen('ls -p --group-directories-first "'..directory..'"')
    for filename in pfile:lines() do
        i = i + 1
        t[i] = filename
    end
    pfile:close()
    return t
end

--[[
--- get table length
-- @param t table
function File.tablelength(t)
  local c = 0
  for _ in pairs(t) do c = c + 1 end
  return c
end
--]]
return File
