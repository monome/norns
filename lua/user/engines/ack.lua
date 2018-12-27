local ControlSpec = require 'controlspec'
local Formatters = require 'jah/formatters'
local Ack = {}

local specs = {}

specs.send = ControlSpec.DB:copy()
specs.send.default = -60

specs.sample_start = ControlSpec.UNIPOLAR
specs.sample_end = ControlSpec.new(0, 1, 'lin', 0, 1, '')
specs.loop_point = ControlSpec.UNIPOLAR

specs.speed = ControlSpec.new(0.05, 5, 'lin', 0, 1, '')
-- specs.slew = ControlSpec.new(0, 5, 'lin', 0, 0, '') -- TODO: slews

specs.volume = ControlSpec.DB:copy()
specs.volume.default = -10

specs.volume_env_attack = ControlSpec.new(0, 1, 'lin', 0, 0.001, 'secs')
specs.volume_env_release = ControlSpec.new(0, 3, 'lin', 0, 3, 'secs')
specs.filter_env_attack = ControlSpec.new(0, 1, 'lin', 0, 0.001, 'secs')
specs_filter_env_release = ControlSpec.new(0, 3, 'lin', 0, 0.25, 'secs')

specs.filter_cutoff = ControlSpec.FREQ:copy()
specs.filter_cutoff.default = 20000

specs.filter_res = ControlSpec.UNIPOLAR
specs.filter_env_mod = ControlSpec.BIPOLAR
specs.dist = ControlSpec.UNIPOLAR

specs.delay_time = ControlSpec.new(0.0001, 5, 'exp', 0, 0.1, 'secs')
specs.delay_feedback = ControlSpec.new(0, 1.25, 'lin', 0, 0.5, '')
specs.delay_level = ControlSpec.DB:copy()
specs.delay_level.default = -10

specs.reverb_room = ControlSpec.new(0, 1, 'lin', 0, 0.5, '')
specs.reverb_damp = ControlSpec.new(0, 1, 'lin', 0, 0.5, '')
specs.reverb_level = ControlSpec.DB:copy()
specs.reverb_level.default = -10

Ack.specs = specs

function Ack.add_channel_sample_param(channel)
  params:add_file(channel.."_sample", channel..": sample")
  params:set_action(channel.."_sample", function(value)
    if value ~= "-" then
      engine.loadSample(channel-1, value)
    end
  end)
end

function Ack.add_start_pos_param(channel)
  params:add_control(channel.."_start_pos", channel..": start pos", specs.sample_start, Formatters.unipolar_as_percentage)
  params:set_action(channel.."_start_pos", function(value) engine.sampleStart(channel-1, value) end)
end

function Ack.add_end_pos_param(channel)
  params:add_control(channel.."_end_pos", channel..": end pos", specs.sample_end, Formatters.unipolar_as_percentage)
  params:set_action(channel.."_end_pos", function(value) engine.sampleEnd(channel-1, value) end)
end

function Ack.add_loop_param(channel)
  params:add_option(channel.."_loop", channel..": loop", {"off", "on"})
  params:set_action(channel.."_loop", function(value)
    if value == 2 then
      engine.enableLoop(channel-1)
    else
      engine.disableLoop(channel-1)
    end
  end)
end

function Ack.add_loop_point_param(channel)
  params:add_control(channel.."_loop_point", channel..": loop point", specs.loop_point, Formatters.unipolar_as_percentage)
  params:set_action(channel.."_loop_point", function(value) engine.loopPoint(channel-1, value) end)
end

function Ack.add_speed_param(channel)
  params:add_control(channel.."_speed", channel..": speed", specs.speed, Formatters.unipolar_as_percentage)
  params:set_action(channel.."_speed", function(value) engine.speed(channel-1, value) end)
end

function Ack.add_vol_param(channel)
  params:add_control(channel.."_vol", channel..": vol", specs.volume, Formatters.default)
  params:set_action(channel.."_vol", function(value) engine.volume(channel-1, value) end)
end

function Ack.add_vol_env_atk_param(channel)
  params:add_control(channel.."_vol_env_atk", channel..": vol env atk", specs.volume_env_attack, Formatters.secs_as_ms)
  params:set_action(channel.."_vol_env_atk", function(value) engine.volumeEnvAttack(channel-1, value) end)
end

function Ack.add_vol_env_rel_param(channel)
  params:add_control(channel.."_vol_env_rel", channel..": vol env rel", specs.volume_env_release, Formatters.secs_as_ms)
  params:set_action(channel.."_vol_env_rel", function(value) engine.volumeEnvRelease(channel-1, value) end)
end

function Ack.add_pan_param(channel)
  params:add_control(channel.."_pan", channel..": pan", ControlSpec.PAN, Formatters.bipolar_as_pan_widget)
  params:set_action(channel.."_pan", function(value) engine.pan(channel-1, value) end)
end

function Ack.add_filter_cutoff_param(channel)
  params:add_control(channel.."_filter_cutoff", channel..": filter cutoff", specs.filter_cutoff, Formatters.round(0.001))
  params:set_action(channel.."_filter_cutoff", function(value) engine.filterCutoff(channel-1, value) end)
end

function Ack.add_filter_res_param(channel)
  params:add_control(channel.."_filter_res", channel..": filter res", specs.filter_res, Formatters.unipolar_as_percentage)
  params:set_action(channel.."_filter_res", function(value) engine.filterRes(channel-1, value) end)
end

function Ack.add_filter_mode_param(channel)
  params:add_option(channel.."_filter_mode", channel..": filter mode", {"lowpass", "bandpass", "highpass", "notch", "peak"})
  params:set_action(channel.."_filter_mode", function(value) engine.filterMode(channel-1, value-1) end)
end

function Ack.add_filter_env_atk_param(channel)
  params:add_control(channel.."_filter_env_atk", channel..": filter env atk", specs.filter_env_attack, Formatters.secs_as_ms)
  params:set_action(channel.."_filter_env_atk", function(value) engine.filterEnvAttack(channel-1, value) end)
end

function Ack.add_filter_env_rel_param(channel)
  params:add_control(channel.."_filter_env_rel", channel..": filter env rel", Ack.specs_filter_env_release, Formatters.secs_as_ms)
  params:set_action(channel.."_filter_env_rel", function(value) engine.filterEnvRelease(channel-1, value) end)
end

function Ack.add_filter_env_mod_param(channel)
  params:add_control(channel.."_filter_env_mod", channel..": filter env mod", specs.filter_env_mod, Formatters.bipolar_as_percentage)
  params:set_action(channel.."_filter_env_mod", function(value) engine.filterEnvMod(channel-1, value) end)
end

function Ack.add_dist_param(channel)
  params:add_control(channel.."_dist", channel..": dist", specs.dist, Formatters.unipolar_as_percentage)
  params:set_action(channel.."_dist", function(value) engine.dist(channel-1, value) end)
end

function Ack.add_mutegroup_param(channel)
  params:add_option(channel.."_in_mutegroup", channel..": in mutegroup", {"no", "yes"})
  params:set_action(channel.."_in_mutegroup", function(value)
    if value == 2 then
      engine.includeInMuteGroup(channel-1, 1)
    else
      engine.includeInMuteGroup(channel-1, 0)
    end
  end)
end

function Ack.add_delay_send_param(channel)
  params:add_control(channel.."_delay_send", channel..": delay send", specs.send, Formatters.default)
  params:set_action(channel.."_delay_send", function(value) engine.delaySend(channel-1, value) end)
end

function Ack.add_reverb_send_param(channel)
  params:add_control(channel.."_reverb_send", channel..": reverb send", specs.send, Formatters.default)
  params:set_action(channel.."_reverb_send", function(value) engine.reverbSend(channel-1, value) end)
end

function Ack.add_channel_params(channel)
  Ack.add_channel_sample_param(channel)
  Ack.add_start_pos_param(channel)
  Ack.add_end_pos_param(channel)
  Ack.add_loop_param(channel)
  Ack.add_loop_point_param(channel)
  Ack.add_speed_param(channel)
  Ack.add_vol_param(channel)
  Ack.add_vol_env_atk_param(channel)
  Ack.add_vol_env_rel_param(channel)
  Ack.add_pan_param(channel)
  Ack.add_filter_mode_param(channel)
  Ack.add_filter_cutoff_param(channel)
  Ack.add_filter_res_param(channel)
  Ack.add_filter_env_atk_param(channel)
  Ack.add_filter_env_rel_param(channel)
  Ack.add_filter_env_mod_param(channel)
  Ack.add_dist_param(channel)
  Ack.add_mutegroup_param(channel)
  Ack.add_delay_send_param(channel)
  Ack.add_reverb_send_param(channel)

  -- TODO: refactor each param into a separate function
  --[[
  TODO: slews
  params:add_control(channel..": speed slew", slew_spec, Formatters.default)
  params:set_action(channel..": speed slew", function(value) engine.speedSlew(channel-1, value) end)
  params:add_control(channel..": vol slew", slew_spec, Formatters.default)
  params:set_action(channel..": vol slew", function(value) engine.volumeSlew(channel-1, value) end)
  params:add_control(channel..": pan slew", slew_spec, Formatters.default)
  params:set_action(channel..": pan slew", function(value) engine.panSlew(channel-1, value) end)
  params:add_control(channel..": filter cutoff slew", slew_spec, Formatters.default)
  params:set_action(channel..": filter cutoff slew", function(value) engine.filterCutoffSlew(channel-1, value) end)
  params:add_control(channel..": filter res slew", slew_spec, Formatters.default)
  params:set_action(channel..": filter res slew", function(value) engine.filterResSlew(channel-1, value) end)
  ]]
end

function Ack.add_effects_params()
  params:add_control("delay_time", "delay time", specs.delay_time, Formatters.secs_as_ms)
  params:set_action("delay_time", engine.delayTime)
  params:add_control("delay_feedback", "delay feedback", specs.delay_feedback, Formatters.unipolar_as_percentage)
  params:set_action("delay_feedback", engine.delayFeedback)
  params:add_control("delay_level", "delay level", specs.delay_level, Formatters.default)
  params:set_action("delay_level", engine.delayLevel)
  params:add_control("reverb_room_size", "reverb room size", specs.reverb_room, Formatters.unipolar_as_percentage)
  params:set_action("reverb_room_size", engine.reverbRoom)
  params:add_control("reverb_damp", "reverb damp", specs.reverb_damp, Formatters.unipolar_as_percentage)
  params:set_action("reverb_damp", engine.reverbDamp)
  params:add_control("reverb_level", "reverb level", specs.reverb_level, Formatters.default)
  params:set_action("reverb_level", engine.reverbLevel)
end

function Ack.add_params()
  for channel=1,8 do
    Ack.add_channel_params(channel)
    params:add_separator()
  end

  Ack.add_effects_params()
end

return Ack
