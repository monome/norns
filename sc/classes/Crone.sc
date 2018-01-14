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
			server.waitForBoot ({
				Routine {
					// this is necessary due to a bug in sclang terminal timer!
					// GH issue 2144 on upstream supercollider
					// hoping for fix in 3.9 release...
					///... arg, actually this messes with SendTrig....
					// server.statusWatcher.stopAliveThread;
					// server.initTree;
					// server.sync;
					CroneDefs.sendDefs(server);
					server.sync;
					// create the audio context
					// sets up boilerplate routing and analysis
					ctx = AudioContext.new(server);
                    complete = 1;
				}.play;
			}); 

            // FIXME get rid of these postln's later
			oscfunc = (

				'/ready':OSCFunc.new({
					arg msg, time, addr, recvPort;
					//[msg, time, addr, recvPort].postln; 
                    if(complete==1) {
                        postln(">>> /crone/ready");
                        remoteAddr.sendMsg('/crone/ready'); 
                    }
				}, '/ready'),

				'/audio/out/level':OSCFunc.new({
					arg msg, time, addr, recvPort;
					//[msg, time, addr, recvPort].postln;
					ctx.outLevel(msg[1]);
				}, '/audio/out/level'),

				'/report/engines':OSCFunc.new({
					arg msg, time, addr, recvPort;
					[msg, time, addr, recvPort].postln;
					this.reportEngines;
				}, '/report/engines'),

				'/report/commands':OSCFunc.new({
					arg msg, time, addr, recvPort;
					[msg, time, addr, recvPort].postln;
					this.reportCommands;
				}, '/report/commands'),

				'/report/polls':OSCFunc.new({
					arg msg, time, addr, recvPort;
					[msg, time, addr, recvPort].postln;
					this.reportPolls;
				}, '/report/polls'),

				'/engine/kill':OSCFunc.new({
					if(engine.notNil, { engine.kill; });
				}, '/engine/kill'),

				'/engine/load/name':OSCFunc.new({
					arg msg, time, addr, recvPort;
					[msg,time,addr,recvPort].postln;
					this.setEngine('CroneEngine_' ++ msg[1]);
				}, '/engine/load/name'),

				'/poll/start':OSCFunc.new({
					arg msg, time, addr, recvPort;
					[msg,time,addr,recvPort].postln;
					this.startPoll(msg[1]);
				}, '/poll/start'),

				'/poll/stop':OSCFunc.new({
					arg msg, time, addr, recvPort;
					[msg,time,addr,recvPort].postln;
					this.stopPoll(msg[1]);
				}, '/poll/stop'),

				'/poll/time':OSCFunc.new({
					arg msg, time, addr, recvPort;
					[msg,time,addr,recvPort].postln;
					this.setPollTime(msg[1], msg[2]);
				}, '/poll/time')

			);
		}

	}

	*setEngine { arg name;
		var class;
		class = CroneEngine.subclasses.select({ arg n; n.asString == name.asString })[0];
		postln(class);
		if(engine.class != class, {
			if(class.notNil, {
				if(engine.notNil, {
					engine.kill;
				});
				engine = class.new(Server.default, ctx.xg, ctx.in_b, ctx.out_b);
				postln('set engine: ' ++ engine);
			});
		});
	}

	// start a thread to continuously send a named report with a given interval
	*startPoll { arg idx, intervalMs =100;
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
		var names = CroneEngine.subclasses.collect({ arg n;
			n.asString.split($_)[1]
		});
		postln('engines: ' ++ names);
		remoteAddr.sendMsg('/report/engines/start', names.size);
		names.do({ arg name, i;
			remoteAddr.sendMsg('/report/engines/entry', i, name);

		});
		remoteAddr.sendMsg('/report/engines/end');
	}

	*reportCommands {
		var commands = engine.commands;
		postln("commands: " ++ commands);
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
			var poll = CronePollRegistry.getPollFromIdx(i);
			postln(poll.name);
			// FIXME: polls should just have format system like commands
			remoteAddr.sendMsg('/report/polls/entry', i, poll.name, if(poll.type == \value, {0}, {1}));
		});

		remoteAddr.sendMsg('/report/polls/end');
	}

}
