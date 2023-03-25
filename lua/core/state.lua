-- State

local state = {}
state.script = ''
state.path = _path.code
state.lib = _path.code
state.data = _path.data
state.name = ''
state.shortname = ''
state.clean_shutdown = false
state.battery_warning = 1

state.mix = {}
state.mix.output = 0
state.mix.input = 0
state.mix.monitor = 0
state.mix.engine = 0
state.mix.cut = 0
state.mix.tape = 0
state.mix.monitor_mode = 1
state.mix.headphone_gain = 40
state.mix.aux = 2
state.mix.ins = 1

state.mix.rev_eng_input = -9
state.mix.rev_cut_input = -9
state.mix.rev_monitor_input = -math.huge
state.mix.rev_tape_input = -math.huge
state.mix.rev_return_level = 0
state.mix.rev_pre_delay = 60
state.mix.rev_lf_fc = 200
state.mix.rev_low_time = 6
state.mix.rev_mid_time = 6
state.mix.rev_hf_damping = 6000

state.mix.comp_mix = 1
state.mix.comp_ratio = 4
state.mix.comp_threshold = -18
state.mix.comp_attack = 5
state.mix.comp_release = 50
state.mix.comp_pre_gain = 0
state.mix.comp_post_gain = 9

state.mix.cut_input_adc = 0
state.mix.cut_input_eng = 0
state.mix.cut_input_tape = -math.huge

state.clock = {}
state.clock.source = 1
state.clock.tempo = 90
state.clock.link_quantum = 4
state.clock.link_start_stop_sync = 1
state.clock.midi_out = {}
state.clock.crow_out = 1
state.clock.crow_out_div = 4
state.clock.crow_in_div = 4

-- read state.lua and set parameters back to stored vals.
state.resume = function()
  -- restore state object
  local f = io.open(_path.data..'system.state')
  if f ~= nil then
    io.close(f)
    dofile(_path.data..'system.state')
  end

  -- if previously-saved state.clock.midi_out is a number,
  -- make it a table
  if type(state.clock.midi_out) == 'number' then
    state.clock.midi_out = {}
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
      norns.scripterror("NO SCRIPT")
    end
    -- reset clean_shutdown flag and save state so that
    -- if the script causes a crash we don't restart into it
    state.clean_shutdown = false
    state.save_state()
  else
    norns.script.clear()
    norns.scripterror("NO SCRIPT")
  end
end

-- save current norns state to system.pset and system.state
state.save = function()
  state.save_state()
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
  io.write("norns.state.battery_warning = " .. state.battery_warning .. "\n")
  io.write("norns.state.mix.output = " .. norns.state.mix.output .. "\n")
  io.write("norns.state.mix.input = " .. norns.state.mix.input .. "\n")
  io.write("norns.state.mix.monitor = " .. norns.state.mix.monitor .. "\n")
  io.write("norns.state.mix.engine = " .. norns.state.mix.engine .. "\n")
  io.write("norns.state.mix.cut = " .. norns.state.mix.cut .. "\n")
  io.write("norns.state.mix.tape = " .. norns.state.mix.tape .. "\n")
  io.write("norns.state.mix.aux = " .. norns.state.mix.aux .. "\n")
  io.write("norns.state.mix.ins = " .. norns.state.mix.ins .. "\n")
  io.write("norns.state.mix.monitor_mode = " .. norns.state.mix.monitor_mode .. "\n")
  io.write("norns.state.mix.headphone_gain = " .. norns.state.mix.headphone_gain .. "\n")
  io.write("norns.state.mix.rev_eng_input = " .. norns.state.mix.rev_eng_input .. "\n")
  io.write("norns.state.mix.rev_cut_input = " .. norns.state.mix.rev_cut_input .. "\n")
  io.write("norns.state.mix.rev_monitor_input = " .. norns.state.mix.rev_monitor_input .. "\n")
  io.write("norns.state.mix.rev_tape_input = " .. norns.state.mix.rev_tape_input .. "\n")
  io.write("norns.state.mix.rev_return_level = " .. norns.state.mix.rev_return_level .. "\n")
  io.write("norns.state.mix.rev_pre_delay = " .. norns.state.mix.rev_pre_delay .. "\n")
  io.write("norns.state.mix.rev_lf_fc = " .. norns.state.mix.rev_lf_fc .. "\n")
  io.write("norns.state.mix.rev_low_time = " .. norns.state.mix.rev_low_time .. "\n")
  io.write("norns.state.mix.rev_mid_time = " .. norns.state.mix.rev_mid_time .. "\n")
  io.write("norns.state.mix.rev_hf_damping = " .. norns.state.mix.rev_hf_damping .. "\n")
  io.write("norns.state.mix.comp_mix = " .. norns.state.mix.comp_mix .. "\n")
  io.write("norns.state.mix.comp_ratio = " .. norns.state.mix.comp_ratio .. "\n")
  io.write("norns.state.mix.comp_threshold = " .. norns.state.mix.comp_threshold .. "\n")
  io.write("norns.state.mix.comp_attack = " .. norns.state.mix.comp_attack .. "\n")
  io.write("norns.state.mix.comp_release = " .. norns.state.mix.comp_release .. "\n")
  io.write("norns.state.mix.comp_pre_gain = " .. norns.state.mix.comp_pre_gain .. "\n")
  io.write("norns.state.mix.comp_post_gain = " .. norns.state.mix.comp_post_gain .. "\n")
  io.write("norns.state.mix.cut_input_adc = " .. norns.state.mix.cut_input_adc .. "\n")
  io.write("norns.state.mix.cut_input_eng = " .. norns.state.mix.cut_input_eng .. "\n")
  io.write("norns.state.mix.cut_input_tape = " .. norns.state.mix.cut_input_tape .. "\n")
  io.write("norns.state.clock.source = " .. norns.state.clock.source .. "\n")
  io.write("norns.state.clock.tempo = " .. norns.state.clock.tempo .. "\n")
  io.write("norns.state.clock.link_quantum = " .. norns.state.clock.link_quantum .. "\n")
  io.write("norns.state.clock.link_start_stop_sync = " .. norns.state.clock.link_start_stop_sync .. "\n")
  for i = 1,16 do
    io.write("norns.state.clock.midi_out["..i.."] = " .. norns.state.clock.midi_out[i] .. "\n")
  end
  io.write("norns.state.clock.crow_out = " .. norns.state.clock.crow_out .. "\n")
  io.write("norns.state.clock.crow_out_div = " .. norns.state.clock.crow_out_div .. "\n")
  io.write("norns.state.clock.crow_in_div = " .. norns.state.clock.crow_in_div .. "\n")
  for i=1,16 do
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
