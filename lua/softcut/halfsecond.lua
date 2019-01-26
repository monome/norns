-- half sec loop 75% decay

local sc = {}

function sc.init()
  print("starting softcut/halfsecond")
	audio.level_cut(1.0)
	audio.level_adc_cut(1)
	audio.level_ext_cut(1)
  softcut.level(0,1.0)
	softcut.level_input_cut(0, 0, 1.0)
	softcut.level_input_cut(1, 0, 1.0)
	softcut.pan(0, 0.5)

	softcut.rate(0, 1)
	softcut.loop_start(0, 1)
	softcut.loop_end(0, 1.5)
	softcut.loop_flag(0, 1)
	softcut.fade_time(0, 0.1)
	softcut.rec_flag(0, 1)
	softcut.rec_level(0, 1)
	softcut.pre_level(0, 0.75)
	softcut.position(0, 1)
	softcut.enable(0, 1)

	softcut.filter_dry(0, 0.125);
	softcut.filter_fc(0, 1200);
	softcut.filter_lp(0, 0);
	softcut.filter_bp(0, 1.0);
	softcut.filter_rq(0, 2.0);

  params:add("separator")
  local p = softcut.params()
  params:add(p[1].rate)
end

return sc
