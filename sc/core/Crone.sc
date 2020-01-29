// the Crone, a singleton class
// it receives OSC from *matron* and manages the current CroneEngine
Crone {
	classvar <>bootOnInit = true;
	// the audio server
	classvar <>server;
	// current CroneEngine subclass instance
	classvar <>engine;
	// available OSC functions
	classvar <>oscfunc;
	// address of remote client
	classvar <>remoteAddr;
	// port for sending OSC to matron
	classvar <>txPort = 8888;
	// server port
	classvar <>serverPort = 57122;
	// a CroneAudioContext
	classvar <>context;
	// boot completion flag
	classvar complete = 0;

	classvar useRemoteServer = false;

	classvar <croneAddr;

	*initClass {
		StartUp.add { // defer until after sclang init

			croneAddr = NetAddr("127.0.0.1", 9999);

			postln("\n-------------------------------------------------");
			postln(" Crone startup");
			postln("");
			postln(" OSC rx port: " ++ NetAddr.langPort);
			postln(" OSC tx port: " ++ txPort);
			postln(" server port: " ++ serverPort);
			postln(" server port: " ++ croneAddr.port);
			postln("--------------------------------------------------\n");

			remoteAddr = NetAddr("127.0.0.1", txPort);

			"SC_JACK_DEFAULT_INPUTS".setenv("");
			"SC_JACK_DEFAULT_OUTPUTS".setenv("");

			if (Crone.bootOnInit) {
				Crone.startBoot;
			}
		}
	}

	*runShellCommand { arg str;
		var p,l;
		p = Pipe.new(str, "r");
		l = p.getLine;
		while({l.notNil}, {l.postln; l = p.getLine; });
		p.close;
	}

	*startBoot {
		if(useRemoteServer, {
			Server.default = Server.remote(\crone, NetAddr("127.0.0.1", serverPort));
			server = Server.default;
			server.doWhenBooted {
				Crone.finishBoot;
			};
		}, {
			Server.scsynth;
			server = Server.local;
			// doesn't work on supernova - "invallid argument" - too big?
			// server.options.memSize = 2**16;
			server.latency = 0.05;
			server.waitForBoot {
				Crone.finishBoot;
			};
		});
	}

	*finishBoot {
		// FIXME: connect to `crone` client instead
		//Crone.runShellCommand("jack_connect \"crone:output_5\" \"supernova:input_1\"");
		//Crone.runShellCommand("jack_connect \"crone:output_6\" \"supernova:input_2\"");
		Crone.runShellCommand("jack_connect \"crone:output_5\" \"SuperCollider:in_1\"");
		Crone.runShellCommand("jack_connect \"crone:output_6\" \"SuperCollider:in_2\"");

		//Crone.runShellCommand("jack_connect \"supernova:output_1\" \"crone:input_5\"");
		//Crone.runShellCommand("jack_connect \"supernova:output_2\" \"crone:input_6\"");
		Crone.runShellCommand("jack_connect \"SuperCollider:out_1\" \"crone:input_5\"");
		Crone.runShellCommand("jack_connect \"SuperCollider:out_2\" \"crone:input_6\"");

		CroneDefs.sendDefs(server);
		server.sync;
		// create the audio context (boilerplate routing and analysis)
		context = CroneAudioContext.new(server);

		Crone.initOscRx;

		complete = 1;

		/// test..
		{ SinOsc.ar([218,223]) * 0.125 * EnvGen.ar(Env.linen(2, 4, 6), doneAction:2) }.play(server);

	}

	*setEngine { arg name;
		var class;
		class = CroneEngine.allSubclasses.detect({ arg n; n.asString == name.asString });
		if(class.notNil, {
			fork {
				if(engine.notNil, {
					var cond = Condition.new(false);
					postln("free engine: " ++ engine);
					engine.deinit({ cond.test = true; cond.signal; });
					cond.wait;

				});
				class.new(context, {
					arg theEngine;
					postln("-----------------------");
					postln("-- crone: done loading engine, starting reports");
					postln("--------");

					this.engine = theEngine;
					postln("engine: " ++ this.engine);

					this.reportCommands;
					this.reportPolls;
				});
			}
		}, {
		   postln("warning: didn't find engine: " ++ name.asString);
		   this.reportCommands;
		   this.reportPolls;
		});
	}

	// start a thread to continuously send a named report with a given interval
	*startPoll { arg idx;
		var poll = CronePollRegistry.getPollFromIndex(idx);
		if(poll.notNil, {
			poll.start;
		}, {
			postln("startPoll failed; couldn't find index " ++ idx);
		});
	}

	*stopPoll { arg idx;
		var poll = CronePollRegistry.getPollFromIndex(idx);
		if(poll.notNil, {
			poll.stop;
		}, {
			postln("stopPoll failed; couldn't find index " ++ idx);
		});
	}

	*setPollTime { arg idx, dt;
		var pt = CronePollRegistry.getPollFromIndex(idx);
		if(pt.notNil, {
			pt.setTime(dt);
		}, {
			postln("setPollTime failed; couldn't find index " ++ idx);
		});
	}


	*requestPollValue { arg idx;
		var poll = CronePollRegistry.getPollFromIndex(idx);
		if(poll.notNil, {
			poll.requestValue;
		});
	}

	*reportEngines {
		var names = CroneEngine.allSubclasses.select {
			|class| class.name.asString.beginsWith("Engine_");
		}.collect({ arg n;
			n.asString.split($_).drop(1).join($_)
		});
		postln('engines: ' ++ names);
		remoteAddr.sendMsg('/report/engines/start', names.size);
		names.do({ arg name, i;
			remoteAddr.sendMsg('/report/engines/entry', i, name);

		});
		remoteAddr.sendMsg('/report/engines/end');
	}

	*reportCommands {
		var commands = engine !? _.commands;
		remoteAddr.sendMsg('/report/commands/start', commands.size);
		commands.do({ arg cmd, i;
			postln('command entry: ' ++ [i, cmd.name, cmd.format]);
			remoteAddr.sendMsg('/report/commands/entry', i, cmd.name, cmd.format);
		});
		remoteAddr.sendMsg('/report/commands/end');
	}

	*reportPolls {
		var num = CronePollRegistry.getNumPolls;
		remoteAddr.sendMsg('/report/polls/start', num);
		num.do({ arg i;
			var poll = CronePollRegistry.getPollFromIndex(i);
			postln(poll.name);
			// FIXME: polls should just have format system like commands?
			remoteAddr.sendMsg('/report/polls/entry', i, poll.name, if(poll.type == \value, {0}, {1}));
		});

		remoteAddr.sendMsg('/report/polls/end');
	}

	*initOscRx {
		oscfunc = (

			// @module crone
			// @alias crone

			/// send a `/crone/ready` response if Crone is done starting up,
			/// otherwise send nothing
			// @function /ready
			'/ready':OSCFunc.new({
				arg msg, time, addr, recvPort;
				if(complete==1) {
					postln(">>> /crone/ready");
					remoteAddr.sendMsg('/crone/ready');
				}
			}, '/ready'),

			// @section report management

			/// begin OSC engine report sequence
			// @function /report/engines
			'/report/engines':OSCFunc.new({
				arg msg, time, addr, recvPort;
				this.reportEngines;
			}, '/report/engines'),

			/// begin OSC command report sequence
			// @function /report/commands
			'/report/commands':OSCFunc.new({
				arg msg, time, addr, recvPort;
				this.reportCommands;
			}, '/report/commands'),

			/// begin OSC poll report sequence
			// @function /report/polls
			'/report/polls':OSCFunc.new({
				arg msg, time, addr, recvPort;
				this.reportPolls;
			}, '/report/polls'),

			// @function /engine/free
			'/engine/free':OSCFunc.new({
				if(engine.notNil, { engine.deinit; });
			}, '/engine/free'),

			// @function /engine/load/name
			// @param engine name (string)
			'/engine/load/name':OSCFunc.new({
				arg msg, time, addr, recvPort;
				this.setEngine('Engine_' ++ msg[1]);
			}, '/engine/load/name'),

			// @function /poll/start
			// @param poll index (integer)
			'/poll/start':OSCFunc.new({
				arg msg, time, addr, recvPort;
				this.startPoll(msg[1]);
			}, '/poll/start'),

			// @function /poll/stop
			// @param poll index (integer)
			'/poll/stop':OSCFunc.new({
				arg msg, time, addr, recvPort;
				this.stopPoll(msg[1]);
			}, '/poll/stop'),

			/// set the period of a poll
			// @function /poll/time
			// @param poll index (integer)
			// @param poll period(float)
			'/poll/time':OSCFunc.new({
				arg msg, time, addr, recvPort;
				this.setPollTime(msg[1], msg[2]);
			}, '/poll/time'),


			/// set the period of a poll
			// @function /poll/request/value
			// @param poll index (integer)
			'/poll/request/value':OSCFunc.new({
				arg msg, time, addr, recvPort;
				this.requestPollValue(msg[1]);
			}, '/poll/request/value'),


			// @section AudioContext control

			// @function /audio/input/level
			// @param level in db (float: -inf..)
			'/audio/input/level':OSCFunc.new({
				arg msg, time, addr, recvPort;
				context.inputLevel(msg[1]);
			}, '/audio/input/level'),

			// @function /audio/output/level
			// @param level in db (float: -inf..)
			'/audio/output/level':OSCFunc.new({
				arg msg, time, addr, recvPort;
				context.outputLevel(msg[1]);
			}, '/audio/output/level'),

			// @function /audio/monitor/level
			// @param level (float: [0, 1])
			'/audio/monitor/level':OSCFunc.new({
				arg msg, time, addr, recvPort;
				context.monitorLevel(msg[1]);
			}, '/audio/monitor/level'),

			// @function /audio/monitor/mono
			'/audio/monitor/mono':OSCFunc.new({
				arg msg, time, addr, recvPort;
				context.monitorMono;
			}, '/audio/monitor/mono'),

			// @function /audio/monitor/stereo
			'/audio/monitor/stereo':OSCFunc.new({
				arg msg, time, addr, recvPort;
				context.monitorStereo;
			}, '/audio/monitor/stereo'),

			// toggle monitoring altogether (will cause clicks)
			// @function /audio/monitor/on
			'/audio/monitor/on':OSCFunc.new({
				arg msg, time, addr, recvPort;
				context.monitorOn;
			}, '/audio/monitor/on'),

			// @function /audio/monitor/off
			'/audio/monitor/off':OSCFunc.new({
				arg msg, time, addr, recvPort;
				context.monitorOff;
			}, '/audio/monitor/off'),

			// toggle pitch analysis (save CPU)
			// @function /audio/pitch/on
			'/audio/pitch/on':OSCFunc.new({
				arg msg, time, addr, recvPort;
				context.pitchOn;
			}, '/audio/pitch/on'),

			// @function /audio/pitch/off
			'/audio/pitch/off':OSCFunc.new({
				arg msg, time, addr, recvPort;
				context.pitchOff;
			}, '/audio/pitch/off'),

			// recompile the sclang library!
			'/recompile':OSCFunc.new({
				postln("recompile...");
				thisProcess.recompile;
			}, '/recompile'),
		);

	}
}

