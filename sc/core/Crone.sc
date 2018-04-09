// the Crone, a singleton class
// it receives OSC from *matron* and manages the current CroneEngine
Crone {
	// the audio server
	classvar <>server;
	// current CroneEngine subclass instance
	classvar <>engine;
	// available OSC functions
	classvar <>oscfunc;
	// address of remote client
	classvar <>remoteAddr;
	// port to send OSC on
	classvar <>txPort = 8888;
	// an AudioContext
	classvar <>ctx;
	// boot completion flag
	classvar complete = 0;

	// VU report thread
	classvar vuThread;
	// VU report interval
	classvar vuInterval;

	*initClass {
		StartUp.add { // defer until after sclang init

			postln("\n-------------------------------------------------");
			postln(" Crone startup");
			postln("");
			postln(" \OSC rx port: " ++ NetAddr.langPort);
			postln(" \OSC tx port: " ++ txPort);
			postln("--------------------------------------------------\n");

			// FIXME? matron address is hardcoded here
			remoteAddr =NetAddr("127.0.0.1", txPort);

			server = Server.local;
			server.options.memSize = 2**16;
			server.latency = 0.05;

			server.waitForBoot {
				CroneDefs.sendDefs(server);
				server.sync;
				// create the audio context (boilerplate routing and analysis)
				ctx = AudioContext.new(server);

				Crone.initOscRx;
				Crone.initVu;

				complete = 1;
			};

		}

	}

	*setEngine { arg name;
		var class;
		class = CroneEngine.allSubclasses.detect({ arg n; n.asString == name.asString });
		if(engine.class != class, {
			if(class.notNil, {
				if(engine.notNil, {
					postln("free engine: " ++ engine);
					engine.free;
				});
				class.new(ctx, {
					arg theEngine;
					postln("-----------------------");
					postln("-- crone: done loading engine, starting reports");
					postln("--------");
					
					this.engine = theEngine;
					postln("engine: " ++ this.engine);
					
					this.reportCommands;
					this.reportParameters;
					this.reportPolls;
				});
			});
		}, {
			// if we didn't change engines, just resend the reports
			this.reportCommands;
			this.reportParameters;
			this.reportPolls;
		});

	}

	// start a thread to continuously send a named report with a given interval
	*startPoll { arg idx;
		var poll = CronePollRegistry.getPollFromIdx(idx);
		if(poll.notNil, {
			poll.start(remoteAddr);
		}, {
			postln("startPoll failed; couldn't find index " ++ idx);
		});
	}

	*stopPoll { arg idx;

		var poll = CronePollRegistry.getPollFromIdx(idx);
		if(poll.notNil, {
			poll.stop;
		}, {
			postln("stopPoll failed; couldn't find index " ++ idx);
		});
	}

	*setPollTime { arg idx, dt;
		var pt = CronePollRegistry.getPollFromIdx(idx);
		if(pt.notNil, {
			pt.setTime(dt);
		}, {
			postln("setPollTime failed; couldn't find index " ++ idx);
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

	*reportParameters {
		var parameters = engine !? _.parameters;
		postln("parameters: " ++ parameters);
		remoteAddr.sendMsg('/report/parameters/start', parameters.size);
		parameters.do({ arg parameter, i;
			var spec = parameter.spec;
			var minval, maxval, warp, step, default, units;
			if (spec.notNil) {
				minval = spec.minval;
				maxval = spec.maxval;
				warp = Warp.warps.findKeyForValue(spec.warp.asWarp.class); // NOTE: for now, only warps having Symbol-asWarp support is sent to matron
				// TODO: might also be good to include CurveWarp specified by a number, not a Symbol, too (*initClass in core Spec.sc for details)
				step = spec.step;
				default = spec.default;
				units = spec.units;
			};

			postln('parameters entry: ' ++ [
				i,
				parameter.name,
				parameter.controlBusIndex,
				minval,
				maxval,
				warp,
				step,
				default,
				units
			]);

			remoteAddr.sendMsg(
				'/report/parameters/entry',
				i,
				parameter.name,
				parameter.controlBusIndex,
				minval,
				maxval,
				warp,
				step,
				default,
				units
			);
		});
		remoteAddr.sendMsg('/report/parameters/end');
	}

	*reportPolls {
		var num = CronePollRegistry.getNumPolls;
		remoteAddr.sendMsg('/report/polls/start', num);
		num.do({ arg i;
			var poll = CronePollRegistry.getPollFromIdx(i);
			postln(poll.name);
			// FIXME: polls should just have format system like commands?
			remoteAddr.sendMsg('/report/polls/entry', i, poll.name, if(poll.type == \value, {0}, {1}));
		});

		remoteAddr.sendMsg('/report/polls/end');
	}

	*initVu {
		// VU levels are reported to a dedicated OSC address.
		vuInterval = 0.05;
		vuThread = Routine { inf.do {
			remoteAddr.sendMsg('/poll/vu', ctx.buildVuBlob);
			vuInterval.wait;
		}}.play;
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

			/// begin OSC parameter report sequence
			// @function /report/parameters
			'/report/parameters':OSCFunc.new({
				arg msg, time, addr, recvPort;
				this.reportParameters;
			}, '/report/parameters'),

			/// begin OSC poll report sequence
			// @function /report/polls
			'/report/polls':OSCFunc.new({
				arg msg, time, addr, recvPort;
				this.reportPolls;
			}, '/report/polls'),

			// @function /engine/free
			'/engine/free':OSCFunc.new({
				if(engine.notNil, { engine.free; });
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

			// @section AudioContext control

			// @function /audio/input/level
			// @param input channel (integer: 0 or 1)
			// @param level (float: [0, 1])
			'/audio/input/level':OSCFunc.new({
				arg msg, time, addr, recvPort;
				ctx.in_s[msg[1]].set(\level, msg[2]);
			}, '/audio/input/level'),

			// @function /audio/output/level
			// @param level (float)
			// @param level (float: [0, 1])
			'/audio/output/level':OSCFunc.new({
				arg msg, time, addr, recvPort;
				ctx.out_s.set(\level, msg[1]);
			}, '/audio/output/level'),

			// @function /audio/monitor/level
			// @param level (float: [0, 1])
			'/audio/monitor/level':OSCFunc.new({
				arg msg, time, addr, recvPort;
				ctx.monitorLevel(msg[1]);
			}, '/audio/monitor/level'),

			// @function /audio/monitor/mono
			'/audio/monitor/mono':OSCFunc.new({
				arg msg, time, addr, recvPort;
				ctx.monitorMono;
			}, '/audio/monitor/mono'),

			// @function /audio/monitor/stereo
			'/audio/monitor/stereo':OSCFunc.new({
				arg msg, time, addr, recvPort;
				ctx.monitorStereo;
			}, '/audio/monitor/stereo'),

			// toggle monitoring altogether (will cause clicks)
			// @function /audio/monitor/on
			'/audio/monitor/on':OSCFunc.new({
				arg msg, time, addr, recvPort;
				ctx.monitorOn;
			}, '/audio/monitor/on'),

			// @function /audio/monitor/off
			'/audio/monitor/off':OSCFunc.new({
				arg msg, time, addr, recvPort;
				ctx.monitorOff;
			}, '/audio/monitor/off'),

			// toggle pitch analysis (save CPU)
			// @function /audio/pitch/on
			'/audio/pitch/on':OSCFunc.new({
				arg msg, time, addr, recvPort;
				ctx.pitchOn;
			}, '/audio/pitch/on'),

			// @function /audio/pitch/off
			'/audio/pitch/off':OSCFunc.new({
				arg msg, time, addr, recvPort;
				ctx.pitchOff;
			}, '/audio/pitch/off'),

			// recompile the sclang library!
			'/recompile':OSCFunc.new({
				postln("recompile...");
				thisProcess.recompile;
			}, '/recompile')

		);

	}
}

