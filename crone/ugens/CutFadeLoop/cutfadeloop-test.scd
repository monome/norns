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
		var cfv, snd, phase, tr;
		#phase, tr, snd =  CutFadeLoop.ar(buf, trig, rate, start, end, fade, loop);
		Out.ar(out, (snd*amp).dup);
		Out.ar(phase_out, phase.linlin(start, end, 0, 1));
		SendTrig.ar(tr, 0, 0);
//		phase.poll;
	}).send(s);

	s.sync;

	~phase_b = Bus.audio(s, 2);

	x = Synth.new(\cutfadeloop, [\buf, b.bufnum, \out, 0, \phase_out, ~phase_b.index], s);
	y = Synth.new(\cutfadeloop, [\buf, b.bufnum, \out, 0, \phase_out, ~phase_b.index + 1], s);

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

	t = OSCFunc({ arg msg, time;
		    [time, msg].postln;
	},'/tr', s.addr);

}.play };
