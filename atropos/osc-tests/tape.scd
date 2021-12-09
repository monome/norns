n = NetAddr("localhost", 9999);


// "master" I/O levels
n.sendMsg("/set/level/adc", 1.0);
n.sendMsg("/set/level/dac", 1.0);

// ADC monitor level. this is _before_ the aux effects send
n.sendMsg("/set/level/monitor", 1.0);

p = "/home/emb/snd/tape-test-6.wav";

n.sendMsg("/tape/record/open", p);
n.sendMsg("/tape/record/start");
n.sendMsg("/tape/record/stop");

// test playback
n.sendMsg("/set/level/tape", 1.0);
n.sendMsg("/tape/play/open", p);
n.sendMsg("/tape/play/start");
n.sendMsg("/tape/play/stop");

(
s = Server.local;
s.boot;
s.waitForBoot {
	b = Buffer.read(s, p);
	s.sync;
	b.plot;
	x = { Out.ar(0, PlayBuf.ar(2, b.bufnum)); }.play(s);
	/*
	x.free
	*/
};
)