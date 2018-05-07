
/// send
Routine {
n = NetAddr.localAddr;
n.sendMsg('/test/start');
	400.do({ |i|
		n.sendMsg('/test/ping', i);
	});
	0.01.wait;
	n.sendMsg('/test/end');
}.play;
