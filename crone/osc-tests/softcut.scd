// n = NetAddr("localhost", 9999);


// global mix
n.sendMsg("/set/level/adc_cut", 1.0);
n.sendMsg("/set/level/ext_cut", 1.0);
n.sendMsg("/set/level/cut_aux", 1.0);

/// voice 1

// args: channel index, voice index, level
n.sendMsg("/set/level/in_cut", 0, 0, 1.0);
n.sendMsg("/set/level/in_cut", 1, 0, 1.0);

n.sendMsg("/set/level/cut", 0, 0.9);
n.sendMsg("/set/pan/cut", 0, 0.75);
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

n.sendMsg("/set/param/cut/rate", 0, 1.0);
/*
n.sendMsg("/set/param/cut/rate", 0, 0.165);
n.sendMsg("/set/param/cut/rate", 0, 1.75);
*/

/// voice 2
// args: channel index, voice index, level
n.sendMsg("/set/level/in_cut", 0, 1, 1.0);
n.sendMsg("/set/level/in_cut", 1, 1, 1.0);

n.sendMsg("/set/level/cut", 1, 0.9);
n.sendMsg("/set/pan/cut", 1, 0.25);
n.sendMsg("/set/enabled/cut", 1, 1.0);

n.sendMsg("/set/param/cut/rate", 1, 1.0);
n.sendMsg("/set/param/cut/loop_start", 1, 3.0);
n.sendMsg("/set/param/cut/loop_end", 1, 4.555);
n.sendMsg("/set/param/cut/loop_flag", 1, 1.0);
n.sendMsg("/set/param/cut/fade_time", 1, 0.1);
n.sendMsg("/set/param/cut/rec_level", 1, 1.0);
n.sendMsg("/set/param/cut/pre_level", 1, 0.4);
n.sendMsg("/set/param/cut/rec_flag", 1, 1);

n.sendMsg("/set/param/cut/position", 1, 1);

n.sendMsg("/set/param/cut/filter_dry", 1, 0.125);
n.sendMsg("/set/param/cut/filter_fc", 1, 800);
n.sendMsg("/set/param/cut/filter_lp", 1, 0);
n.sendMsg("/set/param/cut/filter_bp", 1, 1.0);
n.sendMsg("/set/param/cut/filter_rq", 1, 2.0);

n.sendMsg("/set/param/cut/rate", 1, 1.0);

/*
n.sendMsg("/set/param/cut/rate", 1, 0.1175);
n.sendMsg("/set/param/cut/rate", 1, 1.75);
*/



//// wooo xfeed
/*
n.sendMsg("/set/level/cut_cut", 0, 1, 0.6);
n.sendMsg("/set/level/cut_cut", 1, 0, 0.2);

n.sendMsg("/set/level/cut_cut", 1, 0, 0.0);

// freeze...
n.sendMsg("/set/param/cut/pre_level", 0, 1);
n.sendMsg("/set/param/cut/rec_level", 0, 0);
n.sendMsg("/set/param/cut/pre_level", 1, 1);
n.sendMsg("/set/param/cut/rec_level", 1, 0);

// go.
n.sendMsg("/set/param/cut/pre_level", 0, 0.5);
n.sendMsg("/set/param/cut/rec_level", 0, 1);
n.sendMsg("/set/param/cut/pre_level", 1, 0.5);
n.sendMsg("/set/param/cut/rec_level", 1, 1);

*/

////////////
// phase poll
n.sendMsg("/set/param/cut/phase_quant", 0, 0.125);
n.sendMsg("/set/param/cut/phase_quant", 1, 0.5);


~phase = OSCFunc.new({ arg msg, time, addr, recvPort;
	if(msg[1] == 0, {
		postln(msg);
	});
}, '/poll/softcut/phase');

/*
n.sendMsg("/poll/start/cut/phase");
n.sendMsg("/poll/stop/cut/phase");