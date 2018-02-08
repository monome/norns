// define and execute a poll
CronePoll {
	var task; // Task; timing thread
	var <dt; // Number; callback interval
	var func; // Function; what produces the value/data
	var <name; // Symbol; label for outside world
	var <idx;  // Integer; another label, for ordering/efficiency
	var <type; 	// Symbol; either \value or \data
	var isRunning; 	// Boolean; running state
	var <oscPath; 	// Symbol, full descination path
	var <>oscAddr; 	// address to send to.
	var ret; 	// current return value
	var cb; 	// full (private) callback function

	// create and define a new poll.
	// function argument should return value or data
	// type should be \value or \data
	*new { arg i, n, f = {}, d=0.25, t=\value;
		^super.new.init(i, n, f, d, t);
	}

	init { arg i, n, f, d, t;
		idx = i;
		name = n;
		dt = d;
		type = t;
		func = f;
		if(type == \value, {
			oscPath = '/poll/value';
		}, {
			oscPath = '/poll/data';
		});
		cb = {
			ret = func.value;
			//postln("poll callback " ++ [ name , oscAddr, oscPath, ret ]);
			oscAddr.sendMsg(oscPath, idx, ret);
		};
		task = Task { inf.do {
			cb.value;
			dt.wait;
		}};
		isRunning = false;

	}

	start { arg addr;
		oscAddr = addr;
		if(isRunning.not, {
			isRunning = true;
			task.play;
		});
	}

	stop {
		if(isRunning, {
			isRunning = false;
			task.stop;
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
	classvar polls;
	classvar pollNames;

	*initClass {
		polls = Dictionary.new;
		pollNames = Dictionary.new;
	}
	//	*initClass { polls = Dictionary.new; }

	// create a CronePoll and add to the registry
	*register { arg name, func, dt=0.25, type=\value;
		name = name.asSymbol;
		if(polls.keys.includes(name), {
			postln("warning: attempted to add poll using existing key");
			^false;
		}, {
			pollNames[polls.size] = name;
			polls[name] = CronePoll.new(polls.size, name, func, dt, type);
			^true;
		});

	}

	*remove { arg name;
		name = name.asSymbol;
		if(polls.keys.includes(name), {
			polls[name].stop;
			pollNames.removeAt(polls[name].idx);
			polls.removeAt(name);
		})
	}

	*clear {
		polls.do({ |p| p.stop; });
		polls.clear;
	}

	*getPollFromIdx { arg idx;
//		postln("getting poll for idx: " ++ idx);
//		postln("poll names: " ++ pollNames);
		^polls[pollNames[idx]];
	}

	*getPollFromName { arg name;
		^polls[name];
	}

	*getNumPolls { ^polls.size }

}

