--- State
-- @module state

state = {}
state.script = ''
state.clean_shutdown = false

-- read state.lua and set parameters back to stored vals
state.resume = function()
  -- restore mix state
  mix:read("system.pset")
  mix:bang()

  -- restore state object
  local f = io.open(data_dir..'system.lua')
  if f ~= nil then
    io.close(f)
    dofile(data_dir..'system.lua')
  end

  -- update vports
  midi.update_devices()
  grid.update_devices()
  arc.update_devices()

  -- only resume the script if we shut down cleanly
  if state.clean_shutdown and state.script ~= '' then
    -- resume last file
    local f = io.open(script_dir..state.script, "r")
    if f ~= nil then
      io.close(f)
      print("last file loaded: " .. state.script)
      norns.script.load()
    else
      state.script=''
      norns.script.clear()
      -- FIXME does tis work?
    end
    -- reset clean_shutdown flag and save state so that
    -- if the script causes a crash we don't restart into it
    state.clean_shutdown = false
    state.save_state()
  else
    state.script=''
    norns.scripterror("NO SCRIPT")
  end
end

--- save current norns state to system.pset and system.lua in data_dir
state.save = function()
  state.save_mix()
  state.save_state()
end

state.save_mix = function()
  mix:write("system.pset")
end

state.save_state = function()
  local fd=io.open(data_dir .. "system.lua","w+")
  io.output(fd)
  io.write("-- system state\n")
  io.write("norns.state.script = '" .. state.script .. "'\n")
  io.write("norns.state.clean_shutdown = " .. (state.clean_shutdown and "true" or "false") .. "\n")
  for i=1,4 do
    io.write("midi.vport[" .. i .. "].name = '" .. midi.vport[i].name .. "'\n")
  end
  for i=1,4 do
    io.write("grid.vport[" .. i .. "].name = '" .. grid.vport[i].name .. "'\n")
  end
  for i=1,4 do
    io.write("arc.vport[" .. i .. "].name = '" .. arc.vport[i].name .. "'\n")
  end
  io.close(fd)
end

return state
