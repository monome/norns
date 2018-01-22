--- file system
-- @module file
norns.version.file = '0.0.1'

--- state management
-- @section state

norns.state = {}
norns.state.out = 0

norns.state.resume = function()
  dofile(script_dir .. '../state.lua')
  print("last file loaded: " .. norns.state.script)
  norns.script.load()
end

norns.state.save = function()
  local fd=io.open(script_dir .. "../state.lua","w+")
  io.output(fd)
  io.write("-- state\n")
  io.write("norns.state.script = '" .. norns.state.script .. "'\n")
  io.write("norns.state.out = '" .. norns.state.out .. "'\n")
  io.close(fd)   
end


--- script managment
-- @section script

norns.script = {}

cleanup = norns.none

norns.script.clear = function()
    -- reset cleanup script
    cleanup = norns.none
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
    -- clear polls
    poll.report = norns.none
    -- clear engine 
    engine = nil
    -- clear init
    init = norns.none
end


--- load a script from the /scripts folder
-- @param filename (string) - file to load, no extension. leave blank to reload current file.
norns.script.load = function(filename)
  if filename == nil then
    filename = norns.state.script end
  local filepath = script_dir .. filename
  local f=io.open(filepath,"r")
  if f==nil then 
    print("file not found: "..filepath)
  else
    io.close(f)
    cleanup() -- script-specified memory free
    norns.script.clear() -- clear script variables and functions
    dofile(filepath) -- do the new script
    norns.state.script = filename -- store script name
    norns.state.save() -- remember this script for next launch
    norns.map.init() -- redirect i/o functions to script
    norns.script.run() -- load engine then run script-specified init function
  end 
end

--- load engine, execute script-specified init (if present)
norns.script.run = function()
    if engine ~= nil then 
        e.load(engine, init)
    else
        init()
    end
    grid.reconnect()
end

--- general file access
-- @section general

--- scan directory, return file list
-- @param directory path to directory
scandir = function(directory)
    local i, t, popen = 0, {}, io.popen
    local pfile = popen('ls -p --group-directories-first "'..directory..'"')
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
