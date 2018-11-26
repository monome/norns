//n = NetAddr("localhost", 9999);
n = NetAddr("172.20.10.5", 9999);


/// voice 1
n.sendMsg("/set/level/adc_cut", 0, 0, 1.0);
n.sendMsg("/set/level/adc_cut", 0, 1, 1.0);


n.sendMsg("/set/level/ext_cut", 0, 0, 1.0);
n.sendMsg("/set/level/ext_cut", 0, 1, 1.0);

n.sendMsg("/set/level/cut", 0, 0.6);
n.sendMsg("/set/pan/cut", 0, 0);
n.sendMsg("/set/enabled/cut", 0, 1.0);

n.sendMsg("/set/param/cut/rate", 0, 1.0);
n.sendMsg("/set/param/cut/loop_start", 0, 1.0);
n.sendMsg("/set/param/cut/loop_end", 0, 2.0);
n.sendMsg("/set/param/cut/loop_flag", 0, 1.0);
n.sendMsg("/set/param/cut/fade_time", 0, 0.1);
n.sendMsg("/set/param/cut/rec_level", 0, 1.0);
n.sendMsg("/set/param/cut/pre_level", 0, 0.7);
n.sendMsg("/set/param/cut/rec_flag", 0, 1);

n.sendMsg("/set/param/cut/position", 0, 1);

n.sendMsg("/set/param/cut/filter_dry", 0, 0.125);
n.sendMsg("/set/param/cut/filter_fc", 0, 1200);
n.sendMsg("/set/param/cut/filter_lp", 0, 0);
n.sendMsg("/set/param/cut/filter_bp", 0, 1.0);
n.sendMsg("/set/param/cut/filter_rq", 0, 2.0);

n.sendMsg("/set/param/cut/rate", 0, 0.75);
n.sendMsg("/set/param/cut/rate", 0, 1.0);
n.sendMsg("/set/param/cut/rate", 0, 1.75);


/// voice 1
n.sendMsg("/set/level/adc_cut", 1, 0, 1.0);
n.sendMsg("/set/level/adc_cut", 1, 1, 1.0);

n.sendMsg("/set/level/ext_cut", 1, 0, 1.0);
n.sendMsg("/set/level/ext_cut", 1, 1, 1.0);


n.sendMsg("/set/level/cut", 1, 0.6);
n.sendMsg("/set/pan/cut", 1, 1.0);
n.sendMsg("/set/enabled/cut", 1, 1.0);

n.sendMsg("/set/param/cut/rate", 1, 1.0);
n.sendMsg("/set/param/cut/loop_start", 1, 3.0);
n.sendMsg("/set/param/cut/loop_end", 1, 4.250);
n.sendMsg("/set/param/cut/loop_flag", 1, 1.0);
n.sendMsg("/set/param/cut/fade_time", 1, 0.1);
n.sendMsg("/set/param/cut/rec_level", 1, 1.0);
n.sendMsg("/set/param/cut/pre_level", 1, 0.7);
n.sendMsg("/set/param/cut/rec_flag", 1, 1);

n.sendMsg("/set/param/cut/position", 1, 1);

n.sendMsg("/set/param/cut/filter_dry", 1, 0.125);
n.sendMsg("/set/param/cut/filter_fc", 1, 1200);
n.sendMsg("/set/param/cut/filter_lp", 1, 0);
n.sendMsg("/set/param/cut/filter_bp", 1, 1.0);
n.sendMsg("/set/param/cut/filter_rq", 1, 2.0);

n.sendMsg("/set/param/cut/rate", 1, 0.75);
n.sendMsg("/set/param/cut/rate", 1, 1.0);
n.sendMsg("/set/param/cut/rate", 1, 1.75);