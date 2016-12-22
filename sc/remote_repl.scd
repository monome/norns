//--- server / interpreter

OSCdef(\interpret, {
	arg msg, time, addr, recvPort;
	var res = this.interpret(msg[1].asString);
	addr.sendMsg('/result', res.asString);
}, '/interpret');


//--- test: client

// handle result
OSCdef(\result, {
	arg msg, time, addr, recvPort;
	postln("result: " ++ msg[1].asString);
}, '/result');

// evaluate whatever
~code = " Array.geom(5, 1, 2).postln.sum ";

// loopback: send to ourself
m = NetAddr("127.0.0.1", NetAddr.langPort);
m.sendMsg('/interpret', ~code);