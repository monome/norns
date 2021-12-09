n = NetAddr("localhost", 9999);
//n = NetAddr("172.20.10.5", 9999);

// test (does nothing, prints "hello")
n.sendMsg("/hello");

// "master" I/O levels
n.sendMsg("/set/level/adc", 1.0);
n.sendMsg("/set/level/dac", 1.0);

// ADC monitor level. this is _before_ the aux effects send
n.sendMsg("/set/level/monitor", 1.0);

// monitor mix levels. 1st arg is which route in 2x2 matrix:
// L -> L
n.sendMsg("/set/level/monitor_mix", 0, 0.5);
// L -> R
n.sendMsg("/set/level/monitor_mix", 1, 0.5);
// R -> L
n.sendMsg("/set/level/monitor_mix", 2, 0.5);
// R -> R
n.sendMsg("/set/level/monitor_mix", 3, 0.5);

// "ext" level (input from supercollider, pd or &c.) also pre-send
n.sendMsg("/set/level/ext", 0.5);


// VU polls
//// this will only work if `crone` is using sclang port instead of matron
~vu = OSCFunc.new({ arg msg, time, addr, recvPort;
	postln(msg);
}, '/poll/vu');

/*
n.sendMsg("/poll/start/vu");
n.sendMsg("/poll/stop/vu");
*/

// quit
/*
n.sendMsg("/goodbye");
*/