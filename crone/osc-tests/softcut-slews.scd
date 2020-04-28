n = NetAddr("localhost", 9999);


// load buffer
n.sendMsg("/softcut/buffer/read_stereo", "/home/emb/snd/blue-lights.wav", 0, 0, -1);

// mix to DAC
n.sendMsg("/set/level/cut", 0, 0.9);
n.sendMsg("/set/level/cut_aux", 1.0);

/// voice 1
// args: channel index, voice index, level

n.sendMsg("/set/pan/cut", 0, 1);
n.sendMsg("/set/enabled/cut", 0, 1.0);

n.sendMsg("/set/param/cut/rate", 0, 2.0);
n.sendMsg("/set/param/cut/loop_start", 0, 300.0);
n.sendMsg("/set/param/cut/loop_end", 0, 304.0);

n.sendMsg("/set/param/cut/loop_flag", 0, 1.0);
n.sendMsg("/set/param/cut/fade_time", 0, 0.1);

n.sendMsg("/set/param/cut/play_flag", 0, 1);
n.sendMsg("/set/param/cut/rec_flag", 0, 0);
n.sendMsg("/set/param/cut/rec", 0, 1.0);
n.sendMsg("/set/param/cut/pre", 0, 0.5);

n.sendMsg("/set/param/cut/position", 0, 200);

n.sendMsg("/set/param/cut/post_filter_dry", 0, 0);
n.sendMsg("/set/param/cut/post_filter_lp", 0, 1);
n.sendMsg("/set/param/cut/post_filter_hp", 0, 0.0);
n.sendMsg("/set/param/cut/post_filter_bp", 0, 0.0);
n.sendMsg("/set/param/cut/post_filter_br", 0, 0.0);

n.sendMsg("/set/param/cut/level_slew_time", 0, 10);
n.sendMsg("/set/param/cut/level_slew_time", 0, 0.1);


n.sendMsg("/set/level/cut", 0, 1.00);
n.sendMsg("/set/level/cut", 0, 0.0);
n.sendMsg("/set/param/cut/level_slew_time", 0, 4);
n.sendMsg("/set/level/cut", 0, 0.5);
n.sendMsg("/set/level/cut", 0, 0.6);
n.sendMsg("/set/level/cut", 0, 0.8);


n.sendMsg("/set/param/cut/pan_slew_time", 0, 20);
n.sendMsg("/set/pan/cut", 0, -1);


n.sendMsg("/set/param/cut/pan_slew_time", 0, 0.1);
n.sendMsg("/set/pan/cut", 0, 1);
n.sendMsg("/set/pan/cut", 0, 0);


n.sendMsg("/set/param/cut/rate_slew_time", 0, 1);
n.sendMsg("/set/param/cut/rate", 0, -4);
n.sendMsg("/set/param/cut/rate", 0, 12);


n.sendMsg("/set/param/cut/rate_slew_time", 0, 4);
n.sendMsg("/set/param/cut/rate", 0, 1);


n.sendMsg("/set/param/cut/filter_fc_slew_time", 0, 16);
n.sendMsg("/set/param/cut/post_filter_fc", 0, 100);
n.sendMsg("/set/param/cut/post_filter_rq", 0, 0.1);


n.sendMsg("/set/param/cut/filter_fc_slew_time", 0, 2);
n.sendMsg("/set/param/cut/post_filter_fc", 0, 7000);
n.sendMsg("/set/param/cut/post_filter_fc", 0, 700);

n.sendMsg("/set/param/cut/post_filter_rq", 0, 0.75);
n.sendMsg("/set/param/cut/filter_rq_slew_time", 0, 12);
n.sendMsg("/set/param/cut/filter_rq_rise_shape", 0, 4);
n.sendMsg("/set/param/cut/filter_rq_fall_shape", 0, 2);


n.sendMsg("/set/param/cut/filter_fc_rise_shape", 0, 12);
n.sendMsg("/set/param/cut/filter_fc_fall_shape", 0, 14);

n.sendMsg("/set/param/cut/rate_slew_shape", 0, 12);
n.sendMsg("/set/param/cut/filter_fc_fall_shape", 0, 12);