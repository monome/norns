--- Script class
-- @module script

local Script = {}

--- reset script environment;
-- ie redirect draw, key, enc functions, stop timers, clear engine, etc
Script.clear = function()
  print("# script clear")
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
  metro.free_all()
  -- clear polls
  poll.report = norns.none
  -- clear engine
  engine.name = nil
  -- clear init
  init = norns.none
  -- clear last run
  norns.state.script = ''
  norns.state.name = 'none'
  -- clear params
  params:clear()
end

Script.init = function()
  print("# script init")
  params.name = norns.state.name
  if not pcall(init) then norns.scripterror() end
  norns.menu.init()
end

--- load a script from the /scripts folder
-- @param filename (string) - file to load. leave blank to reload current file.
Script.load = function(filename)
  print("# script load")
  if filename == nil then
    filename = norns.state.script end
  local filepath = script_dir .. filename
  local f=io.open(filepath,"r")
  if f==nil then
    print("file not found: "..filepath)
  else
    io.close(f)
    if pcall(cleanup) then print("# cleanup") 
    else print("### cleanup failed") end

    Script.clear() -- clear script variables and functions
    
    local status = norns.try(function() dofile(filepath) end, "load fail") -- do the new script
    if status == true then
      norns.log.post("loaded " .. filename) -- post to log
      norns.state.script = filename -- store script name
      norns.state.name = string.gsub(filename,'.lua','') -- store name
      norns.state.name = norns.state.name:match("[^/]*$") -- strip path from name
      norns.state.save() -- remember this script for next launch
      norns.script.redraw = redraw -- store redraw function for context switching
      redraw = norns.none -- block redraw until Script.init
      Script.run() -- load engine then run script-specified init function
    end
  end
end

--- load engine, execute script-specified init (if present)
Script.run = function()
  print("# script run")
  if engine.name ~= nil then
     print("loading engine; name: " .. engine.name)
     engine.load(engine.name, Script.init) 
  else
     Script.init()
  end
  print("reconnecting grid...")
  grid.reconnect()
end

--- load script metadata
-- @param filename file to load
-- @return meta table with metadata
Script.metadata = function(filename)
  local meta = {}
  if filename == nil then
    filename = norns.state.script end
  local filepath = script_dir .. filename
  local f=io.open(filepath,"r")
  if f==nil then
    print("file not found: "..filepath)
  else
    io.close(f)
    for line in io.lines(filepath) do
      if util.string_starts(line,"--") then
        table.insert(meta, string.sub(line,4,-1)) 
      else return meta end
    end
  end
  return meta
end


return Script
