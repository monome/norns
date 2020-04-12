n = NetAddr("localhost", 9999);

n.sendMsg("/set/level/adc_cut", 1.0);

p = "/home/emb/snd/dont_explain_48k.wav";

n.sendMsg("/softcut/buffer/read_stereo",p, 0, 0, -1);


n.sendMsg("/set/param/cut/buffer", 0, 0);
n.sendMsg("/set/param/cut/buffer", 1, 1);
n.sendMsg("/set/param/cut/follow", 1, 0);



/// voice 1
// args: channel index, voice index, level

n.sendMsg("/set/pan/cut", 0,-1);
n.sendMsg("/set/level/cut", 0, 1);
n.sendMsg("/set/enabled/cut", 0, 1.0);

n.sendMsg("/set/param/cut/rate", 0, 1.0);
n.sendMsg("/set/param/cut/loop_start", 0, 2.0);
n.sendMsg("/set/param/cut/loop_end", 0, 13.0);

n.sendMsg("/set/param/cut/loop_flag", 0, 1.0);
n.sendMsg("/set/param/cut/fade_time", 0, 0.1);

n.sendMsg("/set/param/cut/play_flag", 0, 1);
n.sendMsg("/set/param/cut/rec_flag", 0, 0);
n.sendMsg("/set/param/cut/rec", 0, 1.0);
n.sendMsg("/set/param/cut/pre", 0, 0.5);

n.sendMsg("/set/param/cut/position", 0, 2);

n.sendMsg("/set/param/cut/pre_filter_dry", 0, 1);
n.sendMsg("/set/param/cut/pre_filter_lp", 0, 0);
n.sendMsg("/set/param/cut/pre_filter_hp", 0, 0.0);
n.sendMsg("/set/param/cut/pre_filter_bp", 0, 0.0);
n.sendMsg("/set/param/cut/pre_filter_br", 0, 0.0);


n.sendMsg("/set/param/cut/post_filter_dry", 0, 1);
n.sendMsg("/set/param/cut/post_filter_lp", 0, 0);
n.sendMsg("/set/param/cut/post_filter_hp", 0, 0.0);
n.sendMsg("/set/param/cut/post_filter_bp", 0, 0.0);
n.sendMsg("/set/param/cut/post_filter_br", 0, 0.0);


/// voice 2
// args: channel index, voice index, level

n.sendMsg("/set/pan/cut", 1, 1);
n.sendMsg("/set/level/cut", 1, 1);
n.sendMsg("/set/enabled/cut", 1, 1.0);
n.sendMsg("/set/param/cut/play_flag", 1, 1);

n.sendMsg("/set/param/cut/pre_filter_dry", 1, 1);
n.sendMsg("/set/param/cut/pre_filter_lp", 1, 0);
n.sendMsg("/set/param/cut/pre_filter_hp", 1, 0.0);
n.sendMsg("/set/param/cut/pre_filter_bp", 1, 0.0);
n.sendMsg("/set/param/cut/pre_filter_br", 1, 0.0);


n.sendMsg("/set/param/cut/post_filter_dry", 1, 1);
n.sendMsg("/set/param/cut/post_filter_lp", 1, 0);
n.sendMsg("/set/param/cut/post_filter_hp", 1, 0.0);
n.sendMsg("/set/param/cut/post_filter_bp", 1, 0.0);
n.sendMsg("/set/param/cut/post_filter_br", 1, 0.0);


Routine {
	4.0.wait;
	n.sendMsg("/set/param/cut/rate_slew_time", 0, 20.0);
	n.sendMsg("/set/param/cut/rate", 0, -1.5);


	24.0.wait;

	n.sendMsg("/set/param/cut/rate", 0, 1.0);
}.play;
