s.waitForBoot { Routine {

	b = Buffer.read(s, "/Users/emb/snd/vla-test.wav");
	s.sync;

	b.numChannels.postln;
	b.sampleRate.postln;

	s.sampleRate.postln;

	SynthDef.new(\cutfadeloop, {
		arg buf, out, phase_out, trig_out, amp=0.2, trig=0, rate=1, start=2, end=4, fade=0.1, loop=1;
		var snd, phase, tr;
		var bufratescale = BufRateScale.kr(buf);
		// bufratescale.poll;
		#phase, tr, snd =  CutFadeLoop.ar(buf, trig, rate * bufratescale, start, end, fade, loop);
		Out.ar(out, (snd*amp).dup);
		Out.ar(phase_out, phase.linlin(start, end, 0, 1));
		Out.ar(trig_out, tr);
		SendTrig.ar(tr, 0, 0);
//		phase.poll;
	}).send(s);

	s.sync;

	~phase_b = Bus.audio(s, 4);

	x = Synth.new(\cutfadeloop, [\buf, b.bufnum, \out, 0,
		\phase_out, ~phase_b.index,
		\trig_out, ~phase_b.index + 2
	], s);

	{ ~phase_b.scope }.defer;
	Routine  { x.set(\trig, 0); s.sync; x.set(\trig, 1); }.play;

	t = OSCFunc({ arg msg, time;
		    [time, msg].postln;
	},'/tr', s.addr);

}.play };
