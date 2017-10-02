// an audio "engine."
// maintains some DSP processing and provides control over parameters and analysis results

CroneEngine {
	var <server;
	var <group;

	var <commands;
	var <commandNames;

	var <reports;
	var <reportNames;

	*new { arg serv;
		^super.new.init(serv);
	}

	init { arg serv;
		server = serv;
		group = Group.new(server);
		commands = List.new;
		commandNames = Dictionary.new;
		reports = List.new;
		reportNames = Dictionary.new;
	}

	// NB: subclasses should override this if they need to free resources
	// but the superclass method should be called as well
	kill {
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

	addReport { arg name, format, func;
		var idx, cmd;
		name = name.asSymbol;
		if(reportNames[name].isNil, {
			idx = reportNames.size;
			reportNames[name] = idx;
			cmd = Event.new;
			cmd.name = name;
			cmd.format = format;
			cmd.oscdef = OSCdef(name.asSymbol, {
				arg msg, time, addr, recvPort;
				func.value(msg);
			}, ("/report/"++name).asSymbol);
			reports.add(cmd);
		}, {
			idx = reportNames[name];
		});
		^idx
	}


}

