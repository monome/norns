--- State
-- @module state

state = {}
-- volumes and gains
state.out = 0
state.monitor = 0
state.monitor_mode = 0
state.input_left = 0
state.input_right = 0
state.hp = 0
-- last script
state.script = ''

-- read state.lua and set parameters back to stored vals
state.resume = function()
  dofile(data_dir .. 'system.lua')
  -- set stored output level
  audio_output_level(state.out / 64.0)
  -- set inputs
  gain_in(state.input_left,0)
  gain_in(state.input_right,1)
  -- set hp
  gain_hp(state.hp)
  -- resume last file
  local f=io.open(script_dir .. state.script,"r")
  if f~=nil then
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
  local fd=io.open(data_dir .. "system.lua","w+")
  io.output(fd)
  io.write("-- system state\n")
  io.write("norns.state.script = '" .. state.script .. "'\n")
  io.write("norns.state.out = '" .. state.out .. "'\n")
  io.write("norns.state.monitor = '" .. state.monitor .. "'\n")
  io.write("norns.state.monitor_mode = '" .. state.monitor_mode.. "'\n")
  io.write("norns.state.input_left = '" .. state.input_left .. "'\n")
  io.write("norns.state.input_right = '" .. state.input_right .. "'\n")
  io.write("norns.state.hp = '" .. state.hp .. "'\n")
  io.close(fd)
end

return state
