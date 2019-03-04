-- half sec loop 75% decay

local sc = {}

function sc.init()
  print("starting softcut/halfsecond")
	audio.level_cut(1.0)
	audio.level_adc_cut(1)
	audio.level_ext_cut(1)
  softcut.level(1,1.0)
	softcut.level_input_cut(1, 1, 1.0)
	softcut.level_input_cut(1, 2, 1.0)
	softcut.pan(1, 0.5)

  softcut.play(1, 1)
	softcut.rate(1, 1)
	softcut.loop_start(1, 1)
	softcut.loop_end(1, 1.5)
	softcut.loop(1, 1)
	softcut.fade_time(1, 0.1)
	softcut.rec(1, 1)
	softcut.rec_level(1, 1)
	softcut.pre_level(1, 0.75)
	softcut.position(1, 1)
	softcut.enable(1, 1)

	softcut.filter_dry(1, 0.125);
	softcut.filter_fc(1, 1200);
	softcut.filter_lp(1, 0);
	softcut.filter_bp(1, 1.0);
	softcut.filter_rq(1, 2.0);

  params:add_separator()
  local p = softcut.params()
  params:add(p[1].rate)
end

return sc
