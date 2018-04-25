// one chunk at a time..
Crone.setEngine("Engine_SoftCut");

c = Crone.ctx;
e = Crone.engine;
n = NetAddr.localAddr;

// start voice 1 looping
n.sendMsg("/command/rec", 1, 1);
n.sendMsg("/command/pre", 1, 0.75);
n.sendMsg("/command/amp", 1, 1);

n.sendMsg("/command/rec_on", 1, 1);
n.sendMsg("/command/loop_on", 1, 1);

n.sendMsg("/command/play_dac", 1, 1, 1);
n.sendMsg("/command/play_dac", 1, 2, 1);
n.sendMsg("/command/adc_rec", 1, 1, 0.5);
n.sendMsg("/command/adc_rec", 2, 1, 0.5);

n.sendMsg("/command/loop_start", 1, 1);
n.sendMsg("/command/loop_end", 1, 5);
n.sendMsg("/command/pos", 1, 4);
n.sendMsg("/command/reset", 1);
n.sendMsg("/command/start", 1);

/// test the envelope
n.sendMsg("/command/env_time", 1, 5.0);
n.sendMsg("/command/stop", 1);
n.sendMsg("/command/start", 1);

n.sendMsg("/command/clear", 1);

// stop recording and mess with the playback rate..
n.sendMsg("/command/rec_on", 1, 0);

r = Routine { var rate; inf.do {
	rate = #[0.5, 0.75, 1.2, 1.5, 2.0, 0.25, 1.0, 1.0, 1.0, 0.5, 2.0, 1.5, 2.0, 1.0, 0.5].choose;
	n.sendMsg("/command/rate", 1, rate);
	1.0.wait;
}};
r.play;

n.sendMsg("/command/rate_lag", 1, 0.01);

// try resampling voice 1 to to voice 2
n.sendMsg("/command/play_rec", 1, 2, 1);

n.sendMsg("/command/play_dac", 2, 1, 1);
n.sendMsg("/command/play_dac", 2, 2, 1);

n.sendMsg("/command/rec", 2, 1);
n.sendMsg("/command/pre", 2, 0.75);
n.sendMsg("/command/amp", 2, 1);
n.sendMsg("/command/rec_on", 2, 1);

n.sendMsg("/command/loop_on", 2, 1);
n.sendMsg("/command/loop_start", 2, 1);
n.sendMsg("/command/loop_end", 2, 1.4);

n.sendMsg("/command/pos", 2, 1);
n.sendMsg("/command/reset", 2);
n.sendMsg("/command/start", 2);


// ok, set up a short loop recording into buffer 1
n.sendMsg("/command/adc_rec", 1, 5, 1);
n.sendMsg("/command/rec", 5, 1);
n.sendMsg("/command/pre", 5, 0.0);
n.sendMsg("/command/rec_on", 5, 1);

n.sendMsg("/command/loop_on", 5, 1);
n.sendMsg("/command/loop_start", 5, 2);
n.sendMsg("/command/loop_end", 5, 3.2);

n.sendMsg("/command/pos", 5, 1);
n.sendMsg("/command/reset", 5);
n.sendMsg("/command/start", 5);


// assign voice 1 to buffer 2
n.sendMsg("/command/set_buf", 1, 2);



// ok, set up a short loop recording into buffer 2 (voice 6)
n.sendMsg("/command/adc_rec", 1, 6, 1);
n.sendMsg("/command/rec", 6, 1);
n.sendMsg("/command/pre", 6, 0.75);
n.sendMsg("/command/rec_on", 6, 1);

n.sendMsg("/command/loop_on", 6, 1);
n.sendMsg("/command/loop_start", 6, 2);
n.sendMsg("/command/loop_end", 6, 3.2);

n.sendMsg("/command/pos", 6, 1);
n.sendMsg("/command/reset", 6);
n.sendMsg("/command/start", 6);


n.sendMsg("/command/stop", 1);

// fake poll
p = Routine { inf.do {
	postln(e.voices[0].phase_b.getSynchronous / c.server.sampleRate);
	0.25.wait;
} }.play;


