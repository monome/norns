// minimal testing tool for Crone, sends OSC back to interpreter


CroneTester {

	// TODO: add custom setters if we want to change these outside the class
	classvar <host;
	classvar <port;
	classvar <addr;

	*initClass  {
		//Startup.add {
		host = "127.0.0.1";
		port = NetAddr.langPort;
		addr = NetAddr(host, port);

	}


	// set the engine name
	*engine  { arg name;
		addr.sendMsg('/engine/load/name', name);
	}


	// send engine command
	*cmd { arg name, args;
		var l = List.new;
		l.add(("/command/" ++ name).asSymbol);
		l = l ++ args;
		l.postln;
		addr.sendMsg(*l);
	}

	// send poll request
	// TODO

	// handle polls
	// TODO



}