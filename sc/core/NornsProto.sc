
NornsProto {
	// address of remote client
	classvar remoteAddr;
	// port for sending OSC to matron
	classvar txPort = 8888;

	var <>traceCommands=false;
	var name;
	var >onFree;

	var oscfuncs;
	var <commands;
	var <polls;
	var complete = 0;
	var <server;

	*initClass {
		remoteAddr = NetAddr("127.0.0.1", txPort);
	}

	*new { arg name, func;
		^super.new.init(name, func);
	}

	bootServer { |completeFunc|
		Server.scsynth;
		server = Server.local;
		server.options.memSize = 2**16;
		server.latency = 0.05;
		server.waitForBoot {
			completeFunc.value;
		};
	}

	init { arg argName, func;
		name = argName;

		commands = Array.new;
		polls = Array.new;

		this.bootServer {
			func.value(this);
			this.initOscRx;
			complete = 1;
			CmdPeriod.doOnce {
				this.free;
			};
		};
	}

	free {
		"engine %: about to free...".format(name).inform;
		oscfuncs.do { arg oscfunc;
			oscfunc.free;
		};
		commands.do { arg command;
			command[\oscfunc].free;
		};
		/*
		polls.do { arg name; // TODO: ??
			CronePollRegistry.remove(name);
		};
		*/
		fork {
			onFree.value;
			"engine %: freed".format(name).inform;
		};
	}

	*spawn { arg name, func;
		^this.new(name, func);
	}

	initOscRx {
		oscfuncs = [
			/// send a `/engine/ready` response if engine is done starting up,
			/// otherwise send nothing
			// @function /ready
			OSCFunc.new({
				arg msg, time, addr, recvPort;
				if (complete==1) {
					remoteAddr.sendMsg('/engine/ready');
				}
			}, '/ready', recvPort: NetAddr.langPort),

			// @function /poll/start
			// @param poll index (integer)
			OSCFunc.new({
				arg msg, time, addr, recvPort;
				this.startPoll(polls[msg[1]]);
			}, '/poll/start', recvPort: NetAddr.langPort),

			// @function /poll/stop
			// @param poll index (integer)
			OSCFunc.new({
				arg msg, time, addr, recvPort;
				this.stopPoll(polls[msg[1]]);
			}, '/poll/stop', recvPort: NetAddr.langPort),

			/// set the period of a poll
			// @function /poll/time
			// @param poll index (integer)
			// @param poll period(float)
			OSCFunc.new({
				arg msg, time, addr, recvPort;
				this.setPollTime(polls[msg[1]], msg[2]);
			}, '/poll/time', recvPort: NetAddr.langPort),

			/// request a poll's value
			// @function /poll/value
			// @param poll index (integer)
			OSCFunc.new({
				arg msg, time, addr, recvPort;
				this.requestPollValue(polls[msg[1]]);
			}, '/poll/value', recvPort: NetAddr.langPort),

			/// shutdown engine gracefully disposing all resources
			// @function /quit
			OSCFunc.new({
				arg msg, time, addr, recvPort;
				// TODO: wait for engine.free, then:
				0.exit;
			}, '/quit', recvPort: NetAddr.langPort),
		];
	}

	addCommand { arg commandName, format, func;
		commandName = commandName.asSymbol;

		postln([ "engine % adding command".format(name), commandName, format, func ]);

		if (this.commandExists(commandName).not) {
			commands = commands.add(
				(
					name: commandName,
					format: format,
					oscfunc: OSCFunc(
						{ arg msg, time, addr, rxport;
							if (traceCommands) {
								postln(["rx command", msg, time, addr, rxport]);
							};
							func.value(*msg[1..]);
						},
						("/command/"++commandName).asSymbol, // TODO: rename to cmd to optimize UDP transmission size 
						recvPort: NetAddr.langPort
					)
				)
			);
		} {
			"in engine % command % not added, already exists".format(name, commandName).error;
		};
	}

	cmd { |commandName ... args|
		if (this.commandExists(commandName)) {
			var msg = ["/command/"++commandName] ++ args;
			NetAddr.localAddr.sendMsg(*msg);
			// TODO: lookup command and invoke function instead
		} {
			"in % engine, no command named: %".format(name, commandName).error;
		}
	}

/*
	polli { |selector, index ... args|
		this.poll(selector, polls[index][\name], *args); // TODO: works or not ???
	}
*/

	poll { |selector, pollName ... args|
		if (this.pollExists(pollName)) {
			var pollIndex = polls.detectIndex { |poll| poll[\name] == pollName };
			var msg = case
			{ selector == 'start' } {
				['/poll/start', pollIndex]
			}
			{ selector == 'stop' } {
				['/poll/stop', pollIndex]
			}
			{ selector == 'time' } {
				['/poll/time', pollIndex, args.first]
			}
			{ selector == 'value' } {
				['/poll/value', pollIndex]
			};

			NetAddr.localAddr.sendMsg(*msg);
			// TODO: lookup command and invoke function instead
		} {
			"in % engine, no poll named: %".format(name, pollName).error;
		}
	}

	commandExists { arg commandName;
		^commands.any { |command| command[\name] == commandName }
	}

	lookupCommandByName { arg commandName;
		^commands.detect { |command| command[\name] == commandName }
	}

	/* polls as events */

	// create and define a new poll.
	// function argument should return value or data
	// type should be \value or \data
	addPoll { arg pollName, func, periodic=true, type=\value, period=0.25;
		pollName = pollName.asSymbol;

		postln([ "engine % adding poll".format(name), pollName, if (periodic, "periodic", "aperiodic"), func ]);

		if (this.pollExists(pollName).not) {
			var index = polls.size;
			var poll = (
				name: pollName, // Symbol; label for outside world
				index: index, // Integer; another label, for ordering/efficiency
				period: period, // Number; callback interval
				type: type, // Symbol; either \value or \data
				function: func, // Function; what produces the value/data
// TODO				callback: callback, // Function; full (private) callback function
				periodic: periodic, // Boolean; is this a scheduled/repeating poll or not
				isRunning: false, // Boolean; running state
				oscPath: if( type == \value, '/poll/value', '/poll/data'), // Symbol, full destination path
				trace: false // Boolean; whether to dump values to sclang Post Window
			);

			if (periodic) {
				poll[\task] = Task { // Task; timing thread
					inf.do {
						var val;

						poll[\val] = poll[\function].value;
						this.pollSendValue(poll);

						poll[\period].wait;
					}
				};
			};

			polls = polls.add(poll);
		} {
			"in engine % poll % not added, already exists".format(name, pollName).error;
		};
	}

	lookupPollByName { arg pollName;
		^polls.detect { |poll| poll[\name] == pollName }
	}

	pollExists { arg pollName;
		^polls.any { |poll| poll[\name] == pollName }
	}

	startPoll { arg poll;
		postln("in % engine, starting poll # % (%)".format(name, poll[\index], poll[\name]));
		if(poll[\isRunning].not) {
			poll[\isRunning] = true;
			if (poll[\periodic]) {
				poll[\task].play;
			};
		};
	}

	stopPoll { arg poll;
		if(poll[\isRunning]) {
			poll[\isRunning] = false;
			if(poll[\periodic]) {
				poll[\task].stop;
			};
		};
	}

	setPollTime { arg poll, time;
		if (poll[\periodic].not) {
			"in % engine, setPollTime: poll # % (%) is aperiodic".format(name, poll[\index], poll[\name]).warn;
		};
		poll[\period] = time;
	}

	// update and send the poll's value
	// non-periodic polls should call this
	pollUpdate { arg pollName, val;
		if (this.pollExists(pollName)) {
			var poll = this.lookupPollByName(pollName);
			poll[\val] = val;
			if (poll[\isRunning]) {
				this.pollSendValue(poll);
			};
		} {
			"in % engine, no poll named: %".format(name, pollName).error;
		};
	}

	// triggered by remote client.
	// sends the last value, and refresh it using the value-function if there is one
	requestPollValue { arg poll;
		if(poll[\function].notNil) {
			poll[\val] = poll[\function].value
		};
		this.pollSendValue(poll);
	}

	// send the most recent value
	pollSendValue { arg poll;
		remoteAddr.sendMsg(poll[\oscPath], poll[\index], poll[\val]);
		if (poll[\trace]) {
			"in % engine, poll # % (%) sent value %".format(name, poll[\index], poll[\name], poll[\val]).inform;
		};
	}

	tracePoll { arg pollName, toggle=true;
		if (this.pollExists(pollName)) {
			var poll = this.lookupPollByName(pollName);
			poll[\trace] = toggle;
		} {
			"in % engine, no poll named: %".format(name, pollName).error;
		};
	}
}
