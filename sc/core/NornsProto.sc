NornsProto {
	// address of remote client
	classvar remoteAddr;
	// port for sending OSC to matron
	classvar txPort = 8888;

	var commandPrefix = "/command";
	var <>traceCommands=false;
	var name;
	var >onFree;

	var oscfuncs;
	var <commands;
	var <polls;
	var complete = 0;
	var <server;

	var <context;

	*initClass {
		remoteAddr = NetAddr("127.0.0.1", txPort);
	}

	*spawn { arg name, func;
		^this.new(name, func);
	}

	*new { arg name, func;
		^super.new.init(name, func);
	}

	init { arg argName, func;
		name = argName;

		commands = Array.new;
		polls = Array.new;

		this.prBootServer {
			context = this.prSpawnContext;

			func.value(this);
			this.prInitOscRx;
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
		polls.do { arg poll;
			this.prStopPoll(poll);
		};
		fork {
			onFree.value;
			"engine %: freed".format(name).inform;
		};
	}

	/* engine API */

	addCommand { arg commandName, format, func;
		commandName = commandName.asSymbol;

		postln([ "engine % adding command".format(name), commandName, format, func ]);

		if (this.prCommandExists(commandName).not) {
			commands = commands.add(
				(
					name: commandName,
					format: format,
					oscfunc: OSCFunc(
						{ arg msg, time, addr, rxport;
							if (traceCommands) {
								postln(["rx command", msg, time, addr, rxport]);
							};
							func.value(msg); // TODO: consider changing to method call with "exploded" arguments...
							// func.value(*msg[1..]); // ... like so
						},
						(commandPrefix++"/"++commandName).asSymbol, // TODO: rename to cmd to optimize UDP transmission size
						recvPort: NetAddr.langPort
					)
				)
			);
		} {
			"in engine % command % not added, already exists".format(name, commandName).error;
		};
	}

	// create and define a new poll.
	// function argument should return value or data
	// type should be \value or \data
	addPoll { arg pollName, func, periodic=true, type=\value, period=0.25;
		pollName = pollName.asSymbol;

		postln([ "engine % adding poll".format(name), pollName, if (periodic, "periodic", "aperiodic"), func ]);

		if (this.prPollExists(pollName).not) {
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
						this.prPollSendValue(poll);

						poll[\period].wait;
					}
				};
			};

			polls = polls.add(poll);
			^poll
		} {
			"in engine % poll % not added, already exists".format(name, pollName).error;
		};
	}

	// update and send the poll's value
	// non-periodic polls should call this
	pollUpdate { arg pollName, val;
		if (this.prPollExists(pollName)) {
			var poll = this.prLookupPollByName(pollName);
			poll[\val] = val;
			if (poll[\isRunning]) {
				this.prPollSendValue(poll);
			};
		} {
			"in % engine, no poll named: %".format(name, pollName).error;
		};
	}

	/* engine debugging facilities */

	cmd { |commandName ... args|
		if (this.prCommandExists(commandName)) {
			var msg = [commandPrefix++"/"++commandName] ++ args;
			NetAddr.localAddr.sendMsg(*msg);
			// TODO: lookup command and invoke function instead
		} {
			"in % engine, no command named: %".format(name, commandName).error;
		}
	}

	polli { |selector, index ... args|
		this.poll(selector, polls[index][\name], *args);
	}

	poll { |selector, pollName ... args|
		if (this.prPollExists(pollName)) {
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

	tracePoll { arg pollName, toggle=true;
		if (this.prPollExists(pollName)) {
			var poll = this.prLookupPollByName(pollName);
			poll[\trace] = toggle;
		} {
			"in % engine, no poll named: %".format(name, pollName).error;
		};
	}

	/* private helper methods */

	prBootServer { |completeFunc|
		Server.scsynth;
		server = Server.local;
		server.options.memSize = 2**16;
		server.latency = 0.05;
		server.waitForBoot {
			completeFunc.value;
		};
	}

	prSpawnContext {
		var ig, xg, og;
		ig = Group.new(server);
		xg = Group.after(ig);
		og = Group.after(xg);
		^(
			server: server,
			// input, process, output groups
			// FIXME: not good naming, use an Event to match engine style
			ig: ig,
			xg: xg,
			og: og,
			// input, output busses
			in_b: Server.default.options.numOutputBusChannels,
			out_b: 0
		);
	}

	prInitOscRx {
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
				this.prStartPoll(polls[msg[1]]);
			}, '/poll/start', recvPort: NetAddr.langPort),

			// @function /poll/stop
			// @param poll index (integer)
			OSCFunc.new({
				arg msg, time, addr, recvPort;
				this.prStopPoll(polls[msg[1]]);
			}, '/poll/stop', recvPort: NetAddr.langPort),

			/// set the period of a poll
			// @function /poll/time
			// @param poll index (integer)
			// @param poll period(float)
			OSCFunc.new({
				arg msg, time, addr, recvPort;
				this.prSetPollTime(polls[msg[1]], msg[2]);
			}, '/poll/time', recvPort: NetAddr.langPort),

			/// request a poll's value
			// @function /poll/value
			// @param poll index (integer)
			OSCFunc.new({
				arg msg, time, addr, recvPort;
				this.prRequestPollValue(polls[msg[1]]);
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

	prCommandExists { arg commandName;
		^commands.any { |command| command[\name] == commandName }
	}

	prLookupCommandByName { arg commandName;
		^commands.detect { |command| command[\name] == commandName }
	}

	prPollExists { arg pollName;
		^polls.any { |poll| poll[\name] == pollName }
	}

	prLookupPollByName { arg pollName;
		^polls.detect { |poll| poll[\name] == pollName }
	}

	prStartPoll { arg poll;
		postln("in % engine, starting poll # % (%)".format(name, poll[\index], poll[\name]));
		if(poll[\isRunning].not) {
			poll[\isRunning] = true;
			if (poll[\periodic]) {
				poll[\task].play;
			};
		};
	}

	prStopPoll { arg poll;
		if(poll[\isRunning]) {
			poll[\isRunning] = false;
			if(poll[\periodic]) {
				poll[\task].stop;
			};
		};
	}

	prSetPollTime { arg poll, time;
		if (poll[\periodic].not) {
			"in % engine, setPollTime: poll # % (%) is aperiodic".format(name, poll[\index], poll[\name]).warn;
		};
		poll[\period] = time;
	}

	// triggered by remote client.
	// sends the last value, and refresh it using the value-function if there is one
	prRequestPollValue { arg poll;
		if(poll[\function].notNil) {
			poll[\val] = poll[\function].value
		};
		this.prPollSendValue(poll);
	}

	// send the most recent value
	prPollSendValue { arg poll;
		remoteAddr.sendMsg(poll[\oscPath], poll[\index], poll[\val]);
		if (poll[\trace]) {
			"in % engine, poll # % (%) sent value %".format(name, poll[\index], poll[\name], poll[\val]).inform;
		};
	}
}
