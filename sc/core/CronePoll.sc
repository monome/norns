// define and execute a poll
CronePoll {
	var task; // Task; timing thread
	var <period; // Number; callback interval
	var function; // Function; what produces the value/data
	var <name; // Symbol; label for outside world
	var <index;  // Integer; another label, for ordering/efficiency
	var <type; 	// Symbol; either \value or \data
	var isRunning; 	// Boolean; running state
	var <oscPath; 	// Symbol, full destination path
	var <>oscAddr; 	// NetAddr; address to send to.
	var <callback; 	// Function; full (private) callback function
	var <periodic; // Boolean; is this a scheduled/repeating poll or not
	var <>value;   // last value

	// create and define a new poll.
	// function argument should return value or data
	// type should be \value or \data
	*new { arg index, name, function = {}, period=0.25, type=\value, periodic=true;
		^super.new.init(index, name, function, period, type, periodic);
	}

	init { arg argIndex, argName, argFunction, argPeriod, argType, argPeriodic;
		index = argIndex;
		name = argName.asSymbol;
		period = argPeriod;
		type = argType;
		function = argFunction;
		periodic = argPeriodic;

		oscAddr = Crone.remoteAddr;
		
		if(type == \value, {
			oscPath = '/poll/value';
		}, {
			oscPath = '/poll/data';
		});
		
		callback = {
			var val = function.value;
			//postln(val);
			this.value = val; 
			this.sendValue();
		};
		
		if(periodic, {
			task = Task { inf.do {
				//postln("poll task callback...");
				callback.value;
				period.wait;
			}};

		});
		isRunning = false;
	}

	start {
		postln("starting poll...");
		if(isRunning.not, {
			isRunning = true;
			if(periodic, {
				task.play;
			});
		});
	}

	stop {
		if(isRunning, {
			isRunning = false;
			if(periodic, {
				task.stop;
			});
		});
	}

	setTime { arg time;
		period = time;
	}

	// update and send the poll's value
	// non-periodic polls should call this
	update {
		arg val;
		value = val;
		if(isRunning, { 
			this.sendValue;
		});
	}

	// triggered by remote client.
	// sends the last value, and refresh it using the value-function if there is one
	requestValue {
		if(function.notNil, {
			value = function.value
		});
		this.sendValue;
	}

	// send the most recent value
	sendValue {
		this.oscAddr.sendMsg(oscPath, index, value);
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

	// create a CronePoll and add to the registry
	*register { arg name, func, dt=0.25, type=\value, periodic=true;
		name = name.asSymbol;
		if(polls.keys.includes(name), {
			postln("warning: attempted to add poll using existing key");
			^false;
		}, {
			pollNames[polls.size] = name;
			polls[name] = CronePoll.new(polls.size, name, func, dt, type, periodic);
			^true;
		});

	}

	*remove { arg name;
		name = name.asSymbol;
		if(polls.keys.includes(name), {
			polls[name].stop;
			pollNames.removeAt(polls[name].index);
			polls.removeAt(name);
		})
	}

	*clear {
		polls.do({ |p| p.stop; });
		polls.clear;
	}

	*getPollFromIndex { arg index;
		^polls[pollNames[index]];
	}

	*getPollFromName { arg name;
		^polls[name];
	}

	*getNumPolls { ^polls.size }

}

