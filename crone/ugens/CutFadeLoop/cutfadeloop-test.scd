s = Server.local;
/*
s.boot;
*/
s.waitForBoot { Routine {

	b = Buffer.read(s, "/Users/emb/snd/vla-test.wav");

	x = { arg trig=0, rate=1.0, start=0.0, end=4.0, fade=0.1, loop=1;
		var snd;
		snd = CutFadeLoop.ar(b.bufnum, trig, rate, start, end, fade, loop);
		Out.ar(0, snd.dup);
	}.play;



}.play };


/*
x = { (BrownNoise.ar * 0.1).dup}.play;
x.free;
*/


z = {
	Out.ar(0, AnalogEcho.ar(SoundIn.ar(0)).dup);
}.play(s);


z.free;

x.set(\trig, 0);
x.set(\trig, 1);

x.set(\rate, 2.0);

x.set(\start, 1.0);
x.set(\end, 2.0);


DiskIn

//		snd = PlayBuf.ar(1, b.bufnum, 1.0, loop:1);

b.plot;