// define and execute a poll
CronePoll {
	// Routine; timing thread
	var routine;
	// Number; callback interval
	var <dt;
	// Funciton; to be evaluated each callback
	var func;
	// Symbol; identifies this poll to outside world
	var <name;
	// Symbol; either \value or \data
	var <type;
	// Boolean; running state
	var isRunning;
	// Symbol, full OSC path to be sent with poll results
	var <oscPath;
	// address to send to.
	var <>oscAddr;
	// current return value
	var ret;
	// callback function
	var cb;
	
	// create and define a new poll.
	// function argument should return value or data
	// type should be \value or \data
	*new { arg n, f = {}, d=0.1, t=\value;
		^super.new.init(n, f, d, t);
	}

	init { arg n, f, d, t;
		name = n;
		dt = d;
		type = t;
		func = f;
		cb = {
			ret = func.value;
			oscAddr.sendMsg(oscPath, ret);
		};
		routine = Routine { inf.do {
			cb.value;
			dt.wait;
		}};
		isRunning = false;

	}

	start {
		if(isRunning.not, { 
			routine.play;
			isRunning = true;
		});
	}

	stop {
		if(isRunning, { 
			routine.stop;
			isRunning = false;
		});
	}

	setTime { arg time;
		dt = time;
	}
	
	bang {
		cb.value();
	}
}


// singleton registry of available polls
CronePollRegistry {
	classvar <polls;

	*initClass { polls = Dictionary.new; }

	// create a CronePoll and add to the registry
	*register { arg name, func, dt=0.1, type=\value;
		name = name.asSymbol;
		if(polls.keys.includes(name), {
			postln("warning: attempted to add poll using existing key");
			^false;
		}, {
			polls[name] = CronePoll.new(name, func, dt, type);
			^true;
		});

	}

	*remove { arg name;
		name = name.asSymbol;
		if(polls.keys.includes(name), {
			polls[name].stop;
			polls.removeAt(name);
		});
	}
	
	clear {
		polls.do({ |p| p.stop; });
		polls.clear;		
	}

}
 