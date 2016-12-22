// control side API for user code
NornsApp {
	var server; // scsynth server
	var grid; // grid interface
	var timers; // collection of timers

	// private handler functions
	var handlers;

	*new { arg server_, grid_;
		^super.new.init(server_, grid_)
	}

	init { arg server_, grid_;
		handlers = Dictionary.new;
		timers = Dictionary.new;
		server = server_;
		grid = grid_;
		// glue serial grid to handler
		grid.keyDown = { arg x, y; handlers[\grid_key].value(x, y, 1); };
		grid.keyUp = { arg x, y; handlers[\grid_key].value(x, y, 0); };
		// ... glue other handlers to other device drivers...
	}

	//---- methods for setting handlers
	encoder {
		arg func = { arg n, val; /*... */} ;
		if (func.class != Function, {
			postln("error: handler not a function");
		}, {
			handlers[\enc] = func;
		});
	}

	key {
		arg func = { arg n, val; /*... */ };
		if (func.class != Function, {
			postln("error: handler not a function");
		}, {
			handlers[\key]  = func;
		});
	}


	grid_key {
		arg func = { arg x, y, val; /*... */ } ;
		if (func.class != Function, {
			postln("error: handler not a function");
		}, {
			handlers[\grid_key] = func;
		});
	}



	midi_key {
		arg func = { arg num, vel; /*... */ } ;
		if (func.class != Function, {
			postln("error: handler not a function");
		}, {
			handlers[\midi_key] = func;
		});
	}


	hid_key {
	arg func = { arg keycode; /*... */ };
		if (func.class != Function, {
			postln("error: handler not a function");
		}, {
			handlers[\hid_key] = func;
		});
	}


	hid_mouse {
		arg func = { arg dx, dy, buts; /*... */} ;
		if (func.class != Function, {
			postln("error: handler not a function");
		}, {
			handlers[\hid_mouse] = func;
		});
	}

	// create a new timer with an arbitrary ID tag.
	// (recommened that this be an Integer or Symbol)
	timer_set {
		arg tag=\bang, period=1, callback={"bang".postln;},
		data=nil, play=true;
		timers[tag] = Routine { inf.do {
			callback.value(data);
			period.wait;
		} };
		if(play, { timers[tag].play; });
	}

	timer_play { arg tag;
		timers[tag].play;
	}

	timer_stop { arg tag;
		timers[tag].stop;
	}

}

NornsTimer {
	var <> callback;
	var <> period;
	var <> routine;

	*new { arg callback, period;
		^super.new.init(callback, period);
	}

	init { arg cb = {"bang".postln;}, p=1.0;
		callback = cb;
		period = p;
		routine = Routine { inf.do { }};
		this.play;
	}

	play { routine.play; }
	stop { routine.stop; }
}