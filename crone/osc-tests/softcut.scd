n = NetAddr("localhost", 9999);

n.sendMsg("/set/level/adc_cut", 0, 0, 1.0);
n.sendMsg("/set/level/adc_cut", 0, 1, 1.0);

n.sendMsg("/set/level/cut", 0, 1.0);
n.sendMsg("/set/pan/cut", 0, 1.0);
n.sendMsg("/set/enabled/cut", 0, 1.0);

n.sendMsg("/set/param/cut/rate", 0, 1.0);
n.sendMsg("/set/param/cut/loop_start", 0, 1.0);
n.sendMsg("/set/param/cut/loop_end", 0, 2.0);
n.sendMsg("/set/param/cut/loop_flag", 0, 1.0);
n.sendMsg("/set/param/cut/fade_time", 0, 0.1);
n.sendMsg("/set/param/cut/rec_level", 0, 1.0);
n.sendMsg("/set/param/cut/pre_level", 0, 0.2);
n.sendMsg("/set/param/cut/rec_flag", 0, 1);

n.sendMsg("/set/param/cut/position", 0, 1);



n.sendMsg("/set/param/cut/filter_dry", 0, 0.0);
