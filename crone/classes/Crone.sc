// the Crone, a singleton class
// it receives OSC from *matron* and manages the current CroneEngine
Crone {
	classvar engine;
	classvar oscfunc;
	
	*initClass {
		StartUp.add { // defer until after sclang init
			oscfunc = OSCFunc.new({
				// TODO: i guess make this a collection of responders
				// associated with different OSC patterns
				arg msg, time, addr, recvPort;
				[msg, time, addr, recvPort].postln;
			});
		}
	}

	*setEngine { arg eng;
		engine.kill;
		engine = eng;
	}
}