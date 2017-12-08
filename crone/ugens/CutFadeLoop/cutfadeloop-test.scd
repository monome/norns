s = Server.local;
/*
s.boot;
*/
s.waitForBoot { Routine {

	b = Buffer.read(s, "/Users/emb/snd/vla-test.wav");
	s.sync;

	b.numChannels.postln;

	SynthDef.new(\cutfadeloop, {
		arg buf, out=0, amp=0.2, trig=0, rate=1, start=2, end=4, fade=0.1, loop=1;
		var snd;
		snd = CutFadeLoop.ar(buf, trig, rate, start, end, fade, loop);
		Out.ar(out, (snd*amp).dup);
	}).send(s);

	s.sync;

	z = { Out.ar(0, AnalogEcho.ar(SoundIn.ar(0) * 0.1).dup); }.play(s);
	/*
	z.free;
	*/

	x = Synth.new(\cutfadeloop, [\buf, b.bufnum], s);
	y = Synth.new(\cutfadeloop, [\buf, b.bufnum], s);

	y.set(\rate, 6/5);



	0.2.wait;

	Routine  { x.set(\trig, 0); s.sync; x.set(\trig, 1); }.play;
	Routine  { y.set(\trig, 0); s.sync; y.set(\trig, 1); }.play;

	x.set(\rate, -4/5);

	y.set(\fade, 0.8);
	y.set(\end, 6.0);

	x.set(\start, 2.2);
	x.set(\end, 4.2);
	x.set(\fade, 1.25);

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