--- Script class
-- @module script

local Script = {}

--- reset script environment;
-- ie redirect draw, key, enc functions, stop timers, clear engine, etc
Script.clear = function()
    -- reset cleanup script
    cleanup = sys.none
    -- reset oled redraw
    redraw = sys.blank
    -- redirect inputs to nowhere
    key = sys.none
    enc = sys.none
    -- redirect and reset grid
    if g then g.key = sys.none end
    g = nil
    -- stop all timers
    for i=1,30 do metro[i]:stop() end
    -- clear polls
    poll.report = sys.none
    -- clear engine 
    engine = nil
    -- clear init
    init = sys.none
end


--- load a script from the /scripts folder
-- @param filename (string) - file to load. leave blank to reload current file.
Script.load = function(filename)
  if filename == nil then
    filename = sys.file.state.script end
  local filepath = script_dir .. filename
  local f=io.open(filepath,"r")
  if f==nil then 
    print("file not found: "..filepath)
  else
    io.close(f)
    cleanup() -- script-specified memory free
    Script.clear() -- clear script variables and functions
    sys.log.post("loaded " .. filename) -- post to log
    dofile(filepath) -- do the new script
    sys.file.state.script = filename -- store script name
    sys.file.state.save() -- remember this script for next launch
    sys.menu.init() -- redirect i/o functions to script
    Script.run() -- load engine then run script-specified init function
  end 
end

--- load engine, execute script-specified init (if present)
Script.run = function()
    if engine ~= nil then 
        e.load(engine, init)
    else
        init()
    end
    grid.reconnect()
end

--- load script metadata
-- @param filename file to load
-- @return meta table with metadata
Script.metadata = function(filename)
  local meta = {}
  if filename == nil then
    filename = sys.file.state.script end
  local filepath = script_dir .. filename
  local f=io.open(filepath,"r")
  if f==nil then 
    print("file not found: "..filepath)
  else
    io.close(f)
    for line in io.lines(filepath) do
        if string.starts(line,"--") then
			-- FIXME: this should be smart and be able to extract any @thing
			if string.starts(line,"-- @name ") then
            	meta.name = string.sub(line,string.len("-- @name ")+1,-1)
			elseif string.starts(line,"-- @version ") then
            	meta.version = string.sub(line,string.len("-- @version ")+1,-1)
			elseif string.starts(line,"-- @author ") then
            	meta.author = string.sub(line,string.len("-- @author ")+1,-1)
			elseif string.starts(line,"-- @url ") then
            	meta.url = string.sub(line,string.len("-- @url ")+1,-1)
			elseif string.starts(line,"-- @txt ") then
            	meta.txt = string.sub(line,string.len("-- @txt ")+1,-1)
			end
        else return meta end 
    end
  end 
  return meta
end



return Script
