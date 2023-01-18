--- Script class
-- @module script

local hook = require 'core/hook'

local Script = {}

--- reset script environment.
-- ie redirect draw, key, enc functions, stop timers, clear engine, etc.
Script.clear = function()
  print("# script clear")

  if cleanup ~= nil then
    local ok, err
    ok, err = pcall(cleanup)
    if not ok then
      print("### cleanup failed with error: "..err)
    end
  end

  -- allow mods to do cleanup
  hook.script_post_cleanup()

  -- unload asl package entry so `require 'asl'` works
  -- todo(pq): why is this not needed generally (e.g., for 'ui', 'util', etc.)?
  if package.loaded['asl'] ~= nil then
    package.loaded['asl'] = nil
  end

  if norns.lfo ~= nil then
    norns.lfo.lattice:destroy()
    norns.lfo = nil
  end

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
  norns.enc.sens(0,2)

  -- clear, redirect, and reset devices
  grid.cleanup()
  arc.cleanup()
  midi.cleanup()
  hid.cleanup()
  osc.cleanup()

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
  norns.crow.init()

  -- clear HID device handlers
  keyboard.clear()
  gamepad.clear()

  -- clear last run
  norns.state.script = ''
  norns.state.name = 'none'
  norns.state.shortname = 'none'
  norns.state.path = _path["dust"]
  norns.state.data = _path.data
  norns.state.lib = norns.state.path
  norns.version.required = nil

  -- clear params
  params:clear()
  norns.pmap.clear()
  -- add audio menu
  audio.add_params()
  -- add clock menu
  clock.add_params()
  -- re-enable crow clock if needed
  if params:string("clock_source") == "crow" then
    crow.input[1].change = function() end
    crow.input[1].mode("change",2,0.1,"rising")
  end

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
  local name, path
  if filename == nil then
    filename = norns.state.script
    name = norns.state.name
    path = norns.state.path
  else
    filename = string.sub(filename,1,1) == "/" and filename or _path["dust"]..filename
    path, scriptname = filename:match("^(.*)/([^.]*).*$")
    name = string.sub(path, string.len(_path["code"]) + 1)
    -- append scriptname to the name if it doesn't match directory name in case multiple scripts reside in the same directory
    -- ex: we/study/study1, we/study/study2, ...
    if string.sub(name, -#scriptname) ~= scriptname then
      name_parts = tab.split(name, "/")
      table.insert(name_parts, scriptname)
      name = table.concat(name_parts, "/")
    end
  end

  local f=io.open(filename,"r")
  if f==nil then
    print("# script load failed, file not found: " .. filename)
  else
    io.close(f)

    Script.clear() -- clear script variables and functions

    print("# script load: " .. filename)

    norns.state.script = filename
    norns.state.name = name
    norns.state.shortname = norns.state.name:match( "([^/]+)$" )
    norns.state.path = path .. '/'
    norns.state.lib = path .. '/lib/'
    norns.state.data = _path.data .. name .. '/'
    norns.state.pset_last = 1

    if util.file_exists(norns.state.data) == false then
      print("### initializing data folder")
      util.make_dir(norns.state.data)
      if util.file_exists(norns.state.path.."/data") then
        os.execute("cp "..norns.state.path.."/data/*.pset "..norns.state.data)
        print("### copied default psets")
      end
    end

    local file = norns.state.data.."pset-last.txt"
    if util.file_exists(file) then
      local f = io.open(file,"r")
      io.input(f)
      local i = io.read("*line")
      io.close(f)
      if i then
        print("pset last used: "..i)
        norns.state.pset_last = tonumber(i)
      end
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
  if tonumber(norns.version.required) and tonumber(norns.version.required) > tonumber(norns.version.update) then
    norns.scripterror("version " .. norns.version.required .. " required")
    Script.clear()
    return
  end
  -- allow mods to do initialization
  hook.script_pre_init()

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
        local skip_hyphens = string.sub(line,4,-1)
        local fix_newlines = string.gsub(skip_hyphens,"\r$","")
        table.insert(meta, fix_newlines)
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
