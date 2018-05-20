// pings received
~pings = List.new;

~def_start = OSCdef.newMatching(\test_start, {
	|msg, time, addr, recvPort|
	~pings.clear;
}, '/test/start');

~def_ping = OSCdef.newMatching(\test_ping, {
	|msg, time, addr, recvPort|
	[msg, time, addr, recvPort].postln;
	~pings.add(msg[1]);
}, '/test/ping');

~def_end = OSCdef.newMatching(\test_end, {
	|msg, time, addr, recvPort|
	postln("pings received: " ++ ~pings.size);
}, '/test/end');

