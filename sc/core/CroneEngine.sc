// an audio "engine."
// maintains some DSP processing and provides control over parameters and analysis results
// new engines should inherit from this
CroneEngine {
	var <doneCallback;

	// an AudioContext
	var <context;

	// list of registered commands
	var <commands;
	var <commandNames;

	// list of registered polls
	var <pollNames;

	*new { arg context, doneCallback;
		^super.new.init(context, doneCallback);
	}

	init { arg argContext, argDoneCallback;
		commands = List.new;
		commandNames = Dictionary.new;
		pollNames = Set.new;
		context = argContext;
		context.postln;
		doneCallback = argDoneCallback;
	}

	addPoll { arg name, func;
		name = name.asSymbol;
		CronePollRegistry.register(name, func);
		pollNames.add(name);
	}

	// NB: subclasses should override this if they need to free resources
	// but the superclass method should be called as well
	free {
		postln("CroneEngine.free");
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
		postln([ "CroneEngine adding command", name, format, func ]);
		if(commandNames[name].isNil, {
			idx = commandNames.size;
			commandNames[name] = idx;
			cmd = Event.new;
			cmd.name = name;
			cmd.format = format;
			cmd.oscdef = OSCdef(name.asSymbol, {
				arg msg, time, addr, rxport;
				// ["CroneEngine rx command", msg, time, addr, rxport].postln;
				func.value(msg);
			}, ("/command/"++name).asSymbol);
			commands.add(cmd);
		}, {
			idx = commandNames[name];
		});
		^idx
	}

}

