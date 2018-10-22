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
  -- clear, redirect, and reset grids
  for _,dev in pairs(grid.devices) do
    dev:all(0)
    dev:refresh()
    dev.key = norns.none
  end
  grid.cleanup()
  --g = nil
  -- reset gridkey callback
  --gridkey = norns.none
  -- reset midi callbacks
  midi.add = norns.none
  midi.remove = norns.none
  for _,dev in pairs(midi.devices) do
    dev.event = norns.none
  end
  midi.cleanup()
  -- stop all timers
  metro.free_all()
  -- stop all polls and clear callbacks
  poll.clear_all()
  -- clear engine
  engine.name = nil
  free_engine()
  -- clear init
  init = norns.none
  -- clear last run
  norns.state.script = ''
  norns.state.name = 'none'
  -- clear params
  params:clear()
  -- reset PLAY mode screen settings
  local status = norns.menu.status()
  if status == true then s_restore() end
  screen.aa(0)
  screen.level(15)
  screen.line_width(1)
  screen.font_face(0)
  screen.font_size(8)
  if status == true then s_save() end
end

Script.init = function()
  print("# script init")
  params.name = norns.state.name
  init()
  s_save()
  grid.reconnect()
  midi.reconnect()
end

--- load a script from the /scripts folder
-- @param filename (string) - file to load. leave blank to reload current file.
Script.load = function(filename)
  print("# script load")
  if filename == nil then
    filename = norns.state.script end

  -- script local state
  local state = { }

  setmetatable(_G, {
    __index = function (t,k)
      return state[k]
    end,
    __newindex = function(t,k,v)
      state[k] = v
    end,
  })

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
      norns.state.folder_name = string.gsub(filename,'.lua','') -- store name
      norns.state.name = norns.state.folder_name:match("[^/]*$") -- strip path from name
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
    print("loading engine: " .. engine.name)
    engine.load(engine.name, Script.init)
  else
    local status = norns.try(Script.init,"init")
    norns.init_done(status)
  end
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
