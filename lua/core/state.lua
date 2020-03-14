-- State
-- @classmod state

local state = {}
state.script = ''
state.path = _path.code
state.data = _path.data
state.name = ''
state.shortname = ''
state.clean_shutdown = false

-- read state.lua and set parameters back to stored vals.
state.resume = function()
  -- restore mix state
  mix:read(_path.data.."system.pset")
  mix:bang()

  -- restore state object
  local f = io.open(_path.data..'system.state')
  if f ~= nil then
    io.close(f)
    dofile(_path.data..'system.state')
  end

  -- update vports
  midi.update_devices()
  grid.update_devices()
  arc.update_devices()
  hid.update_devices()

  -- only resume the script if we shut down cleanly
  if state.clean_shutdown and state.script ~= '' then
    -- resume last file
    local f = io.open(state.script, "r")
    if f ~= nil then
      io.close(f)
      print("last file loaded: " .. state.script)
      norns.script.load()
    else
      norns.script.clear()
      state.script=''
      state.name = 'none'
      state.shortname = 'none'
      state.path = _path.code
      state.data = _path.data
      norns.scripterror("NO SCRIPT")
    end
    -- reset clean_shutdown flag and save state so that
    -- if the script causes a crash we don't restart into it
    state.clean_shutdown = false
    state.save_state()
  else
    norns.script.clear()
    state.script=''
    state.name = 'none'
    state.shortname = 'none'
    state.path = _path.code
    state.data = _path.data
    norns.scripterror("NO SCRIPT")
  end
end

-- save current norns state to system.pset and system.state
state.save = function()
  state.save_mix()
  state.save_state()
end

state.save_mix = function()
  mix:write(_path.data.."system.pset")
end

state.save_state = function()
  local fd=io.open(_path.data .. "system.state","w+")
  io.output(fd)
  io.write("-- norns system state\n")
  io.write("norns.state.clean_shutdown = " .. (state.clean_shutdown and "true" or "false") .. "\n")
  io.write("norns.state.script = '" .. state.script .. "'\n")
  io.write("norns.state.name = '" .. state.name .. "'\n")
  io.write("norns.state.shortname = '" .. state.shortname .. "'\n")
  io.write("norns.state.path = '" .. state.path .. "'\n")
  io.write("norns.state.data = '" .. state.data .. "'\n")
  for i=1,4 do
    io.write("midi.vports[" .. i .. "].name = '" .. midi.vports[i].name .. "'\n")
  end
  for i=1,4 do
    io.write("grid.vports[" .. i .. "].name = '" .. grid.vports[i].name .. "'\n")
  end
  for i=1,4 do
    io.write("arc.vports[" .. i .. "].name = '" .. arc.vports[i].name .. "'\n")
  end
  for i=1,4 do
    io.write("hid.vports[" .. i .. "].name = '" .. hid.vports[i].name .. "'\n")
  end
  io.close(fd)
end

return state
