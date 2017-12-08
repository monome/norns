s = Server.local;
/*
s.boot;
*/
s.waitForBoot { Routine {

	b = Buffer.read(s, "/Users/emb/snd/vla-test.wav");
	s.sync;

	b.numChannels.postln;

	SynthDef.new(\cutfadeloop, {
		arg buf, out, phase_out, amp=0.2, trig=0, rate=1, start=2, end=4, fade=0.1, loop=1;
		var cfv, snd, phase;
		#snd, phase=  CutFadeLoop.ar(buf, trig, rate, start, end, fade, loop);
//		snd = cfv[0];
//		phase = cfv[1];
		Out.ar(out, (snd*amp).dup);
		Out.kr(phase_out, A2K.kr(phase));
	}).send(s);

	s.sync;


	~phase_b = Bus.control(s, 2);

	x = Synth.new(\cutfadeloop, [\buf, b.bufnum, \out, 0, \out_phase, ~phase_b.index], s);
	y = Synth.new(\cutfadeloop, [\buf, b.bufnum, \out, 0, \out_phase, ~phase_b.index + 1], s);

	{ ~phase_b.scope }.defer;

	y.set(\rate, 6/5);
	x.set(\rate, -4/5);

	x.set(\start, 2.0);
	y.set(\end, 6.0);
	y.set(\fade, 0.8);

	x.set(\start, 2.2);
	x.set(\end, 4.2);
	x.set(\fade, 1.25);

	Routine  { x.set(\trig, 0); s.sync; x.set(\trig, 1); }.play;
	Routine  { y.set(\trig, 0); s.sync; y.set(\trig, 1); }.play;


//	y.free;


	/*
	*/

	/*
	x.set(\loop, 0);
	x.set(\loop, 1);
	*/


}.play };

/*

z.free;

x.set(\trig, 0);
x.set(\trig, 1);

x.set(\rate, 2.0);

x.set(\start, 1.0);
x.set(\end, 2.0);

x.set(\fade, 0.1);
x.set(\loop, 1);



DiskIn

//		snd = PlayBuf.ar(1, b.bufnum, 1.0, loop:1);

b.plot;
*/