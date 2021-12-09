~crone_addr = n; //  NetAddr("172.20.10.5", 9999);

~num_voices = 6;

~pos = 0.0;
~dur = 1.0;
~pad = 1.0; // max fade time, basically

~num_voices.do({ |i|

	~crone_addr.sendMsg("/set/level/in_cut", 0, i, 1.0);
	~crone_addr.sendMsg("/set/level/in_cut", 1, i, 1.0);

	~crone_addr.sendMsg("/set/level/cut", i, 0.9);
	~crone_addr.sendMsg("/set/pan/cut", i, i.linlin(0, ~num_voices-1, 0.1, 0.9));
	~crone_addr.sendMsg("/set/enabled/cut", i, 1.0);

	~crone_addr.sendMsg("/set/param/cut/rate", i, 1.0);
	~crone_addr.sendMsg("/set/param/cut/loop_start", i, ~pos);
	~crone_addr.sendMsg("/set/param/cut/loop_end", i, ~pos + ~dur);
	~crone_addr.sendMsg("/set/param/cut/loop_flag", i, 1.0);
	~crone_addr.sendMsg("/set/param/cut/fade_time", i, 0.1);
	~crone_addr.sendMsg("/set/param/cut/rec_level", i, 1.0);
	~crone_addr.sendMsg("/set/param/cut/pre_level", i, 0.7);
	~crone_addr.sendMsg("/set/param/cut/rec_flag", i, 1);

	~crone_addr.sendMsg("/set/param/cut/position", i, 1);

	~crone_addr.sendMsg("/set/param/cut/filter_dry", i, 0.125);
	~crone_addr.sendMsg("/set/param/cut/filter_fc", i, 1200 * (1.2 ** i));
	~crone_addr.sendMsg("/set/param/cut/filter_lp", i, 0);
	~crone_addr.sendMsg("/set/param/cut/filter_bp", i, 1.0);
	~crone_addr.sendMsg("/set/param/cut/filter_rq", i, 2.0);

	~crone_addr.sendMsg("/set/param/cut/rate", i, 0.165);
	~crone_addr.sendMsg("/set/param/cut/rate", i, 1.0);
	~crone_addr.sendMsg("/set/param/cut/rate", i, 8.0);

	~pos = ~pos + ~dur + ~pad;
	~dur = ~dur * 1.5;

});