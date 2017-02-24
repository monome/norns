// an audio "engine." only one exists at a time.
CroneEngine {
	var <server;
	var <group;

	var <commands;
	var <commandNames;

	*new { arg serv;
		^super.new.init(serv);
	}

	init { arg serv;
		server = serv;
		group = Group.new(server);
		commands = List.new;
		commandNames = Dictionary.new;
	}

	kill {
		// TODO: let the subclasses decide how to do this more gracefully
		group.free;
		commands.do({ arg com;
			com.oscdef.free;
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

