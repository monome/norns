n = NetAddr("localhost", 9999);

// mix to softcut
n.sendMsg("/set/level/adc_cut", 1.0);

// mix to DAC

/// voice 1, playing, not recording, ducking voice 2
n.sendMsg("/set/level/cut", 0, 0.9);
n.sendMsg("/set/param/cut/buffer", 0, 0);
n.sendMsg("/set/pan/cut", 0, -0.5);
n.sendMsg("/set/enabled/cut", 0, 1.0);

n.sendMsg("/set/param/cut/rate", 0, -2);
n.sendMsg("/set/param/cut/loop_start", 0, 0.33);
n.sendMsg("/set/param/cut/loop_end", 0, 0.8);

n.sendMsg("/set/param/cut/loop_flag", 0, 1.0);
n.sendMsg("/set/param/cut/fade_time", 0, 0.025);
n.sendMsg("/set/param/cut/rec_level", 0, 1.0);
n.sendMsg("/set/param/cut/pre_level", 0, 0.0);
n.sendMsg("/set/param/cut/rec_flag", 0, 0);
n.sendMsg("/set/param/cut/play_flag", 0, 1);
n.sendMsg("/set/param/cut/position", 0, 1);
n.sendMsg("/set/param/cut/read_duck", 0, 1);


/// voice 2, recording and playing
n.sendMsg("/set/level/in_cut", 0, 1, 1.0);
n.sendMsg("/set/level/in_cut", 1, 1, 1.0);
n.sendMsg("/set/param/cut/buffer", 1, 0);
n.sendMsg("/set/level/cut", 1, 0.6);
n.sendMsg("/set/pan/cut", 1, 0);
n.sendMsg("/set/enabled/cut", 1, 1.0);

n.sendMsg("/set/param/cut/rate", 1, 0.5);
n.sendMsg("/set/param/cut/loop_start", 1, 0.3);
n.sendMsg("/set/param/cut/loop_end", 1, 0.833);
n.sendMsg("/set/param/cut/loop_flag", 1, 1.0);
n.sendMsg("/set/param/cut/fade_time", 1, 0.25);
n.sendMsg("/set/param/cut/rec_level", 1, 1);
n.sendMsg("/set/param/cut/pre_level", 1, 0.65);
n.sendMsg("/set/param/cut/rec_flag", 1, 1);
n.sendMsg("/set/param/cut/play_flag", 1, 1);
n.sendMsg("/set/param/cut/position", 1, 0);
n.sendMsg("/set/param/cut/read_duck", 1, 0); // cycle


/// voice 3, playing, not recording, ducking voice 2
n.sendMsg("/set/param/cut/rec_flag", 2, 0);
n.sendMsg("/set/param/cut/play_flag", 2, 1);

n.sendMsg("/set/level/cut", 2, 0.8);
n.sendMsg("/set/pan/cut", 2, 0.5);
n.sendMsg("/set/param/cut/buffer", 2, 0);
n.sendMsg("/set/enabled/cut", 2, 1);

n.sendMsg("/set/param/cut/rate", 2, 1.5);
n.sendMsg("/set/param/cut/rate_slew_time", 2, 2);
n.sendMsg("/set/param/cut/loop_start", 2, 0.41);
n.sendMsg("/set/param/cut/loop_end", 2, 0.7);

n.sendMsg("/set/param/cut/loop_flag", 2, 1.0);
n.sendMsg("/set/param/cut/fade_time", 2, 0.0125);
n.sendMsg("/set/param/cut/rec_level", 2, 1.0);
n.sendMsg("/set/param/cut/pre_level", 2, 0.0);
n.sendMsg("/set/param/cut/position", 2, 0.5);
n.sendMsg("/set/param/cut/read_duck", 2, 1);


/// route 1+3 back to 2 for a little shimmer
n.sendMsg("/set/level/cut_cut", 0, 1, 0.8);
n.sendMsg("/set/level/cut_cut", 2, 1, 0.5);

n.sendMsg("/set/level/cut_cut", 0, 1, 0.0);
n.sendMsg("/set/level/cut_cut", 2, 1, 0.0);

// no duck
/*
n.sendMsg("/set/param/cut/duck", 0, -1);
n.sendMsg("/set/param/cut/duck", 2, -1);
*/

