--- Script class
-- @classmod script
-- @alias Script

local Script = {}

--- reset script environment.
-- ie redirect draw, key, enc functions, stop timers, clear engine, etc.
Script.clear = function()
  print("# script clear")

  -- script local state
  local state = { }

  setmetatable(_G, {
    __index = function (_,k)
      return state[k]
    end,
    __newindex = function(_,k,v)
      state[k] = v
    end,
  })

  -- reset cleanup script
  cleanup = norns.none

  -- reset oled redraw
  redraw = norns.blank

  -- redirect inputs to nowhere
  key = norns.none
  enc = norns.none

  -- reset encoders
  norns.enc.accel(0,true)
  norns.enc.sens(0,1)

  -- clear, redirect, and reset devices
  grid.cleanup()
  arc.cleanup()
  midi.cleanup()
  hid.cleanup()

  -- stop all timers
  metro.free_all()

  -- stop clock
  clock.cleanup()

  -- stop all polls and clear callbacks
  poll.clear_all()

  -- clear engine
  engine.name = nil

  -- clear softcut
  softcut.reset()

  -- clear init
  init = norns.none

  -- clear crow functions
  crow.init()

  -- clear last run
  norns.state.script = ''
  norns.state.name = 'none'
  norns.state.shortname = 'none'
  norns.state.path = _path["dust"]

  -- clear params
  params:clear()
  norns.pmap.clear()
  -- add audio
  audio.add_params()

  -- reset PLAY mode screen settings
  local status = norns.menu.status()
  if status == true then _norns.screen_restore() end

  screen.aa(0)
  screen.level(15)
  screen.line_width(1)
  screen.font_face(1)
  screen.font_size(8)

  if status == true then _norns.screen_save() end

  -- ensure finalizers run before next script
  collectgarbage()
end

Script.init = function()
  print("# script init")
  params.name = norns.state.shortname
  init()
  _norns.screen_save()
end

--- load a script from the /scripts folder.
-- @tparam string filename file to load. leave blank to reload current file.
Script.load = function(filename)
  local name, path, relative
  if filename == nil then
    filename = norns.state.script
    name = norns.state.name
    path = norns.state.path
  else
	if string.sub(filename,1,1) == "/" then
	  relative = string.sub(filename,string.len(_path["dust"]))
	else
	  relative = filename
	  filename = _path["dust"] .. filename
	end

	local t = tab.split(string.sub(relative,0,-5),"/")
	if t[#t] == t[#t-1] then
	  name = t[#t]
	else
	  name = t[#t-1].."/"..t[#t]
	end
  if #t==4 then name = t[2].."/"..name end -- dumb hack for 3-deep subfolers
	path = string.sub(_path["dust"],0,-2)
	for i = 1,#t-1 do path = path .. "/" .. t[i] end
	--print("name "..name)
	--print("final path "..path)
  end

  print("# script load: " .. filename)

  local f=io.open(filename,"r")
  if f==nil then
    print("file not found: "..filename)
  else
    io.close(f)
    local ok, err
    ok, err = pcall(cleanup)
    if ok then print("# cleanup")
    else
      print("### cleanup failed with error: "..err)
    end
    
    -- unload asl package entry so `require 'asl'` works
    -- todo(pq): why is this not needed generally (e.g., for 'ui', 'util', etc.)?
    if package.loaded['asl'] ~= nil then
      package.loaded['asl'] = nil
    end 

    Script.clear() -- clear script variables and functions

    norns.state.script = filename
    norns.state.name = name
    norns.state.shortname = norns.state.name:match( "([^/]+)$" )
    norns.state.path = path .. '/'
    norns.state.data = _path.data .. name .. '/'

    if util.file_exists(norns.state.data) == false then
      print("### initializing data folder")
      util.make_dir(norns.state.data)
    end

    local status = norns.try(function() dofile(filename) end, "load fail") -- do the new script
    if status == true then
      norns.state.save() -- remember this script for next launch
      norns.script.nointerface = redraw == norns.blank -- check if redraw is present
      norns.script.redraw = redraw -- store redraw function for context switching
      redraw = norns.none -- block redraw until Script.init
      Script.run() -- load engine then run script-specified init function
    else
      Script.clear()
    end
  end
end

--- load engine, execute script-specified init (if present).
Script.run = function()
  print("# script run")
  if engine.name ~= nil then
    print("loading engine: " .. engine.name)
    engine.load(engine.name, Script.init)
  else
    engine.load("None", Script.init)
  end
  norns.pmap.read() -- load parameter map
end

--- load script metadata.
-- @tparam string filename file to load
-- @treturn table meta table with metadata
Script.metadata = function(filename)
  local meta = {}
  local f=io.open(filename,"r")
  if f==nil then
    print("file not found: "..filename)
  else
    io.close(f)
    for line in io.lines(filename) do
      if util.string_starts(line,"--") then
        table.insert(meta, string.sub(line,4,-1))
      else
        if #meta == 0 then
          table.insert(meta, "no script information")
        end
        return meta
      end
    end
  end
  return meta
end


return Script
