s = Server.local;
/*
s.boot;
*/
s.waitForBoot { Routine {

	b = Buffer.read(s, "/Users/emb/snd/vla-test.wav");
	// b.plot;
	s.sync;

	x = {
		CutFadeLoop.ar(b.bufnum, 1.0, trigger:0, start:0.0, end:4.0, loop:1).dup;
	}.play;


}.play };


x = { (BrownNoise.ar * 0.1).dup}.play;