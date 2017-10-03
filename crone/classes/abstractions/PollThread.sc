// class for repeatedly sending the result of some function
PollThread {
	// a Routine
	var r;
	// a time interval2
	var t;
	// a function to evaluate
	var f;

	var isRunning;
	
	*new { arg func = {}, time=0.1;
		^super.new.init(func, time);
	}

	init { arg func, time;
		f = func;
		t = time;
		r = Routine { inf.do {
			func.value;
		}};
		isRunning = false;
	}

	start {
		if(isRunning.not, { 
			r.play;
			isRunning = true;
		});
	}

	stop {
		if(isRunning, { 
			r.stop;
			isRunning = false;
		});
	}

	setTime { arg time;
		t = time;
	}
}


// a specialized collection of PollThreads, also acts as a descriptor
PollThreadCollection {
	var <>polls;

	*new {
		^super.new.init();
	}

	init {
		polls = Dictionary.new;
	}

	add { arg name, func, time=0.01;
		polls.add (name.asSymbol -> PollThread.new(func, time));
	}

	clear {
		polls.do({ |p| p.stop; });
		polls.clear;		
	}

	names {
		^polls.keys;
	}

	includes { arg key;
		^polls.includes(key.asSymbol);
	}
	
}