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
	classvar <>remote_addr;
	// port to send OSC on
	classvar <>tx_port = 8888;

	//
	classvar <>polls;
	// an AudioContext
	classvar <>ctx;

	*initClass {

		polls = Dictionary.new;


		StartUp.add { // defer until after sclang init

			postln("\n-------------------------------------------------");
			postln(" Crone startup");
			postln("");
			postln(" \OSC rx port: " ++ NetAddr.langPort);
			postln(" \OSC tx port: " ++ tx_port);
			postln("--------------------------------------------------\n");

			server = Server.local;
			server.waitForBoot ({
				Routine {
					// this is necessary due to a bug in sclang terminal timer!
					// GH issue 2144 on upstream supercollider
					// hoping for fix in 3.9 release...
					server.statusWatcher.stopAliveThread;
					server.initTree;
					server.sync;
					CroneDefs.sendDefs(server);
					server.sync;
					// create the audio context
					// sets up boilerplate routing and analysis
					ctx = AudioContext.new(server);

				}.play;
			});


			// FIXME: hardcoded remote client address for now
			remote_addr =NetAddr("127.0.0.1", tx_port);

			oscfunc = (

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
				}, '/poll/stop')
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
	*startPoll { arg name, intervalMs =100;
		var pt = CronePollRegistry.polls[name.asSymbol];
		if(pt.notNil, {
			pt.start;
		});
	}

	*stopPoll { arg name;
		var pt = CronePollRegistry.polls[name.asSymbol];
		if(pt.notNil, {
			pt.stop;
		});
	}

	*setPollTime { arg name, dt;
		var pt = CronePollRegistry.polls[name.asSymbol];
		if(pt.notNil, {
			pt.setTime(dt);
		});
	}

	*reportEngines {
		var names = CroneEngine.subclasses.collect({ arg n;
			n.asString.split($_)[1]
		});
		postln('engines: ' ++ names);
		remote_addr.sendMsg('/report/engines/start', names.size);
		names.do({ arg name, i;
			remote_addr.sendMsg('/report/engines/entry', i, name);

		});
		remote_addr.sendMsg('/report/engines/end');
	}

	*reportCommands {
		var commands = engine.commands;
		postln("commands: " ++ commands);
		remote_addr.sendMsg('/report/commands/start', commands.size);
		commands.do({ arg cmd, i;
			postln('command entry: ' ++ [i, cmd.name, cmd.format]);
			remote_addr.sendMsg('/report/commands/entry', i, cmd.name, cmd.format);
		});
		remote_addr.sendMsg('/report/commands/end');
	}

	*reportPolls {

		remote_addr.sendMsg('/report/polls/start', CronePollRegistry.polls.size);
		CronePollRegistry.polls.keys.do({ arg key, i;
			var poll = CronePollRegistry.polls[key];
postln(poll.name);
			// FIXME: polls should just have format system like commands
			remote_addr.sendMsg('/report/polls/entry', i, poll.name, if(poll.type == \value, {0}, {1}));
		});

remote_addr.sendMsg('/report/polls/end'); //, CronePollRegistry.polls.size);
	}

}
