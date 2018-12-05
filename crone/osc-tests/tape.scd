n = NetAddr("localhost", 9999);

p = "/home/emb/snd/tape-test-1.wav";

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