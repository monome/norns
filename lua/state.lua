--- State
-- @module state

state = {}
state.script = ''

-- read state.lua and set parameters back to stored vals
state.resume = function()
  -- restore mix state
  mix:read("system.pset")
  mix:bang()

  local f = io.open(data_dir..'system.lua')
  if f ~= nil then
    io.close(f)
    dofile(data_dir..'system.lua')
  end

  -- TODO: default script
  if state.script == nil or state.script == '' then
    state.script = 'tehn/earthsea.lua'
  end

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
end

--- save current norns state to state.lua
state.save = function()
  -- save mix state
  mix:write("system.pset")
  -- save current file
  local fd=io.open(data_dir .. "system.lua","w+")
  io.output(fd)
  io.write("-- system state\n")
  io.write("norns.state.script = '" .. state.script .. "'\n")
  io.close(fd)
end

return state
