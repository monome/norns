// an audio "engine."
// maintains some DSP processing and provides control over parameters and analysis results

CroneEngine {
	// audio server
	var <server;
	// top-level group for all our synths
	var <group;
	// input busses (2x mono)
	var <in_b;
	// output bus (1x stereo)
	var <out_b;

	// list of registered commands
	var <commands;
	var <commandNames;

	// list of registered polls
	var <pollNames;

	
	*new { arg srv, grp, inb, outb;
		^super.new.init(srv, grp, inb, outb);
	}

	init { arg srv, grp, inb, outb;
		server = srv;
		group = Group.new(grp);
		in_b = inb;
		out_b = outb;
		commands = List.new;
		commandNames = Dictionary.new;
		pollNames = Set.new;
	}

	addPoll { arg name, func;
		name = name.asSymbol;
		CronePollRegistry.register(name, func);
		pollNames.add(name);
	}
	
	// NB: subclasses should override this if they need to free resources
	// but the superclass method should be called as well
	kill {
		group.free;
		commands.do({ arg com;
			com.oscdef.free;
		});
		pollNames.do({ arg name;
			CronePollRegistry.remove(name);
		});
	}

	addCommand { arg name, format, func;
		var idx, cmd;
		name = name.asSymbol;
		if(commandNames[name].isNil, {
			idx = commandNames.size;
			commandNames[name] = idx;
			cmd = Event.new;
			cmd.name = name;
			cmd.format = format;
			cmd.oscdef = OSCdef(name.asSymbol, {
				arg msg, time, addr, recvPort;
				func.value(msg);
			}, ("/command/"++name).asSymbol);
			commands.add(cmd);
		}, {
			idx = commandNames[name];
		});
		^idx
	}

}

