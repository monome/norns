// varispeed write head, proof of concept

s = Server.default;
s.waitForBoot { Routine {

	b = Buffer.alloc(s, s.sampleRate * 10.0);
	s.sync;

	~rec = SynthDef.new(\varihead, {arg buf=b.bufnum, rate=1.0, ratelag=0.5, end=2.0;
		var r = Lag.ar(K2A.ar(rate), ratelag);
		VariHead.ar(buf, SoundIn.ar(0), r, end:2.0);
	}).play(s);

	// play back 1st second
	~play = SynthDef.new(\player, {arg out=0, buf=b.bufnum, rate=1.0, trigrate=0.5, amp=0.25;
		var trig, snd;
		trig = Impulse.kr(trigrate);
		snd = PlayBuf.ar(1, buf, BufRateScale.kr(buf) * rate, trig) * amp;
		Out.ar(out, snd.dup);
	}).play(s, addAction:\addToTail);

	s.sync;

	// randomly change the speed of the record head
	~swoop = Routine { var rate;
		inf.do {
			rate = 2 ** (2.0.rand2);
			rate.postln;
			~rec.set(\rate, rate );
			(0.25 + 1.0.rand).wait;
	}}.play;


}.play;
};

