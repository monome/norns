Routine {

	c = Crone.ctx; c.dump;
	b = Buffer.alloc(s, s.sampleRate * 10);
	d = Buffer.alloc(s, s.sampleRate * 10);

	s.sync;

	x = SoftCutVoice.new(s, c.xg, b.bufnum, c.in_b[1].index, c.out_b.index);


//
//	c.in_b[1].scope;
//
//	c.out_b.scope;

	x.syn.set(\recRun, 1);
	x.start;
	x.reset;
	x.syn.set(\start, 0.1); x.syn.set(\end, 0.4); x.syn.set(\loop, 1);
	x.syn.set(\rec, 1.0); x.syn.set(\pre, 0.8); x.syn.set(\amp, 1);

	x.syn.set(\fade, 0.25);

	y = SoftCutVoice.new(s, c.xg, d.bufnum, c.in_b[1].index, c.out_b.index + 1);

	y.syn.set(\recRun, 1);
	y.start;
	y.reset;
	y.syn.set(\start, 0.1); y.syn.set(\end, 0.45); y.syn.set(\loop, 1);
	y.syn.set(\rate, 1.0); y.syn.set(\end, 0.65); y.syn.set(\loop, 1);
	y.syn.set(\rec, 1.0); y.syn.set(\pre, 0.8); y.syn.set(\amp, 1);

}.play;

/*
x.stop;
y.stop;

x.start;
y.start;

*/

