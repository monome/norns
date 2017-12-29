--- file system
-- @module file
norns.version.file = '0.0.1'

--- state management
-- @section state

norns.state = {}

norns.state.resume = function()
  dofile(script_dir .. '../state.lua')
  print("last file loaded: " .. norns.state.script)
  norns.script.load()
end

norns.state.save = function()
  local last=io.open(script_dir .. "../state.lua","w+")
  io.output(last)
  io.write("-- state\n")
  io.write("norns.state.script = '" .. norns.state.script .. "'\n")
  io.close(last)   
end


--- script managment
-- @section script

norns.script = {}
norns.script.cleanup = function()
   print("cleanup (default)")
end

cleanup = norns.script.cleanup

norns.script.clear = function()
    -- reset cleanup script
    cleanup = norns.script.cleanup
    -- reset oled redraw
    redraw = norns.blank
    -- redirect inputs to nowhere
    key = norns.none
    enc = norns.none
    -- redirect and reset grid
    if g then g.key = norns.none end
    g = nil
    -- stop all timers
    for i=1,30 do metro[i]:stop() end
end


--- load a script from the /scripts folder
-- @param filename (string) - file to load, no extension. leave blank to reload current file.
norns.script.load = function(filename)
  if filename == nil then
    filename = norns.state.script end
  local filepath = script_dir .. filename .. '.lua'
  local f=io.open(filepath,"r")
  if f==nil then 
    print("file not found: "..filepath)
  else
    io.close(f)
    cleanup() -- cleanup the old script
    norns.script.clear()
    dofile(filepath)
    norns.state.script = filename
    norns.state.save()
    norns.map.init()
  end 
end

--- general file access
-- @section general

--- scan directory, return file list
-- @param directory path to directory
scandir = function(directory)
    local i, t, popen = 0, {}, io.popen
    local pfile = popen('ls "'..directory..'"')
    for filename in pfile:lines() do
        i = i + 1
        t[i] = filename
    end
    pfile:close()
    return t
end

--- get table length
-- @param t table
function tablelength(t)
  local c = 0
  for _ in pairs(t) do c = c + 1 end
  return c
end
