// varispeed write head, proof of concept

s = Server.default;
s.waitForBoot { Routine {

	b = Buffer.alloc(s, s.sampleRate * 10.0);
	s.sync;

	~rec = SynthDef.new(\varihead, {arg buf=b.bufnum, rate=1.0, ratelag=0.5, start=1.0, end=5.0;
		rate = Lag.kr(rate, ratelag);
		VariHead.ar(buf, SoundIn.ar(0), rate, start, end);
	}).play(s);

	// play back 1st second
	~play = SynthDef.new(\player, {arg out=0, buf=b.bufnum, rate=1.0, trigrate=0.5, amp=0.5, pos=1.5;
		var trig, snd;
		trig = Impulse.kr(trigrate);
		snd = PlayBuf.ar(1, buf, BufRateScale.kr(buf) * rate, trig, pos * SampleRate.ir) * amp;
		Out.ar(out, snd.dup);
	}).play(s, addAction:\addToTail);

	s.sync;

	// randomly change the speed of the record head
	/*
	~swoop = Routine { var rate;
		inf.do {
			rate = 2 ** (2.0.rand2);
			rate.postln;
			~rec.set(\rate, rate );
			(0.25 + 1.0.rand).wait;
	}}.play;
	*/

}.play;
};
//
/*
~swoop.stop;
~rec.set(\rate, 2.0);
~rec.set(\start, 1.0);
~rec.set(\end, 6.0);

~play.set(\trigrate, 0.5);
~play.set(\pos, 1.5);

b.plot;
b.zero;
*/